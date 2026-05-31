@echo off
REM Build script for Hexapod Bot (COM10) - ESP32-S3
echo ========================================================
echo Building Hexapod Bot firmware...
echo ========================================================

REM Call ESP-IDF export to set up environment
echo Setting up ESP-IDF environment...
call D:\Espressif\frameworks\esp-idf-v5.5.4\export.bat

REM Remove old build files
if exist "build" (
    echo Removing old build folder...
    rmdir /s /q "build"
)
if exist "sdkconfig" (
    echo Removing old sdkconfig...
    del /f /q "sdkconfig"
)

REM Set target to ESP32S3
echo Setting target to ESP32-S3...
idf.py set-target esp32s3

REM Build
echo Building project...
idf.py build

echo ========================================================
echo Build complete for Hexapod Bot!
echo Flash with: idf.py -p COM10 flash
echo ========================================================
pause