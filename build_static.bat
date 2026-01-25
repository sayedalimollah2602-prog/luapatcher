@echo off
setlocal enabledelayedexpansion
echo ================================================
echo SteamLuaPatcher - Standalone Static Build
echo ================================================
echo.
echo This script builds a single standalone .exe file
echo that doesn't require any DLLs or additional files.
echo.
echo NOTE: First run requires building Qt statically
echo       which can take 2-4 hours.
echo ================================================
echo.

:: Configuration
set "QT_VERSION=6.6.0"
set "QT_STATIC_PATH=C:\Qt\Static\%QT_VERSION%"
set "QT_SOURCE_PATH=C:\Qt\Source\%QT_VERSION%"
set "BUILD_DIR=build_static"
set "OUTPUT_DIR=dist_static"

:: Check for MinGW
where mingw32-make >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: MinGW not found in PATH!
    echo Please run setup_build_environment.bat first.
    pause
    exit /b 1
)

:: Check for CMake
where cmake >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: CMake not found in PATH!
    echo Please run setup_build_environment.bat first.
    pause
    exit /b 1
)

:: Check if static Qt is already built
if exist "%QT_STATIC_PATH%\bin\qmake.exe" (
    echo Found existing static Qt build at %QT_STATIC_PATH%
    goto :build_app
)

echo.
echo ================================================
echo Static Qt not found. Building from source...
echo This will take 2-4 hours on first run.
echo ================================================
echo.

:: Check for Python (needed to configure Qt)
where python >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: Python not found! Required for Qt build.
    echo Please install Python and try again.
    pause
    exit /b 1
)

:: Download Qt source if not present
if not exist "%QT_SOURCE_PATH%" (
    echo Downloading Qt %QT_VERSION% source...
    
    mkdir "C:\Qt\Source" 2>nul
    cd /d "C:\Qt\Source"
    
    :: Use git to clone Qt
    where git >nul 2>&1
    if %errorLevel% neq 0 (
        echo ERROR: Git not found! Installing via chocolatey...
        choco install git -y
    )
    
    echo Cloning Qt base module (this is large, please wait)...
    git clone --depth 1 --branch v%QT_VERSION% https://github.com/nicktorwald/qt6-static-win-build.git QtBuildHelper 2>nul
    if exist QtBuildHelper (
        echo Using pre-configured Qt static build helper...
        cd QtBuildHelper
        call build.bat
        goto :check_build
    )
    
    :: Alternative: Download official source
    echo Downloading official Qt source...
    python -m pip install aqtinstall >nul 2>&1
    python -m aqt source-archive windows %QT_VERSION% --outputdir "C:\Qt\Source" --archives qtbase qtsvg --no-progressbar
)

:build_qt
echo.
echo Building Qt %QT_VERSION% with static configuration...
echo.

cd /d "%QT_SOURCE_PATH%"

:: Configure Qt for static build
echo Running configure...
call configure.bat -release -static -prefix "%QT_STATIC_PATH%" ^
    -opensource -confirm-license ^
    -nomake examples -nomake tests ^
    -no-opengl -opengl desktop ^
    -schannel ^
    -skip qtwebengine -skip qt3d -skip qtquick3d ^
    -skip qtmultimedia -skip qtremoteobjects

if %errorLevel% neq 0 (
    echo Qt configure failed!
    pause
    exit /b 1
)

:: Build Qt
echo.
echo Building Qt (this will take 2-4 hours)...
cmake --build . --parallel
if %errorLevel% neq 0 (
    echo Qt build failed!
    pause
    exit /b 1
)

:: Install Qt
echo Installing Qt to %QT_STATIC_PATH%...
cmake --install .

:check_build
if not exist "%QT_STATIC_PATH%\bin\qmake.exe" (
    echo ERROR: Static Qt build failed or is incomplete!
    pause
    exit /b 1
)

echo.
echo ================================================
echo Static Qt build complete!
echo ================================================

:build_app
echo.
echo Building SteamLuaPatcher with static Qt...
echo.

cd /d "%~dp0"

:: Clean previous static build
if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
if exist %OUTPUT_DIR% rmdir /s /q %OUTPUT_DIR%

:: Create build directory
mkdir %BUILD_DIR%
cd %BUILD_DIR%

:: Configure with CMake using static Qt
echo Configuring CMake...
cmake -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_STATIC=ON ^
    -DCMAKE_PREFIX_PATH="%QT_STATIC_PATH%" ^
    ..

if %errorLevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

:: Build
echo.
echo Building...
cmake --build . --config Release --parallel

if %errorLevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

:: Create output directory
echo.
echo Creating distribution...
cd ..
mkdir %OUTPUT_DIR%
copy %BUILD_DIR%\SteamLuaPatcher.exe %OUTPUT_DIR%\

:: Get file size
for %%A in (%OUTPUT_DIR%\SteamLuaPatcher.exe) do set "FILESIZE=%%~zA"
set /a FILESIZE_MB=%FILESIZE% / 1048576

echo.
echo ================================================
echo BUILD COMPLETE!
echo ================================================
echo.
echo Standalone executable created:
echo   %OUTPUT_DIR%\SteamLuaPatcher.exe
echo.
echo Size: %FILESIZE_MB% MB
echo.
echo This is a STANDALONE file - no DLLs needed!
echo Just copy the .exe and double-click to run.
echo ================================================
echo.

pause
