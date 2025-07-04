# Setup Guide for Alert-Twitch-Fix

This guide will walk you through setting up the development environment and building the Alert-Twitch-Fix plugin for OBS Studio, which ensures Twitch alerts continue to function properly even when the browser source is not visible.

## Prerequisites

### Required Software

1. **Visual Studio 2019 or 2022**
   - Download from: https://visualstudio.microsoft.com/downloads/
   - Install with "Desktop development with C++" workload
   - Ensure Windows 10/11 SDK is included

2. **CMake 3.16 or later**
   - Download from: https://cmake.org/download/
   - Add to PATH during installation

3. **Git**
   - Download from: https://git-scm.com/download/win
   - Required for cloning OBS Studio source

### Required Libraries

4. **Qt6**
   - Download from: https://www.qt.io/download-qt-installer
   - Install Qt 6.5.0 or later with MSVC 2019 64-bit compiler
   - Typical installation path: `C:\Qt\6.5.0\msvc2019_64`

5. **OBS Studio Source**
   - Clone from: https://github.com/obsproject/obs-studio.git
   - Build OBS Studio first to generate required libraries

## Step-by-Step Setup

### Step 1: Download and Build OBS Studio (Required for Twitch Alert Fix)

```bash
# Clone OBS Studio
git clone --recursive https://github.com/obsproject/obs-studio.git
cd obs-studio

# Create build directory
mkdir build
cd build

# Configure CMake (adjust Qt path as needed)
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DQt6_DIR="C:/Qt/6.5.0/msvc2019_64/lib/cmake/Qt6"

# Build OBS Studio
cmake --build . --config Release
```

### Step 2: Download CEF Binary Distribution (Powers the Alert Fix)

1. **Visit CEF Builds**: https://cef-builds.spotifycdn.com/index.html

2. **Select Version**: 
   - Choose the latest stable release
   - Platform: Windows 64-bit
   - Distribution: Standard Distribution
   - Example: `cef_binary_118.6.1+g309ba6d+chromium-118.0.5993.54_windows64.tar.bz2`

3. **Download and Extract**:
   ```bash
   # Download the CEF binary (replace with actual filename)
   # Extract to the plugin directory
   cd "C:\Users\James\Desktop\Alert-Twitch-Fix"
   
   # Create cef directory
   mkdir cef
   
   # Extract CEF binary distribution to ./cef/
   # The structure should be:
   # ./cef/
   #   ├── Release/
   #   ├── Resources/
   #   ├── include/
   #   └── cmake/
   ```

### Step 3: Verify Directory Structure

Ensure your project directory looks like this:

```
Alert-Twitch-Fix/
├── CMakeLists.txt
├── README.md
├── SETUP.md
├── build.bat
├── .gitignore
├── src/
│   ├── plugin.h
│   ├── plugin.cpp
│   ├── cef_browser.h
│   ├── cef_browser.cpp
│   ├── cef_audio.h
│   ├── cef_audio.cpp
│   ├── chromium_source.h
│   └── chromium_source.cpp
├── resources/
│   └── icon.svg
└── cef/                    # CEF binary distribution
    ├── Release/
    │   ├── libcef.dll
    │   ├── chrome_elf.dll
    │   └── ... (other CEF binaries)
    ├── Resources/
    │   ├── locales/
    │   └── ... (CEF resources)
    ├── include/
    │   └── ... (CEF headers)
    └── cmake/
        └── ... (CEF CMake files)
```

### Step 4: Configure Build Paths

Edit the `build.bat` file to match your system paths:

```batch
REM Update these paths to match your installation
set "OBS_STUDIO_DIR=C:\path\to\obs-studio"
set "QT_DIR=C:\Qt\6.5.0\msvc2019_64"
set "CEF_DIR=%~dp0cef"
```

### Step 5: Build the Alert-Twitch-Fix Plugin

#### Option A: Using Build Script (Recommended)

