# Steam Lua Patcher

A modern, powerful tool designed to search for Steam games, automatically locate corresponding Lua patch files, and apply them to your Steam configuration. It features a sleek, dark-themed UI and seamless Steam integration.

![Release](https://img.shields.io/github/v/release/sayedalimollah2602-prog/luapatcher)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

## ✨ Features

-   **Game Search**: Instantly search for supported games using a rich, real-time index.
-   **One-Click Patching**: Automatically downloads and places Lua patch files in the correct Steam directory (`steam/config/stplug-in`).
-   **Steam Integration**: Includes a built-in utility to restart Steam to apply changes immediately.
-   **Modern UI**: Built with Qt6, featuring a custom dark theme, glassmorphism effects, and responsive design.
-   **Standalone**: Available as a single, portable `.exe` file.

## 📥 Download & Usage

1.  **Download**: Get the latest `SteamLuaPatcher.exe` from the [Releases](../../releases) page.
2.  **Run**: Double-click the executable. No installation is required.
3.  **Patch**:
    -   Type the name of the game you want to patch.
    -   Click the **Patch** button next to the game.
    -   Once finished, click the **Restart Steam** button to apply changes.

> [!IMPORTANT]
> You must have **Steam Tools** installed for the patches to work correctly, as this tool places files in the `stplug-in` directory.

## 🛠️ Building from Source

If you want to modify the code or build it yourself, follow these steps.

### Prerequisites

-   Windows 10/11
-   Administrator privileges (for installing dependencies via Chocolatey)

### 1. Setup Environment

We provide a one-click script to set up the entire development environment (Qt6, CMake, MinGW, Ninja, Python).

1.  Open a terminal as **Administrator**.
2.  Run the setup script:
    ```cmd
    setup_build_environment.bat
    ```
    *This will install all necessary tools and configure your path.*

### 2. Build the Project

#### Option A: Standard Build (Faster)
Useful for development. The output executable requires Qt DLLs to run.

1.  Run the build script:
    ```cmd
    build.bat
    ```
2.  The executable will be located in the `dist` folder.

#### Option B: Standalone Static Build (deployment)
Creates a single `.exe` file with no external dependencies.

1.  Run the static build script:
    ```cmd
    build_static.bat
    ```
2.  The standalone executable will be located in the `dist_static` folder.

> [!NOTE]
> The **first** time you run `build_static.bat`, it will compile Qt from source. This process can take **2-4 hours**. Subsequent builds will be much faster.

## 🏗️ Architecture

### Project Structure

-   **`src/`**: C++ source code for the GUI and logic.
    -   `mainwindow.cpp`: Main UI logic.
    -   `workers/`: Background threads for downloading files and restarting Steam.
-   **`webserver/`**: Python scripts for the backend index.
    -   `generate_index.py`: Fetches Game IDs and Names from Steam APIs to generate `games_index.json`.
-   **`resources/`**: Icons and assets.

