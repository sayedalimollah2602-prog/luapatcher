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
            # Archive name should be relative - flatten to root of zip
            zipf.write(file_path, os.path.basename(file_path)) 

print("Starting build process...")

PyInstaller.__main__.run([
    'main.py',
    '--name=SteamLuaPatcher',
    '--onefile',
    '--noconsole',
    '--clean',
    # PyQt6 collection
    '--collect-all=PyQt6',
    '--hidden-import=PyQt6.QtCore',
    '--hidden-import=PyQt6.QtGui',
    '--hidden-import=PyQt6.QtWidgets',
    # Bundle data files
    '--add-data=games_data.zip;.',
    '--add-data=logo.ico;.',
    # Set EXE file icon
    '--icon=logo.ico'
])

print("Build complete. Executable is in 'dist' folder.")
