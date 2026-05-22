@echo off
REM Flash VoiceBot (COM4) - ESP32-S3 với manual BOOT support
echo ========================================================
echo Flash VoiceBot Firmware to COM4
echo ========================================================
echo.

cd /d "%~dp0"

REM Chọn board type
echo [Step 1] Selecting board type: VoiceBot...
call select_board.bat voicebot
if errorlevel 1 (
    echo ERROR: Failed to select VoiceBot board
    pause
    exit /b 1
)
echo.

REM Build
echo [Step 2] Building project...
idf.py build
if errorlevel 1 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)
echo Build complete!
echo.

REM Flash
echo [Step 3] Flashing to COM4...
echo.
echo If flash fails with "TX path seems to be down":
echo   1. Press and HOLD the BOOT button on your board
echo   2. Run this script again
echo   3. Release BOOT after seeing "Connecting..."
echo.
echo Press any key to start flashing...
pause >nul
echo.

echo Starting flash...
idf.py -p COM4 --baud 921600 flash
if errorlevel 1 (
    echo.
    echo ERROR: Flash failed!
    echo.
    echo Try MANUAL BOOT procedure:
    echo   1. Press and HOLD BOOT button on board
    echo   2. Run: idf.py -p COM4 flash
    echo   3. Release BOOT after "Connecting..."
    pause
    exit /b 1
)

echo.
echo ========================================================
echo Flash VoiceBot COMPLETE!
echo ========================================================
echo.
echo Next steps:
echo   1. Test VoiceBot: idf.py -p COM4 monitor
echo   2. When ready, flash Hexapod Bot to COM10
echo   3. ONLY THEN connect UART between boards
echo.
pause