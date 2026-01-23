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
    '--add-data=logo.ico;.', # Bundle icon for runtime use
    '--icon=logo.ico' # Set EXE file icon
])

print("Build complete. Executable is in 'dist' folder.")
