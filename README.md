# Steam Lua Patcher

A modern, powerful tool designed to provide lua files for **Steam Tools**

![Release](https://img.shields.io/github/v/release/sayedalimollah2602-prog/luapatcher)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

> [!IMPORTANT]
> You must have **Steam Tools** installed for the patches to work correctly, as this tool places files in the `stplug-in` directory.

## ✨ Features

- **Game Search**: Instantly search for supported games.
- **One-Click Patching**: Automatically downloads and places Lua patch files in the correct Steam directory (`steam/config/stplug-in`).
- **Steam Integration**: Includes a built-in utility to restart Steam to apply changes immediately.
- **Standalone**: Available as a single, portable `.exe` file. So just download and run no need to install it.

## 📥 Download & Usage

1.  **Download**: Get the latest `SteamLuaPatcher.exe` from the [Releases](../../releases) page.
2.  **Run**: Double-click the executable. No installation is required.
3.  **Patch**:
    - Type the name or id of the game you want to patch.
    - Click the **Patch** button.
    - Once finished, click the **Restart Steam** button to apply changes.

## 🏗️ Architecture

### Project Structure

- **`src/`**: C++ source code for the GUI and logic.
  - `mainwindow.cpp`: Main UI logic.
  - `utils`: Few configs.
  - `workers/`: Background threads for downloading files and restarting Steam.
- **`webserver/`**: Python scripts for the backend index.
  - `generate_index.py`: Fetches Game IDs and Names from Steam APIs to generate `games_index.json`.
