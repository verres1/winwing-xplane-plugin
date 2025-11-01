#!/bin/bash

# Installation script for Winwing X-Plane plugin

set -e

PLUGIN_NAME="winwing"

# Determine X-Plane path with priority: CLI argument > XPLANE_DIR env > prompt user
if [ -n "$1" ]; then
    # Command-line argument takes highest priority
    XPLANE_PATH="$1"
    echo "Using X-Plane path from command-line argument: $XPLANE_PATH"
elif [ -n "$XPLANE_DIR" ]; then
    # Use XPLANE_DIR environment variable if set
    XPLANE_PATH="$XPLANE_DIR"
    echo "Using X-Plane path from XPLANE_DIR environment variable: $XPLANE_PATH"
else
    # Prompt user to enter the path
    echo "XPLANE_DIR environment variable not set."
    read -p "Please enter the path to your X-Plane installation: " XPLANE_PATH
    if [ -z "$XPLANE_PATH" ]; then
        echo "Error: No path provided"
        exit 1
    fi
fi

# Verify X-Plane path exists
if [ ! -d "$XPLANE_PATH" ]; then
    echo "Error: X-Plane directory not found: $XPLANE_PATH"
    exit 1
fi

# Determine the platform
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM_DIR="lin_x64"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM_DIR="mac_x64"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    PLATFORM_DIR="win_x64"
else
    echo "Unknown platform: $OSTYPE"
    exit 1
fi

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check if plugin is built
if [ ! -f "build/$PLATFORM_DIR/$PLUGIN_NAME.xpl" ]; then
    echo "Error: Plugin not built. Run ./build.sh first"
    exit 1
fi

# Create plugin directory structure
PLUGIN_DIR="$XPLANE_PATH/Resources/plugins/$PLUGIN_NAME"
mkdir -p "$PLUGIN_DIR/$PLATFORM_DIR"

# Copy plugin file
echo "Installing plugin to $PLUGIN_DIR/$PLATFORM_DIR/"
cp "build/$PLATFORM_DIR/$PLUGIN_NAME.xpl" "$PLUGIN_DIR/$PLATFORM_DIR/"

# Copy assets if they exist
if [ -d "assets" ]; then
    cp -r assets "$PLUGIN_DIR/"
    echo "✓ Installed assets"
fi

# Copy README if it exists
if [ -f "README.md" ]; then
    cp "README.md" "$PLUGIN_DIR/"
fi

echo "Installation complete!"
echo "Plugin installed to: $PLUGIN_DIR/$PLATFORM_DIR/$PLUGIN_NAME.xpl"
echo ""
echo "To use the plugin:"
echo "1. Start X-Plane"
echo "2. The Winwing plugin will automatically detect your connected devices"
echo "3. Configure device profiles as needed"

