@echo off
REM OBS Chromium Plugin Build Script
REM This script automates the build process for Windows

setlocal enabledelayedexpansion

echo ========================================
echo OBS Chromium Plugin Build Script
echo ========================================
echo.

REM Check if CMake is available
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake and add it to your PATH
    pause
    exit /b 1
)

REM Set default paths (modify these as needed)
set "OBS_STUDIO_DIR=C:\obs-studio"
set "QT_DIR=C:\Qt\6.5.0\msvc2019_64"
set "CEF_DIR=%~dp0cef"
set "BUILD_DIR=%~dp0build"
set "BUILD_TYPE=Release"

REM Check for command line arguments
if "%1"=="debug" set "BUILD_TYPE=Debug"
if "%1"=="clean" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo Build directory cleaned.
    goto :eof
)

echo Build Configuration:
echo - Build Type: %BUILD_TYPE%
echo - OBS Studio: %OBS_STUDIO_DIR%
echo - Qt Directory: %QT_DIR%
echo - CEF Directory: %CEF_DIR%
echo - Build Directory: %BUILD_DIR%
echo.

REM Verify required directories exist
if not exist "%OBS_STUDIO_DIR%" (
    echo WARNING: OBS Studio directory not found: %OBS_STUDIO_DIR%
    echo Please set OBS_STUDIO_DIR to the correct path
)

if not exist "%QT_DIR%" (
    echo WARNING: Qt directory not found: %QT_DIR%
    echo Please set QT_DIR to the correct path
)

if not exist "%CEF_DIR%" (
    echo ERROR: CEF directory not found: %CEF_DIR%
    echo Please download CEF binary distribution and extract to: %CEF_DIR%
    echo Download from: https://cef-builds.spotifycdn.com/index.html
    pause
    exit /b 1
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Change to build directory
cd /d "%BUILD_DIR%"

echo Configuring CMake...
echo.

REM Configure CMake
cmake .. ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCEF_ROOT="%CEF_DIR%" ^
    -DQt6_DIR="%QT_DIR%\lib\cmake\Qt6" ^
    -Dlibobs_DIR="%OBS_STUDIO_DIR%\build\libobs" ^
    -DCMAKE_INSTALL_PREFIX="%PROGRAMFILES%\obs-studio"

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo.
echo Building plugin...
echo.

REM Build the project
cmake --build . --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Plugin built in: %BUILD_DIR%\%BUILD_TYPE%
echo.
echo To install the plugin:
echo 1. Run: cmake --install . --config %BUILD_TYPE%
echo 2. Or manually copy files to OBS plugins directory
echo.
echo Would you like to install the plugin now? (y/n)
set /p "install_choice=Choice: "

if /i "%install_choice%"=="y" (
    echo Installing plugin...
    cmake --install . --config %BUILD_TYPE%
    if errorlevel 1 (
        echo ERROR: Installation failed
        echo You may need to run as administrator
        pause
        exit /b 1
    )
    echo Plugin installed successfully!
    echo Restart OBS Studio to use the new plugin.
) else (
    echo Installation skipped.
    echo To install later, run: cmake --install . --config %BUILD_TYPE%
)

echo.
echo Build script completed.
pause