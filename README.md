# HEXAPOD PET - Robot 6 Chân Điều Khiển ESP32-S3

## Tổng Quan

Robot hexapod 6 chân sử dụng ESP32-S3, 18 servo MG90S, camera OV5640. Điều khiển qua web browser (Wi-Fi) với joystick tốc độ tỷ lệ và các nút bấm trực tiếp. Không cần board VoiceBot riêng — giao tiếp trực tiếp qua WebSocket.

### Tính Năng Chính
- ✅ Điều khiển 18 servo qua PCA9685 (I2C)
- ✅ Web UI: joystick + nút bấm (stand, dance, jump, sit, strike, lunge)
- ✅ Joystick: tốc độ tỷ lệ theo khoảng cách, di chuyển liên tục
- ✅ Camera OV5640 stream qua WebSocket (320x240, ~12fps)
- ✅ WebSocket control channel (phản hồi tức thì, không cần HTTP POST)
- ✅ HTTP server trên port 8081 (tránh xung đột OTA port 80)

### Board COM4 (VoiceBot)
- Board COM4 vẫn chạy chat AI, voice, loa bình thường
- Giao tiếp với hexapod qua UART bridge (921600 baud)
- Dùng MCP tools (`hexapod.move`, `hexapod.status`, ...)

---

## Cấu Trúc Hardware

### HexapodBot (Chính)
| Linh kiện | Model | SL | GPIO |
|-----------|-------|----|------|
| MCU | ESP32-S3 N16R8 | 1 | - |
| PWM Driver | PCA9685 | 2 | 21(SDA), 47(SCL) |
| Servo | MG90S | 18 | OUT0-17 |
| Camera | OV5640 5MP | 1 | DVP + I2C |
| Màn hình | ST7735 0.96" 80x160 | 1 | SPI |

### Servo Layout (6 chân × 3 DOF)
- Mỗi chân: Coxa (xoay hông), Femur (nâng đùi), Tibia (gập cẳng)
- PCA9685 #1 (0x40): 9 servos đầu
- PCA9685 #2 (0x41): 9 servos cuối

---

## Build & Flash

### Yêu Cầu
- ESP-IDF v5.5+
- Python 3.12+
- Board: ESP32-S3

### Các Bước

```powershell
cd xiaozhi-esp32_vietnam_new

# Chọn board hexapod
.\select_board.ps1 hexapod

# Build
. D:\Espressif\frameworks\esp-idf-v5.5.4\export.ps1
idf.py build

# Flash (COM10 cho hexapod)
idf.py -p COM10 flash monitor
```

### Kết Nối
Sau khi boot, mở browser vào `http://<IP>:8081/` để điều khiển.

---

## Web UI

| Tính năng | Mô tả |
|-----------|-------|
| Camera stream | WS binary JPEG frames, FPS counter |
| Joystick | Kéo xa → tốc độ cao (0-100%), giữ → chạy liên tục |
| Stand | Về vị trí đứng |
| Dance | Nhảy múa 3s |
| Jump | Bật nhảy |
| Sit | Ngồi xuống |
| Strike | Đòn tấn công |
| Lunge | Lao về phía trước (timer tự động về stand) |

---

## Sơ Đồ Kết Nối

```
Web Browser
    ↓ (Wi-Fi)
ESP32-S3 (HexapodBot :8081)
    ├── PCA9685 #1 (0x40) → 9 servos
    ├── PCA9685 #2 (0x41) → 9 servos
    ├── OV5640 Camera (DVP)
    ├── ST7735 Display (SPI)
    └── UART ←→ VoiceBot (COM4)
```

---

## Tham Khảo

- Firmware gốc: [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) bởi Xiage
- Phiên bản Việt Nam: [xiaozhi-esp32_vietnam](https://github.com/TienHuyIoT/xiaozhi-esp32_vietnam)
- ESP-IDF: [github.com/espressif/esp-idf](https://github.com/espressif/esp-idf)
