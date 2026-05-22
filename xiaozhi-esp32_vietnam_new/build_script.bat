@echo off
call "D:\Espressif\frameworks\esp-idf-v5.5.4\export.bat"

cd /d D:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new

echo ======================================
echo SETTING TARGET TO ESP32-S3
echo ======================================
idf.py set-target esp32s3

echo ======================================
echo BUILDING AND FLASHING ESP-IDF PROJECT
echo ======================================

idf.py -p COM10 -b 921600 build flash 2>&1

echo ======================================
echo BUILD EXIT CODE: %ERRORLEVEL%
echo ======================================
