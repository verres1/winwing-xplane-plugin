#!/bin/bash

# Build script for Winwing X-Plane plugin

set -e

# Parse command line arguments
BUILD_TYPE="Release"
CLEAN_BUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug|-d)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release|-r)
            BUILD_TYPE="Release"
            shift
            ;;
        --clean|-c)
            CLEAN_BUILD=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -d, --debug     Build with debug symbols and no optimization (default: release)"
            echo "  -r, --release   Build optimized release binary (default)"
            echo "  -c, --clean     Clean build directory before building"
            echo "  -h, --help      Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Check for XPLANE_SDK_DIR environment variable
if [ -z "$XPLANE_SDK_DIR" ]; then
    echo "ERROR: XPLANE_SDK_DIR environment variable not set"
    echo "Please set it in your .bashrc or run:"
    echo "  export XPLANE_SDK_DIR=/path/to/xplane/SDK"
    echo "  ./build.sh"
    exit 1
fi

echo "Using X-Plane SDK: $XPLANE_SDK_DIR"

# Determine the platform
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="mac"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    PLATFORM="windows"
else
    echo "Unknown platform: $OSTYPE"
    exit 1
fi

# Get SDK version from SDK headers
SDK_VERSION=$(grep "#define kXPLM_Version" "$XPLANE_SDK_DIR/CHeaders/XPLM/XPLMDefs.h" | awk '{print $3}' | tr -d '()' || echo "411")

# Create build directory
BUILD_DIR="build"

if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Run CMake
echo "Configuring build for $PLATFORM with SDK version $SDK_VERSION..."
echo "Build type: $BUILD_TYPE"
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DSDK_VERSION=$SDK_VERSION ..

# Build
echo "Building plugin..."
cmake --build . --config $BUILD_TYPE -- -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "=========================================="
echo "Build complete!"
echo "=========================================="
echo "Build type: $BUILD_TYPE"
echo "Platform: $PLATFORM"
echo "Plugin output location: $BUILD_DIR/"
echo ""

if [[ "$PLATFORM" == "linux" ]]; then
    PLUGIN_PATH="lin_x64/winwing.xpl"
    echo "Linux plugin: $PLUGIN_PATH"
    if [ -f "$PLUGIN_PATH" ]; then
        SIZE_BEFORE=$(stat -c%s "$PLUGIN_PATH" 2>/dev/null || stat -f%z "$PLUGIN_PATH" 2>/dev/null)
        SIZE_BEFORE_HUMAN=$(ls -lh "$PLUGIN_PATH" | awk '{print $5}')
        echo "Plugin size: $SIZE_BEFORE_HUMAN"
        
        if [ "$BUILD_TYPE" = "Release" ]; then
            echo ""
            echo "Stripping debug symbols..."
            strip --strip-all "$PLUGIN_PATH" 2>/dev/null || strip "$PLUGIN_PATH"
            SIZE_AFTER=$(stat -c%s "$PLUGIN_PATH" 2>/dev/null || stat -f%z "$PLUGIN_PATH" 2>/dev/null)
            SIZE_AFTER_HUMAN=$(ls -lh "$PLUGIN_PATH" | awk '{print $5}')
            REDUCTION=$((100 - (SIZE_AFTER * 100 / SIZE_BEFORE)))
            echo "Stripped size: $SIZE_AFTER_HUMAN (reduced by ${REDUCTION}%)"
        fi
    fi
elif [[ "$PLATFORM" == "mac" ]]; then
    PLUGIN_PATH="mac_x64/winwing.xpl"
    echo "Mac plugin: $PLUGIN_PATH"
    if [ -f "$PLUGIN_PATH" ]; then
        SIZE_BEFORE=$(stat -f%z "$PLUGIN_PATH" 2>/dev/null || stat -c%s "$PLUGIN_PATH" 2>/dev/null)
        SIZE_BEFORE_HUMAN=$(ls -lh "$PLUGIN_PATH" | awk '{print $5}')
        echo "Plugin size: $SIZE_BEFORE_HUMAN"
        
        if [ "$BUILD_TYPE" = "Release" ]; then
            echo ""
            echo "Stripping debug symbols..."
            strip -x "$PLUGIN_PATH" 2>/dev/null || strip "$PLUGIN_PATH"
            SIZE_AFTER=$(stat -f%z "$PLUGIN_PATH" 2>/dev/null || stat -c%s "$PLUGIN_PATH" 2>/dev/null)
            SIZE_AFTER_HUMAN=$(ls -lh "$PLUGIN_PATH" | awk '{print $5}')
            REDUCTION=$((100 - (SIZE_AFTER * 100 / SIZE_BEFORE)))
            echo "Stripped size: $SIZE_AFTER_HUMAN (reduced by ${REDUCTION}%)"
        fi
    fi
elif [[ "$PLATFORM" == "windows" ]]; then
    PLUGIN_PATH="win_x64/winwing.xpl"
    echo "Windows plugin: $PLUGIN_PATH"
    if [ -f "$PLUGIN_PATH" ]; then
        SIZE_BEFORE=$(stat -c%s "$PLUGIN_PATH" 2>/dev/null || stat -f%z "$PLUGIN_PATH" 2>/dev/null)
        SIZE_BEFORE_HUMAN=$(ls -lh "$PLUGIN_PATH" | awk '{print $5}')
        echo "Plugin size: $SIZE_BEFORE_HUMAN"
        
        if [ "$BUILD_TYPE" = "Release" ]; then
            echo ""
            echo "Stripping debug symbols..."
            strip --strip-all "$PLUGIN_PATH" 2>/dev/null || strip "$PLUGIN_PATH"
            SIZE_AFTER=$(stat -c%s "$PLUGIN_PATH" 2>/dev/null || stat -f%z "$PLUGIN_PATH" 2>/dev/null)
            SIZE_AFTER_HUMAN=$(ls -lh "$PLUGIN_PATH" | awk '{print $5}')
            REDUCTION=$((100 - (SIZE_AFTER * 100 / SIZE_BEFORE)))
            echo "Stripped size: $SIZE_AFTER_HUMAN (reduced by ${REDUCTION}%)"
        fi
    fi
fi

echo "=========================================="

