# Huong dan Build va Flash Firmware (ESP-IDF)

## Hexapod Bot (COM10)
```powershell
cd "d:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new"
& "D:\Espressif\frameworks\esp-idf-v5.5.4\export.ps1"
.\select_board.ps1 hexapod
idf.py set-target esp32s3
idf.py build
idf.py -p COM10 -b 921600 flash monitor
```

## VoiceBot (COM4)
```powershell
cd "d:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new"
& "D:\Espressif\frameworks\esp-idf-v5.5.4\export.ps1"
.\select_board.ps1 voicebot
idf.py set-target esp32s3
idf.py build
idf.py -p COM4 -b 921600 flash monitor
```

