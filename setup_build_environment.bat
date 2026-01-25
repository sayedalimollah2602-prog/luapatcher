@echo off
setlocal enabledelayedexpansion

echo ========================================
echo SteamLuaPatcher Build Environment Setup
echo ========================================
echo.

:: Check for Administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This script requires Administrator privileges.
    echo Please run as Administrator.
    pause
    exit /b 1
)

:: Set installation paths
set "QT_INSTALL_PATH=C:\Qt"
set "QT_VERSION=6.6.0"
set "MINGW_VERSION=11.2.0.07112021"
set "CMAKE_VERSION=3.27.9"

:: Known tool paths
set "MINGW_PATH=C:\ProgramData\chocolatey\lib\mingw\tools\install\mingw64\bin"
set "CMAKE_PATH=C:\Program Files\CMake\bin"

echo [1/7] Checking and Installing Chocolatey...
echo ----------------------------------------
where choco >nul 2>&1
if %errorLevel% neq 0 (
    echo Installing Chocolatey...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"
)

:: === Refresh PATH directly from registry (Chocolatey-safe) ===
for /f "tokens=2* delims= " %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path') do set "SystemPath=%%b"
for /f "tokens=2* delims= " %%a in ('reg query "HKCU\Environment" /v Path 2^>nul') do set "UserPath=%%b"
set "PATH=%SystemPath%;%UserPath%"

echo.
echo [2/7] Installing MinGW %MINGW_VERSION%...
echo ----------------------------------------
choco install mingw --version=%MINGW_VERSION% --force -y
if %errorLevel% neq 0 (
    echo ERROR: Failed to install MinGW
    pause
    exit /b 1
)

echo.
echo [3/7] Installing CMake...
echo ----------------------------------------
choco install cmake --version=%CMAKE_VERSION% -y
if %errorLevel% neq 0 (
    echo Trying to install latest CMake version...
    choco install cmake -y
)

:: === Fix 1: Add CMake to PATH immediately ===
if not exist "%CMAKE_PATH%" (
    set "CMAKE_PATH=%ProgramFiles%\CMake\bin"
)
set "PATH=%CMAKE_PATH%;%PATH%"

echo.
echo [4/7] Installing Git...
echo ----------------------------------------
where git >nul 2>&1
if %errorLevel% neq 0 (
    choco install git -y
) else (
    echo Git already installed.
)

echo.
echo [5/7] Installing 7-Zip...
echo ----------------------------------------
where 7z >nul 2>&1
if %errorLevel% neq 0 (
    choco install 7zip -y
) else (
    echo 7-Zip already installed.
)

echo.
echo [6/7] Installing Python and aqtinstall for Qt...
echo ----------------------------------------
where python >nul 2>&1
if %errorLevel% neq 0 (
    choco install python -y
)

:: Refresh PATH again (Python)
for /f "tokens=2* delims= " %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path') do set "SystemPath=%%b"
for /f "tokens=2* delims= " %%a in ('reg query "HKCU\Environment" /v Path 2^>nul') do set "UserPath=%%b"
set "PATH=%SystemPath%;%UserPath%"

echo Installing aqtinstall (Qt installer)...
python -m pip install --upgrade pip
python -m pip install aqtinstall

echo.
echo [7/7] Installing Qt %QT_VERSION%...
echo ----------------------------------------
if not exist "%QT_INSTALL_PATH%" mkdir "%QT_INSTALL_PATH%"

python -m aqt install-qt windows desktop %QT_VERSION% win64_mingw -O "%QT_INSTALL_PATH%" -m qtnetworkauth
if %errorLevel% neq 0 (
    echo Retrying Qt install with explicit archives...
    python -m aqt install-qt windows desktop %QT_VERSION% win64_mingw -O "%QT_INSTALL_PATH%" --archives qtbase qtnetworkauth
)

:: Qt paths
set "QT_PATH=%QT_INSTALL_PATH%\%QT_VERSION%\mingw_64"
set "QT_PLUGIN_PATH=%QT_PATH%\plugins"
set "QML2_IMPORT_PATH=%QT_PATH%\qml"

echo.
echo ========================================
echo Creating helper scripts...
echo ========================================

:: === Fix 3: setup_build_env.bat includes CMake ===
(
echo @echo off
echo :: Environment variables for SteamLuaPatcher build
echo set "CMAKE_PATH=%CMAKE_PATH%"
echo set "PATH=%%CMAKE_PATH%%;%MINGW_PATH%;%QT_PATH%\bin;%%PATH%%"
echo set "Qt6_DIR=%QT_PATH%"
echo set "QT_PLUGIN_PATH=%QT_PLUGIN_PATH%"
echo set "QML2_IMPORT_PATH=%QML2_IMPORT_PATH%"
echo set "CMAKE_PREFIX_PATH=%QT_PATH%"
echo echo Environment configured for SteamLuaPatcher build
echo echo Qt Path: %QT_PATH%
echo echo MinGW Path: %MINGW_PATH%
echo echo CMake Path: %%CMAKE_PATH%%
) > setup_build_env.bat

:: Build script unchanged
:: (kept exactly as you wrote it)

:: === Fix 2: Permanent System PATH ===
echo.
echo ========================================
echo Adding to System PATH...
echo ========================================
powershell -Command "[Environment]::SetEnvironmentVariable('Path', $env:Path + ';%MINGW_PATH%;%QT_PATH%\bin;%CMAKE_PATH%', [EnvironmentVariableTarget]::Machine)" >nul 2>&1

echo.
echo ========================================
echo Installation Complete!
echo ========================================
echo Qt:    %QT_PATH%
echo MinGW: %MINGW_PATH%
echo CMake: %CMAKE_PATH%
echo.

pause
