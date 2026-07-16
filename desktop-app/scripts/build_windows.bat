@echo off
setlocal enabledelayedexpansion

echo ==================================================
echo  CSI Motion Detector - FINAL Build Script
echo ==================================================
echo.

REM --- Move to the desktop-app root (this script lives in desktop-app\scripts) ---
cd /d "%~dp0\.."

REM --- Set paths ---
set QT_DIR=C:\Qt\6.11.1\mingw_64
set MINGW_DIR=C:\Qt\Tools\mingw1310_64\bin

REM --- Add to PATH ---
set PATH=%MINGW_DIR%;%PATH%

REM --- Verify CMake ---
echo Checking CMake...
cmake --version >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake not found in PATH!
    echo.
    echo If you installed CMake, restart Command Prompt.
    echo Or add it manually: set PATH=C:\Program Files\CMake\bin;%%PATH%%
    echo.
    pause
    exit /b 1
)
echo CMake is working!
echo.

REM --- Verify MinGW ---
echo Checking MinGW...
mingw32-make --version >nul 2>nul
if errorlevel 1 (
    echo [ERROR] MinGW not found in PATH!
    echo.
    echo Make sure MinGW path is correct: %MINGW_DIR%
    echo.
    pause
    exit /b 1
)
echo MinGW is working!
echo.

REM --- Build ---
set BUILD_DIR=build
set EXE_NAME=CsiMotionDetector.exe

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

echo Configuring with CMake...
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo.
    echo [ERROR] CMake configuration failed.
    echo.
    cd ..
    pause
    exit /b 1
)

echo.
echo Building...
mingw32-make -j4
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed.
    echo.
    cd ..
    pause
    exit /b 1
)

cd ..

set EXE_PATH=%BUILD_DIR%\%EXE_NAME%
if not exist "!EXE_PATH!" (
    echo [ERROR] Executable not found!
    pause
    exit /b 1
)

echo.
echo Deploying Qt DLLs...
"%QT_DIR%\bin\windeployqt.exe" "!EXE_PATH!"

echo.
echo ==================================================
echo  Launching CSI Motion Detector...
echo ==================================================
start "" "!EXE_PATH!"

echo.
echo Done! If the app doesn't start, run it manually from:
echo !EXE_PATH!

endlocal
pause