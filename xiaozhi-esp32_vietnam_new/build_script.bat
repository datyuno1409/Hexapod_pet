@echo off
setlocal

echo ========================================================
echo Building Hexapod Bot firmware (ESP-IDF)...
echo ========================================================

cd /d "%~dp0"
call select_board.bat hexapod
if errorlevel 1 exit /b 1
call D:\Espressif\frameworks\esp-idf-v5.5.4\export.bat
if errorlevel 1 exit /b 1
call idf.py set-target esp32s3
if errorlevel 1 exit /b 1
call idf.py build
if errorlevel 1 exit /b 1

echo ========================================================
echo Build complete for Hexapod Bot! Flashing to COM10...
echo ========================================================
call idf.py -p COM10 flash monitor
if errorlevel 1 exit /b 1

endlocal

