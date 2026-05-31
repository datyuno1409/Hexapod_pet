# Hexapod Pet - 6-Legged Robot Controlled by ESP32-S3

Forked from [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) and [xiaozhi-esp32_vietnam](https://github.com/TienHuyIoT/xiaozhi-esp32_vietnam), focused on hexapod robot control with WebSocket camera streaming.

---

## Features

### Robot Control
- **18x MG90S servos** via 2x PCA9685 (I2C)
- **Web joystick**: drag farther = higher speed (0-100%), continuous movement while holding
- **One-click actions**: Stand, Dance, Jump, Sit, Strike, Lunge
- **Gaits**: tripod, ripple, wave, bi-gait
- **Lunge timer**: auto-return to stand (FreeRTOS timer)

### Camera
- **OV5640** 5MP, JPEG stream 320x240 ~12fps
- WebSocket binary frames to browser
- Native JPEG detection (magic bytes 0xFFD8)
- Software encode fallback

### Web UI
- HTTP server on port 8081
- WebSocket control channel (instant response)
- WebSocket camera stream
- Responsive layout for mobile

### VoiceBot Connection (COM4)
- UART bridge 921600 baud (GPIO 43 TX, 44 RX)
- MCP tools: `hexapod.move`, `hexapod.stream`, `hexapod.emotion`, `hexapod.status`
- COM4 board retains full AI chat + voice + speaker

---

## Build & Flash

### Prerequisites
- ESP-IDF v5.5+
- Python 3.12
- Board: ESP32-S3

### Steps

```powershell
cd xiaozhi-esp32_vietnam_new

# Select hexapod board
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

Open browser at `http://<IP>:8081/` after boot.

---

## Key Files

| File | Purpose |
|------|---------|
| `hexapod_server.cc` | HTTP/WS server, camera stream, Web UI |
| `hexapod_motion.cc` | Motion sequences |
| `hexapod_gait_generator.cc` | Gait pattern generation |
| `hexapod_servo_controller.cc` | PCA9685 driver (18 servos) |
| `hexapod_mcp_tools.cc` | MCP tools for VoiceBot |
| `mcp_server.cc` | MCP server (vision tools disabled) |

---

## License

MIT License, inherited from [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32).
