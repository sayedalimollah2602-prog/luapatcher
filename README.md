<div align="center">

# ⚡ Steam Lua Patcher

**Unlock the full potential of your Steam games with automated Lua patching.**

![Release](https://img.shields.io/github/v/release/sayedalimollah2602-prog/luapatcher?style=for-the-badge&color=blueviolet)
![License](https://img.shields.io/badge/license-MIT-blue?style=for-the-badge)
![Downloads](https://img.shields.io/github/downloads/sayedalimollah2602-prog/luapatcher/total?style=for-the-badge&color=success)

</div>

---

## 🚀 Overview

**Steam Lua Patcher** is a modern, standalone tool designed to seamlessly manage Lua files for **Steam Tools**. It automates the process of finding, downloading, and installing patches, and now includes a powerful **Fix Manager** for games requiring extra file modifications.

> [!NOTE]
> **Steam Tools** must be installed for patches to work correctly. This tool automatically targets your `steam/config/stplug-in` directory.

## ✨ Key Features

### 🎮 Smart Patching
- **Instant Search**: Find supported games by name or ID.
- **Auto-Install**: One-click download and placement of Lua patches.
- **Auto-Generate**: Generate patches for unsupported games instantly.

### 🔧 Fix Manager (New!)
- **Game Fixes**: Automatically detects if a game requires additional file fixes (e.g., corrupted files, missing assets).
- **One-Click Apply**: Downloads and extracts fix files directly to your game installation folder.
- **Smart Detection**: The "Apply Fix" button appears automatically when a fix is available for your selected game.

### ⚡ Power Tools
- **Restart Steam**: Built-in utility to restart Steam and apply changes immediately.
- **Portable**: Single `.exe` file. No installation required.

---

## 📥 Getting Started

1.  **Download** the latest `SteamLuaPatcher.exe` from [Releases](../../releases).
2.  **Run** the executable (no install needed).
3.  **Search** for your game.

### How to Patch a Game
1. Select a game from the list.
2. Click **Patch Game**.
3. Click **Restart Steam** to finalize.

### How to Apply a Game Fix
1. Search for a game (e.g., _Battlefield 4_).
2. If a fix is available, an **Apply Fix** button will appear.
3. Click **Apply Fix** and select your game's installation folder.
4. The tool will download and extract the necessary files automatically.

---

## 🏗️ Architecture

- **Core**: C++ (Qt6) for high performance and native UI.
- **Backend**: Python-based webserver for dynamic index generation.
- **Safety**: Secure token authentication for API access.

## ⭐ Support the Project

If you find this tool useful, please give it a star on GitHub! It helps a lot!

<div align="center">
  <br>
  <a href="https://github.com/sayedalimollah2602-prog/luapatcher">
    <img src="https://img.shields.io/github/stars/sayedalimollah2602-prog/luapatcher?style=social&label=Star%20us%20on%20GitHub" alt="GitHub stars">
  </a>
  <br>
</div>

---

## 📄 License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

---

## 👥 Contributors

A huge thanks to the core contributors who made this project possible:

- **leVi** - Core Development & UI
- **raxnmint** - Backend & Logic

<br>

<div align="center">
<sub>Built with ❤️ by leVi & raxnmint</sub>
</div>
