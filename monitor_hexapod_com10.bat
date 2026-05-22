@echo off
echo ========================================================
echo HEXAPOD BOT - Monitor COM10
echo ========================================================
cd /d "D:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new"
call "D:\Espressif\frameworks\esp-idf-v5.5.4\export.bat" >nul 2>&1
idf.py -p COM10 monitor
