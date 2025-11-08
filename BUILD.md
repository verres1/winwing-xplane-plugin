# Build Instructions

This document describes how to build the Winwing X-Plane plugin.

## Prerequisites

### Environment Setup

You need to set the `XPLANE_SDK_DIR` environment variable to point to your X-Plane SDK directory:

```bash
export XPLANE_SDK_DIR=/path/to/xplane/SDK
```

Add this to your `.bashrc` or `.zshrc` to make it permanent:

```bash
echo 'export XPLANE_SDK_DIR=/path/to/xplane/SDK' >> ~/.bashrc
source ~/.bashrc
```

### Native Build Dependencies

#### Linux
```bash
sudo apt-get install build-essential cmake git libudev-dev libssl-dev
```

#### macOS
```bash
xcode-select --install
brew install cmake
```

#### Windows
- Visual Studio 2019 or later with C++ support
- CMake 3.16 or later

## Build Methods

### Method 1: Native Build (Current Platform Only)

Build for your current platform:

```bash
./build.sh [OPTIONS]
```

#### Build Options

- **Release build (default - optimized, no debug symbols):**
  ```bash
  ./build.sh
  # or explicitly:
  ./build.sh --release
  ```

- **Debug build (no optimization, with debug symbols):**
  ```bash
  ./build.sh --debug
  ```

- **Clean build (remove build directory first):**
  ```bash
  ./build.sh --clean
  ```

- **Combine options:**
  ```bash
  ./build.sh --clean --release
  ./build.sh --clean --debug
  ```

- **Show help:**
  ```bash
  ./build.sh --help
  ```

#### Build Process

The script will:
1. Detect your platform (Linux, macOS, or Windows)
2. Configure CMake with appropriate settings
3. Build the plugin with the selected optimization level:
   - **Release:** `-O3` optimization, `NDEBUG` defined, no debug symbols
   - **Debug:** `-O0` (no optimization), `-g` debug symbols
4. Output the plugin to `build/[platform]_x64/winwing.xpl`
5. **Automatically strip debug symbols** in Release builds (shows size reduction)
6. Display plugin size (before and after stripping for Release builds)

#### Optimization Levels

- **Release builds** (`-O3`):
  - Maximum optimization for production use
  - Smaller binary size
  - Better runtime performance
  - No debug symbols (automatically stripped)
  - All assertions disabled (`NDEBUG`)
  - Uses platform-appropriate strip commands:
    - Linux: `strip --strip-all`
    - macOS: `strip -x`
    - Windows: `strip --strip-all`

- **Debug builds** (`-O0`):
  - No optimization for easier debugging
  - Full debug symbols (`-g`)
  - Larger binary size
  - Easier to debug with GDB/LLDB
  - All assertions enabled

### Method 2: Docker Multi-Platform Build (Recommended)

Build for all platforms using Docker (Linux, Windows, MacOS):

```bash
./docker-build-all.sh
```

This will:
1. Build a Docker image with cross-compilation tools
2. Compile for Linux (native), Windows (MinGW), and macOS (OSXCross)
3. Create a distribution package with all platform builds
4. Output: `dist/winwing-[version]-XP12.zip`

**Note:** The macOS build may fail in Docker. For best results, build macOS binaries natively on a Mac.

### Method 3: Create Package from Local Builds

If you've built locally and want to create a distribution package:

```bash
./create-package.sh
```

This creates a package from any existing builds in the `build/` directory.

## Installation

### Quick Install

After building, install to your X-Plane directory:

```bash
./install.sh [path-to-xplane]
```

If no path is provided, it defaults to `/games/X-Plane 12`.

### Manual Install

Copy the appropriate platform directory to your X-Plane plugins folder:

```bash
cp -r build/[platform]_x64 "X-Plane 12/Resources/plugins/winwing/"
```

Where `[platform]` is:
- `lin` for Linux
- `mac` for macOS  
- `win` for Windows

## Docker Build Details

### Docker Image Contents

The Docker build environment includes:
- GCC for native Linux compilation
- MinGW-w64 for Windows cross-compilation
- OSXCross for macOS cross-compilation (using macOS 12.3 SDK)
- All necessary build tools and libraries

### Customizing the Build

You can customize the build by editing:
- `CMakeLists.txt` - Main CMake configuration
- `toolchain-win.cmake` - Windows cross-compilation settings
- `toolchain-mac.cmake` - macOS cross-compilation settings
- `toolchain-lin.cmake` - Linux native build settings
