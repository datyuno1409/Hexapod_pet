@echo off
REM UART Bridge Connection Guide - Hexapod Robot
echo ========================================================
echo UART Bridge Connection Checklist
echo ========================================================
echo.
echo PRECONDITION: BOTH boards must be flashed FIRST!
echo   - VoiceBot flashed to COM4
echo   - Hexapod Bot flashed to COM10
echo.
echo If you flash AFTER connecting UART, you may get errors!
echo ========================================================
echo.
pause

echo.
echo UART CONNECTION DIAGRAM:
echo ========================================================
echo.
echo   VoiceBot (COM4)        Wire        Hexapod Bot (COM10)
echo   ====================             =====================
echo
echo   GPIO 43 (TX)    ----------------->  GPIO 44 (RX)
echo
echo   GPIO 44 (RX)    <------------------  GPIO 43 (TX)
echo
echo   GND             ----------------->  GND
echo
echo ========================================================
echo.
echo WARNING:
echo   - DO NOT connect 5V/VCC between boards!
echo   - Each board gets its own USB power
echo   - Only connect TX, RX, and GND
echo   - Make sure power is OFF before connecting
echo.
pause

echo.
echo CONNECTION STEPS:
echo ========================================================
echo 1. Disconnect power from BOTH boards
echo 2. Connect GND first (common ground)
echo 3. Connect GPIO43 (COM4 TX) to GPIO44 (COM10 RX)
echo 4. Connect GPIO44 (COM4 RX) to GPIO43 (COM10 TX)
echo 5. Double-check connections
echo 6. Power on BOTH boards
echo 7. Monitor both terminals to verify UART bridge
echo ========================================================
echo.
pause

echo.
echo VERIFICATION:
echo ========================================================
echo Open TWO terminal windows:
echo.
echo Terminal 1:
echo   idf.py -p COM4 monitor
echo   Expected: "UART bridge initialized as MASTER"
echo.
echo Terminal 2:
echo   idf.py -p COM10 monitor
echo   Expected: "UART bridge initialized as SLAVE"
echo.
echo If you see ping/telemetry messages, UART bridge is working!
echo ========================================================
echo.
pause