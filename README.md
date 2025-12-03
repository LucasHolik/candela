# Candela Brightness Control

Candela is a lightweight system tray application for Windows that allows you to control your monitor's brightness using both hardware (DDC/CI) and software (gamma) controls.
Currently it is more 'function over form', it may be updated to look nicer in the future.
It has not been tested with multiple monitors so functionality maybe limited.

## Installation

To install Candela, download the latest `Candela-Setup.exe` from the releases page and run the installer.

## Building from Source

If you prefer to build the application from source, you will need the following prerequisites.

### Prerequisites

- **A C++ Compiler**: MinGW/g++ is recommended. You can download MinGW-w64 from [mingw-w64.org](https://mingw-w64.org/).
- **GNU Make**: A build automation tool. Often included with MinGW or can be downloaded from [gnu.org/software/make](https://www.gnu.org/software/make/).
- **Windows SDK**: Provides necessary headers and libraries for Windows development. You can install it via Visual Studio Installer or as a standalone component from [developer.microsoft.com/windows/downloads/windows-sdk/](https://developer.microsoft.com/windows/downloads/windows-sdk/).
- **NSIS**: A professional open source system to create Windows installers (only if you want to build the installer yourself). Download from [nsis.sourceforge.io/Download](https://nsis.sourceforge.io/Download).

Make sure all of these tools are available in your system's PATH.

### Building the Application

1.  Clone the repository:

    ```sh
    git clone https://github.com/your-username/candela.git
    cd candela
    ```

2.  Build the application:
    ```sh
    make
    ```

After the build is complete, you will find `candela.exe` in the `build` directory.

### Building the Installer

To create the installer, you need to have `makensis.exe` (from the NSIS installation) in your PATH.

1.  Make sure you have successfully built the application and `candela.exe` is present in the `build` directory.

2.  Run the `makensis` command from the root of the project directory:
    ```sh
    makensis installer.nsi
    ```

This will create `Candela-Setup.exe` in the root of the project directory. Run this to install candela.
