# Alert-Twitch-Fix

[![GitHub Repository](https://img.shields.io/badge/GitHub-Alert--Twitch--Fix-blue?logo=github)](https://github.com/Jimmyu2foru18/Alert-Twitch-Fix)

A specialized OBS Studio plugin that ensures Twitch alerts continue to function properly even when the browser source is not visible. This plugin prevents background throttling of browser sources, ensuring your alerts always display and play correctly.

## Features

### Core Functionality
- **Embedded Chromium Browser**: Full CEF (Chromium Embedded Framework) integration
- **Continuous Rendering**: Keeps Twitch alerts active even when OBS preview is minimized or obscured
- **Anti-Throttling**: Disabled background timer throttling and renderer backgrounding for reliable alert playback
- **High-Quality Audio**: Direct audio capture and routing to OBS audio subsystem for alert sounds
- **Real-time Texture Streaming**: Efficient GPU texture rendering pipeline for smooth alert animations

### User Interface
- **Alert URL Input**: Load your Twitch alert dashboard or widget URL
- **Size Presets**: Common resolutions (1080p, 720p, 4K, etc.) or custom dimensions for your alerts
- **Volume Control**: Adjustable audio volume with mute functionality for alert sounds
- **Auto Reload**: Configurable automatic refresh intervals to ensure alerts stay connected
- **Force Continuous Playback**: Ensures alerts never pause or get throttled
- **Manual Reload**: One-click refresh button for reconnecting to alert services

### Advanced Settings
- **Custom Alert Dimensions**: Precise width and height control (100-7680 x 100-4320) for your alert overlays
- **Alert Refresh Intervals**: Automatic refresh every 10 seconds to 1 hour to maintain connection
- **Alert Audio Integration**: Seamless OBS audio source enumeration for alert sounds
- **Performance Optimized**: 60 FPS rendering with efficient memory management for smooth alert animations

## Technical Architecture

### Components Overview

1. **Plugin Base** (`plugin.cpp`, `plugin.h`)
   - OBS Studio plugin API integration
   - CEF framework lifecycle management
   - Source type registration

2. **CEF Browser Engine** (`cef_browser.cpp`, `cef_browser.h`)
   - Chromium Embedded Framework integration
   - Off-screen rendering management
   - Anti-throttling command line switches
   - Browser lifecycle and navigation

3. **Audio System** (`cef_audio.cpp`, `cef_audio.h`)
   - CEF audio capture and processing
   - OBS audio source integration
   - Volume control and audio resampling
   - Real-time audio streaming

4. **Source Implementation** (`chromium_source.cpp`, `chromium_source.h`)
   - OBS source callbacks and properties
   - User interface property management
   - Texture rendering and video output
   - Settings persistence

### Anti-Throttling Technology

The plugin implements several CEF command line switches to ensure continuous rendering:

```cpp
--disable-background-timer-throttling
--disable-renderer-backgrounding
--disable-backgrounding-occluded-windows
--disable-background-media-suspend
--enable-begin-frame-scheduling
```

These switches prevent Chromium from reducing performance when the browser is not visible, ensuring smooth streaming even when OBS is minimized.

## Build Requirements

### Prerequisites
- **Visual Studio 2019/2022** (Windows)
- **CMake 3.16** or later
- **Qt6** (Core and Widgets)
- **OBS Studio SDK**
- **CEF Binary Distribution** (~300MB)

### Dependencies
- `libobs` - OBS Studio core library
- `Qt6::Core` - Qt core functionality
- `Qt6::Widgets` - Qt UI components
- `libcef_lib` - CEF main library
- `libcef_dll_wrapper` - CEF C++ wrapper

## Build Instructions

### 1. Setup Development Environment

```bash
# Clone or download OBS Studio source
git clone https://github.com/obsproject/obs-studio.git

# Download CEF Binary Distribution
# Visit: https://cef-builds.spotifycdn.com/index.html
# Download: Windows 64-bit, Standard Distribution
# Extract to: ./cef/
```

### 2. Configure CMake

```bash
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCEF_ROOT="../cef" \
         -DQt6_DIR="C:/Qt/6.5.0/msvc2019_64/lib/cmake/Qt6" \
         -Dlibobs_DIR="C:/obs-studio/build/libobs"
```

### 3. Build the Plugin

```bash
cmake --build . --config Release
```

### 4. Install

```bash
cmake --install . --config Release
```

The plugin will be installed to your OBS plugins directory:
- Windows: `%ProgramFiles%/obs-studio/obs-plugins/64bit/`

## Installation

1. Download the latest release from the [Releases](https://github.com/Jimmyu2foru18/Alert-Twitch-Fix/releases) page
2. Extract the ZIP file to your OBS Studio installation directory
3. Restart OBS Studio
4. Add a new "Alert Twitch Fix Source" to your scene

### Manual Installation

1. Copy `Alert-Twitch-Fix.dll` to `C:\Program Files\obs-studio\obs-plugins\64bit\`
2. Copy all CEF files to `C:\Program Files\obs-studio\obs-plugins\64bit\`
3. Restart OBS Studio

### Automatic Installation

Run the installer from the [Releases](https://github.com/Jimmyu2foru18/Alert-Twitch-Fix/releases) page and follow the on-screen instructions.

Alternatively, use the CMake install target to automatically place files in the correct locations.

## Usage

### Adding an Alert Twitch Fix Source

1. In OBS Studio, click the + button in the Sources panel
2. Select "Alert Twitch Fix Source" from the list
3. Configure the source properties:
   - Enter the URL of your Twitch alert widget
   - Select a size preset or set custom dimensions for your alerts
   - Adjust volume and other settings as needed
   - Make sure "Force Continuous" is enabled to keep alerts active when not visible
4. Click OK to add the source to your scene

### Property Configuration

| Property | Description | Default |
|----------|-------------|----------|
| **Alert URL** | Twitch alert widget URL to load | `about:blank` |
| **Size Preset** | Common resolution presets for alerts | 1920x1080 |
| **Custom Size** | Enable custom alert dimensions | Disabled |
| **Width** | Alert viewport width | 1920px |
| **Height** | Alert viewport height | 1080px |
| **Force Continuous** | Keep alerts rendering when hidden | Enabled |
| **Volume** | Alert sound level | 100% |
| **Muted** | Disable alert sounds | Disabled |
| **Auto Reload** | Automatic alert refresh | Disabled |
| **Reload Interval** | Time between alert reconnections | 300 seconds |

### Best Practices

1. **Performance**: Use appropriate dimensions for your Twitch alert overlays
2. **Memory**: Monitor memory usage when using multiple alert sources
3. **Network**: Ensure stable internet connection for reliable alert delivery
4. **Testing**: Test your alerts thoroughly before going live
5. **Backup**: Have fallback alert systems ready in case of network issues

## Troubleshooting

### Common Issues

**Plugin Not Loading**
- Verify all CEF binaries are in the correct directory
- Check OBS log files for error messages
- Ensure Visual C++ Redistributable is installed

**Alerts Not Showing**
- Check if Twitch alert URL is accessible
- Verify internet connection
- Try reloading the alert source manually
- Confirm your Twitch account is properly connected

**Alert Sounds Not Working**
- Check volume settings in alert source properties
- Verify alert sounds are not muted in the plugin
- Check OBS audio mixer levels
- Confirm alert sounds are enabled in your Twitch alert settings

**Alert Performance Issues**
- Reduce alert dimensions to appropriate size
- Limit number of alert sources
- Simplify alert animations and effects in your Twitch alert settings
- Close unnecessary applications
- Update graphics drivers

### Debug Logging

To enable debug logging, add these entries to your OBS log configuration:

```ini
[Alert-Twitch-Fix]
LogLevel=debug
```

Look for log entries with the prefix `[Alert-Twitch-Fix]` in your OBS log file.

## Development

### Building from Source

See [SETUP.md](SETUP.md) for detailed build instructions.

### Project Structure

```
Alert-Twitch-Fix/
├── src/                    # Source code
│   ├── cef_browser.cpp     # CEF browser implementation
│   ├── cef_browser.h       # CEF browser interface
│   ├── cef_audio.cpp       # CEF audio implementation
│   ├── cef_audio.h         # CEF audio interface
│   ├── chromium_source.cpp # OBS source implementation
│   ├── chromium_source.h   # OBS source interface
│   └── plugin.cpp          # Plugin entry point
├── resources/              # Plugin resources
│   └── icon.svg            # Source icon
├── SETUP.md                # Setup instructions
├── LICENSE                 # License information
├── .gitignore              # Git ignore file
└── build.bat               # Build script
```

### Contributing

Contributions are welcome! Please follow these steps:

1. Fork the [GitHub repository](https://github.com/Jimmyu2foru18/Alert-Twitch-Fix)
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Code Style

- Follow Google C++ Style Guide
- Use meaningful variable and function names
- Add comprehensive comments for complex logic
- Include error handling and logging

## License

This project is licensed under the MIT License. See LICENSE file for details.

## Acknowledgments

- **OBS Studio Team** - For the excellent streaming software and plugin API
- **Chromium Embedded Framework** - For providing the browser engine
- **Qt Project** - For the user interface framework
- **Community Contributors** - For testing, feedback, and improvements

## Support

For issues, questions, or contributions:
- Create an issue on GitHub
- Check existing documentation
- Review OBS Studio plugin development guides
- Consult CEF documentation for browser-specific issues

---

**Note**: This plugin requires a significant amount of system resources due to the embedded Chromium engine. However, it ensures your Twitch alerts will always display and play properly, even when the source is not visible in your OBS preview. Monitor system performance and adjust settings accordingly for optimal streaming experience.
