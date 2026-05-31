# Hexapod Pet - Robot 6 Chân Điều Khiển ESP32-S3

Fork từ [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) và [xiaozhi-esp32_vietnam](https://github.com/TienHuyIoT/xiaozhi-esp32_vietnam), tập trung vào điều khiển robot hexapod 6 chân với camera stream qua WebSocket.

---

## Tính Năng

### Điều Khiển Robot
- **18 servo MG90S** qua 2 PCA9685 (I2C)
- **Joystick Web**: kéo xa → tốc độ cao (0-100%), giữ → chạy liên tục
- **Nút bấm**: Stand, Dance, Jump, Sit, Strike, Lunge
- **Gait**: tripod, ripple, wave, bi-gait
- **Lunge timer**: tự động về stand sau 1s (dùng FreeRTOS timer)

### Camera
- **OV5640** 5MP, stream JPEG 320x240 ~12fps
- WebSocket binary frames tới browser
- Tự động phát hiện native JPEG (validate magic bytes 0xFFD8)
- Fallback software encode nếu cần

### Web UI
- HTTP server port 8081 (tránh OTA port 80)
- WebSocket control channel: phản hồi tức thì
- WebSocket camera stream
- Giao diện responsive cho mobile

### Kết Nối VoiceBot (COM4)
- UART bridge 921600 baud (GPIO 43 TX, 44 RX)
- MCP tools: `hexapod.move`, `hexapod.stream`, `hexapod.emotion`, `hexapod.status`
- Board COM4 vẫn chạy chat AI + voice + loa bình thường

---

## Build & Flash

### Yêu Cầu
- ESP-IDF v5.5+
- Python 3.12
- Board ESP32-S3

### Các Bước

```powershell
cd xiaozhi-esp32_vietnam_new

# Chọn board
.\select_board.ps1 hexapod

# Build
. D:\Espressif\frameworks\esp-idf-v5.5.4\export.ps1
idf.py set-target esp32s3
idf.py build

# Flash (COM10)
idf.py -p COM10 flash

# Monitor
idf.py -p COM10 monitor
```

Sau khi boot, mở browser tại `http://<IP>:8081/`

---

## Cấu Hình

Board type và hardware config được định nghĩa trong:
- `main/boards/hexapod_bot/config.h` — GPIO, servo layout
- `main/boards/hexapod_bot/hexapod_bot_board.cc` — init sequence
- `main/hexapod_constants.h` — tham số motion, camera, gait

### Timer Stack
Lunge dùng FreeRTOS timer callback. Nếu gặp stack overflow ở task `Tmr Svc`, tăng trong menuconfig:
```
Component config → FreeRTOS → Kernel → Timer task stack size
```
Giá trị hiện tại: **4096** (mặc định 2048).

---

## Files Chính

| File | Mục đích |
|------|----------|
| `hexapod_server.cc` | HTTP server, WebSocket handlers, camera stream task, Web UI |
| `hexapod_motion.cc` | Các motion sequences (walk, jump, dance, lunge, ...) |
| `hexapod_gait_generator.cc` | Sinh gait pattern (tripod, ripple, wave, bi-gait) |
| `hexapod_servo_controller.cc` | PCA9685 driver cho 18 servos |
| `hexapod_mcp_tools.cc` | MCP tools giao tiếp với VoiceBot |
| `hexapod_uart_bridge.cc` | UART bridge UART→JSON→motion |
| `mcp_server.cc` | MCP server cho AI chat (vision tools disabled) |

---

## Liên Hệ & Cộng Đồng

- **Zalo**: [Nhóm hỗ trợ](https://zalo.me/g/qlvffa015)
- **Facebook**: [Xiaozhi AI-IoT Vietnam](https://www.facebook.com/XiaozhiAI.IoTVietnam/)
- **GitHub Issues**: [Tạo issue mới](https://github.com/datyuno1409/Hexapod_pet/issues)

---

## Giấy Phép

MIT License, kế thừa từ [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32).
