@echo off
REM Build & Flash Hexapod Bot (COM10) - ESP32-S3
echo ========================================================
echo Build and Flash Hexapod Bot to COM10
echo ========================================================
echo.

cd /d "%~dp0"

REM Chọn board type
echo [Step 1] Selecting board type: Hexapod Bot...
call select_board.bat hexapod
if errorlevel 1 (
    echo ERROR: Failed to select Hexapod Bot board
    pause
    exit /b 1
)
echo.

REM Build
echo [Step 2] Building project for ESP32-S3...
idf.py set-target esp32s3
if errorlevel 1 (
    echo ERROR: Failed to set target to esp32s3
    pause
    exit /b 1
)

idf.py build
if errorlevel 1 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)
echo.
echo Build complete!
echo.

REM Flash
echo [Step 3] Flashing to COM10...
echo.

idf.py -p COM10 --baud 921600 flash
if errorlevel 1 (
    echo.
    echo ERROR: Flash failed!
    echo.
    echo Check:
    echo   1. COM10 is connected and powered
    echo   2. Driver installed (CP210x or CH343)
    echo   3. No UART wires connected to VoiceBot yet
    pause
    exit /b 1
)

echo.
echo ========================================================
echo Flash Hexapod Bot COMPLETE!
echo ========================================================
echo.
echo Next steps:
echo   1. Test Bot: idf.py -p COM10 monitor
echo   2. Verify both boards work independently
echo   3. ONLY THEN connect UART wires between boards:
echo      - COM4 GPIO43 (TX) to COM10 GPIO44 (RX)
echo      - COM4 GPIO44 (RX) to COM10 GPIO43 (TX)
echo      - COM4 GND to COM10 GND
echo.
pause