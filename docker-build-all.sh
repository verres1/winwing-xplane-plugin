#!/bin/bash

# Script to build Winwing plugin for all platforms using Docker

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "======================================"
echo "Building Winwing Plugin"
echo "Multi-Platform Docker Build"
echo "======================================"
echo ""

# Check for required environment variables
if [ -z "$XPLANE_SDK_DIR" ]; then
    echo "ERROR: XPLANE_SDK_DIR environment variable not set"
    echo "Please set it in your .bashrc:"
    echo "  export XPLANE_SDK_DIR=/path/to/xplane/SDK"
    exit 1
fi

echo "Using X-Plane SDK: $XPLANE_SDK_DIR"

if [ -n "$MACOS_OPENGL_HEADERS_DIR" ]; then
    echo "Using custom OpenGL headers: $MACOS_OPENGL_HEADERS_DIR"
fi

# Build Docker image
echo ""
echo "Building Docker image..."
docker build -t winwing-builder .

echo ""
echo "Running multi-platform build..."
docker run --rm \
    -v "$SCRIPT_DIR:/workspace" \
    -v "$XPLANE_SDK_DIR:/xplane-sdk:ro" \
    -e XPLANE_SDK_DIR=/xplane-sdk \
    -e MACOS_OPENGL_HEADERS_DIR="$MACOS_OPENGL_HEADERS_DIR" \
    winwing-builder

echo ""
echo "======================================"
echo "Creating X-Plane Plugin Package"
echo "======================================"

# Fix ownership of Docker-created files
sudo chown -R $(id -u):$(id -g) dist/ 2>/dev/null || true

# Get version from config.h
VERSION=$(grep "#define VERSION" src/include/config.h | cut -d '"' -f 2)
if [ -z "$VERSION" ]; then
    VERSION="0.0.17"
fi

# Get SDK version for package naming
SDK_VERSION=$(grep "#define kXPLM_Version" "$XPLANE_SDK_DIR/CHeaders/XPLM/XPLMDefs.h" | awk '{print $3}' | tr -d '()' || echo "411")

# Determine X-Plane version
if [ $SDK_VERSION -lt 400 ]; then
    XPLANE_VERSION="XP11"
else
    XPLANE_VERSION="XP12"
fi

# Create proper X-Plane plugin structure
PACKAGE_DIR="dist/winwing"
rm -rf "$PACKAGE_DIR" 2>/dev/null || true
mkdir -p "$PACKAGE_DIR"

# Copy platform binaries
cp -r dist/lin_x64 "$PACKAGE_DIR/" 2>/dev/null || true
cp -r dist/win_x64 "$PACKAGE_DIR/" 2>/dev/null || true
cp -r dist/mac_x64 "$PACKAGE_DIR/" 2>/dev/null || true

# Copy assets
if [ -d "assets" ]; then
    cp -r assets "$PACKAGE_DIR/" 2>/dev/null || true
fi

# Copy documentation
cp README.md "$PACKAGE_DIR/" 2>/dev/null || echo "No README found"

# Only add Skunkcrafts for XP12
if [ $SDK_VERSION -ge 400 ]; then
    echo "module|https://ramonster.nl/winwing-plugin
name|Winwing
version|$VERSION
locked|false
disabled|false
zone|custom" > "$PACKAGE_DIR/skunkcrafts_updater.cfg"
fi

# Create zip package
cd dist
ZIP_NAME="winwing-${VERSION}-${XPLANE_VERSION}.zip"
if command -v zip &> /dev/null; then
    zip -r "$ZIP_NAME" winwing/
    echo "Created: $ZIP_NAME"
else
    ZIP_NAME="winwing-${VERSION}-${XPLANE_VERSION}.tar.gz"
    tar -czf "$ZIP_NAME" winwing/
    echo "Created: $ZIP_NAME (zip not available, used tar.gz)"
fi
cd ..

echo ""
echo "======================================"
echo "Build Complete!"
echo "======================================"
echo ""
echo "✅ Plugin binaries:"
ls -lh dist/winwing/*/winwing.xpl 2>/dev/null || echo "No builds found"

echo ""
echo "📦 Installation package created:"
ls -lh dist/winwing-${VERSION}-${XPLANE_VERSION}.* 2>/dev/null

echo ""
echo "======================================"
echo "Installation Instructions"
echo "======================================"
echo ""
echo "Quick Install:"
echo "  1. Extract the package file:"
if [ -f "dist/winwing-${VERSION}-${XPLANE_VERSION}.zip" ]; then
    echo "     unzip dist/winwing-${VERSION}-${XPLANE_VERSION}.zip"
else
    echo "     tar -xzf dist/winwing-${VERSION}-${XPLANE_VERSION}.tar.gz"
fi
echo "  2. Copy the extracted 'winwing' folder to:"
echo "     X-Plane 12/Resources/plugins/"
echo ""
echo "Manual Install:"
echo "  Linux:   cp dist/winwing/lin_x64/winwing.xpl 'X-Plane 12/Resources/plugins/winwing/lin_x64/'"
echo "  Windows: cp dist/winwing/win_x64/winwing.xpl 'X-Plane 12/Resources/plugins/winwing/win_x64/'"
echo "  macOS:   cp dist/winwing/mac_x64/winwing.xpl 'X-Plane 12/Resources/plugins/winwing/mac_x64/'"
echo ""