```bash
# Open Command Prompt in plugin directory
cd "file location"

# Run build script
build.bat

# For debug build
build.bat debug

# To clean build directory
build.bat clean
```

#### Option B: Manual CMake Build

```bash
# Create build directory
mkdir build
cd build

# Configure CMake
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCEF_ROOT="../cef" \
         -DQt6_DIR="C:/Qt/6.5.0/msvc2019_64/lib/cmake/Qt6" \
         -Dlibobs_DIR="C:/obs-studio/build/libobs"

# Build
cmake --build . --config Release

# Install
cmake --install . --config Release
```

## Installation

### Automatic Installation

The build script will offer to install the plugin automatically. This copies all necessary files to the OBS plugins directory.

### Manual Installation

1. **Copy Plugin DLL**:
   ```
   Source: build/Release/Alert-Twitch-Fix.dll
   Target: C:/Program Files/obs-studio/obs-plugins/64bit/
   ```

2. **Copy CEF Binaries**:
   ```
   Source: cef/Release/*.dll
   Target: C:/Program Files/obs-studio/obs-plugins/64bit/
   ```

3. **Copy CEF Resources**:
   ```
   Source: cef/Resources/
   Target: C:/Program Files/obs-studio/obs-plugins/64bit/Resources/
   ```

## Troubleshooting

### Common Issues

**CMake Configuration Fails**
- Verify all paths in build script are correct
- Ensure OBS Studio was built successfully
- Check that Qt6 is properly installed

**CEF Not Found**
- Verify CEF binary distribution is extracted to `./cef/`
- Check that `cef/Release/libcef.dll` exists
- Ensure CEF version is compatible (Windows 64-bit)

**Build Errors**
- Check Visual Studio installation includes C++ tools
- Verify Windows SDK is installed
- Ensure all dependencies are built for the same architecture (x64)

**Alert-Twitch-Fix Plugin Not Loading in OBS**
- Check OBS log files for error messages
- Verify all CEF binaries are in the plugins directory
- Ensure Visual C++ Redistributable is installed

### Debug Information

**Enable Verbose Logging**:
1. Start OBS with `--verbose` flag
2. Check Help → Log Files → View Current Log File
3. Look for `[Alert-Twitch-Fix]` entries

**Test CEF Installation**:
```cpp
// Check if CEF binaries are accessible
dir "C:\Program Files\obs-studio\obs-plugins\64bit\libcef.dll"
dir "C:\Program Files\obs-studio\obs-plugins\64bit\Resources"
```

## Development Tips

### Debugging

1. **Use Debug Build**: Build with `build.bat debug` for debugging symbols
2. **Attach Debugger**: Use Visual Studio to attach to OBS process
3. **Log Messages**: Add `blog(LOG_INFO, "message")` for debugging

### Testing

1. **Start Simple**: Test with basic Twitch alert widgets first
2. **Check Performance**: Monitor CPU and memory usage during alerts
3. **Test Edge Cases**: Try different alert types (follows, subscriptions, donations)

### Contributing

1. **Code Style**: Follow existing code formatting
2. **Error Handling**: Always check return values and handle errors
3. **Documentation**: Update README and comments for changes
4. **Testing**: Test on different systems and OBS versions

## Support

If you encounter issues:

1. Check this setup guide thoroughly
2. Review the main README.md file
3. Check OBS Studio plugin development documentation
4. Consult CEF documentation for browser-specific issues
5. Create an issue with detailed error information

## Next Steps

After successful installation:

1. Restart OBS Studio
2. Add a new "Alert Twitch Fix Source"
3. Configure your Twitch alert URL and settings
4. Test with your Twitch alert system
5. Monitor performance and adjust settings as needed

Congratulations! You now have a fully functional Alert-Twitch-Fix plugin for OBS Studio that ensures your Twitch alerts will always display and play correctly, even when the source is not visible.