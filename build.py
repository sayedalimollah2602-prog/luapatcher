import PyInstaller.__main__
import os
import shutil

# Ensure dist/build directories are clean
if os.path.exists("dist"):
    shutil.rmtree("dist")
if os.path.exists("build"):
    shutil.rmtree("build")

print("Starting build process...")
print("Note: Lua files are now served from the webserver, not bundled locally.")

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
    '--hidden-import=PyQt6.QtNetwork',
    # Bundle logo icon
    '--add-data=logo.ico;.',
    # Set EXE file icon
    '--icon=logo.ico'
])

print("Build complete. Executable is in 'dist' folder.")
print("Remember to update WEBSERVER_BASE_URL in main.py with your Vercel deployment URL!")
