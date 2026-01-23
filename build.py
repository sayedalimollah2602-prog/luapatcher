import PyInstaller.__main__
import os
import shutil
import zipfile

# Constants
GAMES_DIR = "All Games Files"
ZIP_NAME = "games_data.zip"

# Ensure dist/build directories are clean
if os.path.exists("dist"):
    shutil.rmtree("dist")
if os.path.exists("build"):
    shutil.rmtree("build")

# Create Zip archive to speed up startup (unpacking 28k files is slow)
print(f"Creating {ZIP_NAME}...")
with zipfile.ZipFile(ZIP_NAME, 'w', zipfile.ZIP_DEFLATED) as zipf:
    for root, dirs, files in os.walk(GAMES_DIR):
        for file in files:
            file_path = os.path.join(root, file)
            # Archive name should be relative to keep structure
            # We want them at root of zip so: 3800340.lua
            zipf.write(file_path, os.path.basename(file_path)) 
            # Note: The original code expected LUA_FILES_DIR to contain the files. 
            # We are flattening it here or keeping structure? 
            # Original structure: All Games Files/3800340.lua
            # If we flatten, extraction is simpler. Let's flatten.

print("Starting build process...")

PyInstaller.__main__.run([
    'main.py',
    '--name=SteamLuaPatcher',
    '--onefile',
    '--noconsole',
    '--clean',
    '--collect-all=ttkbootstrap',  # Important for themes
    '--add-data=games_data.zip;.', # Bundle the zip, not the folder
    '--add-data=logo.ico;.', # Bundle icon for runtime use
    '--icon=logo.ico' # Set EXE file icon
])

print("Build complete. Executable is in 'dist' folder.")
