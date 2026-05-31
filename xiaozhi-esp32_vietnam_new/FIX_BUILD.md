# Sua loi Build - ESP-IDF / ESP32-S3

## Quy trinh chuan
### VoiceBot (COM4)
```cmd
cd D:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new
. D:\Espressif\frameworks\esp-idf-v5.5.4\export.ps1
.\select_board.bat voicebot
idf.py set-target esp32s3
idf.py build
idf.py -p COM4 flash monitor
```

### Hexapod Bot (COM10)
```cmd
cd D:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new
. D:\Espressif\frameworks\esp-idf-v5.5.4\export.ps1
.\select_board.bat hexapod
idf.py set-target esp32s3
idf.py build
idf.py -p COM10 flash monitor
```

## Build lai sach khi can
```cmd
cd D:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new
.\select_board.bat hexapod   # hoac voicebot
idf.py fullclean
idf.py set-target esp32s3
idf.py build
```

