#!/bin/bash

# Installation script for Xbox 360 Controller driver
# Automatically detects macOS version and installs appropriate extension type

MACOS_VERSION=$(sw_vers -productVersion)
MACOS_MAJOR=$(echo $MACOS_VERSION | cut -d '.' -f 1)
MACOS_MINOR=$(echo $MACOS_VERSION | cut -d '.' -f 2)

echo "Xbox 360 Controller Driver Installation"
echo "macOS Version: $MACOS_VERSION"
echo ""

# Check if we should use system extension or kernel extension
USE_SYSTEM_EXTENSION=false

if [ "$MACOS_MAJOR" -gt 10 ]; then
    USE_SYSTEM_EXTENSION=true
elif [ "$MACOS_MAJOR" -eq 10 ] && [ "$MACOS_MINOR" -ge 15 ]; then
    USE_SYSTEM_EXTENSION=true
fi

if [ "$USE_SYSTEM_EXTENSION" = true ]; then
    echo "Installing system extension (DriverKit-based)..."
    echo "This requires user approval and may prompt for password."
    echo ""
    
    # Install system extension
    if [ -f "build/Release/Xbox360ControllerSystemExtension.dext" ]; then
        echo "Copying system extension..."
        sudo cp -R "build/Release/Xbox360ControllerSystemExtension.dext" "/Library/SystemExtensions/"
        
        echo "Activating system extension..."
        systemextensionsctl activate "com.mice.driver.Xbox360ControllerSystemExtension"
        
        echo ""
        echo "System extension installation complete!"
        echo "You may need to approve the extension in System Preferences > Security & Privacy."
    else
        echo "Error: System extension not found. Please build the driver first."
        exit 1
    fi
else
    echo "Installing kernel extension (legacy IOKit-based)..."
    echo "This requires disabling system integrity protection on newer macOS versions."
    echo ""
    
    # Install kernel extension
    if [ -f "build/Release/360Controller.kext" ]; then
        echo "Copying kernel extension..."
        sudo cp -R "build/Release/360Controller.kext" "/Library/Extensions/"
        sudo chown -R root:wheel "/Library/Extensions/360Controller.kext"
        
        echo "Loading kernel extension..."
        sudo kextload "/Library/Extensions/360Controller.kext"
        
        echo ""
        echo "Kernel extension installation complete!"
    else
        echo "Error: Kernel extension not found. Please build the driver first."
        exit 1
    fi
fi

# Install preference pane
if [ -f "build/Release/Pref360Control.prefPane" ]; then
    echo "Installing preference pane..."
    cp -R "build/Release/Pref360Control.prefPane" "~/Library/PreferencePanes/"
    echo "Preference pane installed to ~/Library/PreferencePanes/"
fi

# Install daemon
if [ -f "build/Release/360Daemon.app" ]; then
    echo "Installing daemon..."
    sudo cp -R "build/Release/360Daemon.app" "/Library/Application Support/MICE/"
    echo "Daemon installed to /Library/Application Support/MICE/"
fi

echo ""
echo "Installation complete!"
echo "Please restart your system or reconnect your Xbox 360 controller."