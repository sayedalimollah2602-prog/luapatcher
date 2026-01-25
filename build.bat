@echo off
echo Building Steam Lua Patcher (C++ Qt6)...
echo.

REM Clean previous build
if exist build rmdir /s /q build
if exist dist rmdir /s /q dist

REM Create build directory
mkdir build
cd build

REM Check for CMake
where cmake >nul 2>&1
if %errorLevel% neq 0 (
    echo CMake not found!
    if exist setup_build_env.bat (
        echo Found environment setup script. Running it...
        call setup_build_env.bat
    ) else (
        echo.
        echo ERROR: CMake is not in your PATH.
        echo Please run 'setup_build_environment.bat' as Administrator to install dependencies.
        echo.
        pause
        exit /b 1
    )
)

REM Configure with CMake
echo Configuring with CMake...
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build
echo.
echo Building...
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

REM Create distribution folder
echo.
echo Creating distribution...
cd ..
mkdir dist
copy build\SteamLuaPatcher.exe dist\
copy logo.ico dist\ 2>nul

REM Deploy Qt dependencies
echo Deploying Qt dependencies...
windeployqt dist\SteamLuaPatcher.exe --release --no-translations

echo.
echo Build complete! Executable is in 'dist' folder.
echo.
pause
