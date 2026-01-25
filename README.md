# Steam Lua Patcher

A simple tool to search for Steam games, locate and copy corresponding Lua patch files to the Steam configuration directory, and restart Steam.

> [!IMPORTANT]
> To use this app, you must have **Steam Tools** installed.

## Features
- Search for Steam games
- Apply Lua patches
- Restart Steam
- Modern UI

## Download

Download the latest release from [Releases](../../releases). 

**Just download `SteamLuaPatcher.exe` and double-click to run!** No installation needed.

## Building from Source

### Standard Build (with DLLs)
1. Run `setup_build_environment.bat` as Administrator to install dependencies (Qt6, CMake, MinGW)
2. Run `build.bat` to configure and build
3. The executable + DLLs will be in the `dist` folder

### Standalone Build (single .exe)
1. Run `setup_build_environment.bat` as Administrator (if not already done)
2. Run `build_static.bat` to build with static Qt
3. A single standalone `SteamLuaPatcher.exe` will be in the `dist_static` folder

> [!NOTE]
> First static build takes 2-4 hours to compile Qt. Subsequent builds are fast.
