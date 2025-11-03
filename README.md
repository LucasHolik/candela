# Candela Brightness Control

WORK IN PROGRESS

Candela is a lightweight Windows system tray application that allows you to control your monitor's brightness with a simple slider interface. It supports both hardware (DDC/CI) and software (gamma) brightness adjustment methods.

## Features

- System tray icon for easy access
- Brightness slider (1-100%) accessible via left-click
- Toggle between hardware and software brightness control
- Real-time brightness adjustment
- Settings dialog for application preferences
- Automatic startup option
- Support for multiple monitors

## Installation

### Prerequisites

- Windows 7 or later
- Monitor that supports DDC/CI for hardware brightness control (optional)

### Building from Source

1. Install CMake and a C++ compiler (Visual Studio, MinGW, etc.)
2. Clone the repository
3. Create a build directory: `mkdir build && cd build`
4. Generate project files: `cmake ..`
5. Build: `cmake --build . --config Release`
6. The executable will be in the `bin` folder

### Using the Installer

1. Download the installer from the releases page
2. Run the installer as administrator
3. Follow the installation wizard

## Usage

- Left-click on the system tray icon to open the brightness slider
- Drag the slider to adjust brightness
- Toggle the "Hardware Brightness" checkbox to switch between hardware and software brightness control
- Right-click on the system tray icon for the context menu
- Select "Settings" to configure startup behavior and default brightness mode
- Select "Exit" to close the application

## Settings

- **Start on Windows**: Enable or disable automatic startup
- **Default Mode**: Choose between hardware or software brightness as the default
- **Current Brightness**: Shows the current brightness level (1-100%)

## Technical Details

- Built with native Win32 API
- Hardware brightness control uses Windows Monitor Configuration API
- Software brightness control adjusts gamma ramps
- Settings are stored in the Windows registry
- Single executable with no external dependencies

## License

MIT License - see the LICENSE file for details.
