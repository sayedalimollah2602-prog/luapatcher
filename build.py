import PyInstaller.__main__
import os
import shutil

# Ensure dist/build directories are clean
if os.path.exists("dist"):
    shutil.rmtree("dist")
if os.path.exists("build"):
    shutil.rmtree("build")

print("Starting build process...")

PyInstaller.__main__.run([
    'main.py',
    '--name=SteamLuaPatcher',
    '--onefile',
    '--noconsole',
    '--clean',
    '--collect-all=ttkbootstrap',  # Important for themes
    '--add-data=All Games Files;All Games Files', # Format: "source;dest" (Windows)
    '--icon=NONE' # You could add an icon here if available
])

print("Build complete. Executable is in 'dist' folder.")
