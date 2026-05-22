# 🤖 Hexapod Pet Robot — Project Status Report

> **Ngày cập nhật:** 22/05/2026  
> **Phiên bản firmware:** xiaozhi_vn v2.0.5.06  
> **ESP-IDF:** v5.5.4  
> **Target:** ESP32-S3 N16R8 (cả 2 board)

---

## 📋 Tổng quan kiến trúc

```
┌─────────────────────────┐         ┌─────────────────────────┐
│   VoiceBot (COM4)       │◄──UART──►│  Hexapod Bot (COM10)    │
│   MASTER                 │ 921600   │  SLAVE                  │
│                          │         │                         │
│   • Audio I/O (I2S)     │         │  • 18 Servos (PCA9685)  │
│   • TFT ST7789 240x240   │         │  • Camera OV5640        │
│   • MCP Client (AI)     │         │  • TFT ST7735 80x160    │
│   • Wake Word + TTS     │         │  • Motion Controller    │
│   • Buttons x3          │         │                         │
└─────────────────────────┘         └─────────────────────────┘
         │                                    │
         └────────── Wi-Fi Router ────────────┘
```

---

## ✅ Những gì đã thay đổi

### Lần cập nhật gần nhất (22/05/2026)

| Board | Thay đổi | Trạng thái |
|-------|----------|------------|
| **VoiceBot (COM4)** | Build + Flash thành công | ✅ Boot OK, màn hình hiển thị |
| **Hexapod Bot (COM10)** | Thay EmoteDisplay → SpiLcdDisplay | ⚠️ Build OK, cần fix ST7735 init |
| **hexapod_bot_board.cc** | Khởi tạo display dùng SpiLcdDisplay (LVGL) | Cần khôi phục init ST7735 thủ công |
| **config.json (bot)** | Xóa `emotion_display`, thêm `lcd_display` | ✅ |

### Lịch sử thay đổi

1. **07/05** — Khởi tạo project, tạo cấu trúc 2 board
2. **08/05** — Hoàn thiện servo controller, gait generator, attack patterns
3. **13/05** — Thêm UART bridge, WebSocket server, protocol
4. **18/05** — Sửa audio codec (8 tham số NoAudioCodecSimplex)
5. **19/05** — Sửa UART bridge, tạo flash scripts
6. **20/05** — Sửa display Bot: EmoteDisplay → SpiLcdDisplay
7. **21/05** — Bổ sung hexapod_motion.h vào bot board
8. **22/05** — Build + Flash VoiceBot thành công

---

## 📁 Cấu trúc source code

### Board configurations (3 boards)

```
main/boards/
├── hexapod_voicebot/          ← VoiceBot (COM4) — MASTER
│   ├── config.h               ← GPIO: Audio I2S, TFT SPI, Buttons
│   ├── config.json            ← Board metadata
│   └── hexapod_voicebot_board.cc  ← SPI LCD + Audio + Buttons + UART MASTER
│
├── hexapod_bot/               ← Hexapod Bot (COM10) — SLAVE
│   ├── config.h               ← GPIO: Servo I2C, Camera DVP, TFT SPI
│   ├── config.json            ← Board metadata
│   └── hexapod_bot_board.cc   ← Servo + Motion + Display + Camera + UART SLAVE
│
└── hexapod-dual/              ← Tất cả trong 1 board (thử nghiệm)
    └── hexapod_dual_board.cc  ← TFT + Emotion + Servo + Camera + Audio
```

### Core hexapod modules (23 files)

| File | Mô tả | Dùng bởi |
|------|--------|----------|
| `hexapod_servo_controller.h/.cc` | Driver PCA9685 (2x, 18 servos) | Bot, Dual |
| `hexapod_gait_generator.h/.cc` | Biological gait engine (tripod/ripple/wave) | Bot, Dual |
| `hexapod_attack_patterns.h/.cc` | Strike/lunge animations | Bot, Dual |
| `hexapod_motion.h/.cc` | High-level motion API (walk/jump/dance) | Bot, Dual |
| `hexapod_uart_bridge.h/.cc` | UART JSON protocol Master↔Slave | Cả 2 boards |
| `hexapod_uart_link.h/.cc` | Low-level UART framing layer | Bridge dùng |
| `hexapod_mcp_tools.h/.cc` | MCP tools cho AI (move/camera/emotion/status) | VoiceBot |
| `hexapod_server.h/.cc` | WebSocket server (port 8081) | Bot (legacy) |
| `hexapod_protocol.h/.cc` | WebSocket protocol handlers | Bot (legacy) |
| `hexapod_emotion_display.h/.cc` | OLED emotion rendering (stub/TODO) | Chưa hoàn thiện |
| `hexapod_status_display.h/.cc` | Status HUD renderer | Bot |

---

## 🎮 MCP Tools (AI Voice Commands)

VoiceBot đăng ký các MCP tools cho AI điều khiển robot:

| Tool | Mô tả | Ví dụ lệnh giọng nói |
|------|--------|----------------------|
| `hexapod.move` | Di chuyển (forward/backward/left/right/stop/jump/dance/sit/stand) | "đi tới", "nhảy", "dance" |
| `hexapod.camera.capture` | Chụp ảnh từ camera OV5640 | "chụp ảnh" |
| `hexapod.camera.stream` | Stream video | "xem camera" |
| `hexapod.emotion` | Đặt biểu cảm (happy/sad/angry/neutral/...) | "happy" |
| `hexapod.status` | Lấy trạng thái robot | "tình trạng" |

---

## 🔧 Hướng dẫn Build & Flash

### Scripts có sẵn

| Script | Mục đích |
|--------|----------|
| `flash_voicebot_com4.ps1` | Chọn board + build + flash COM4 (VoiceBot) |
| `flash_hexapod_com10.ps1` | Chọn board + build + flash COM10 (Bot) |
| `monitor_hexapod_com10.bat` | Monitor serial COM10 |
| `xiaozhi-esp32_vietnam_new\select_board.bat` | Chuyển board type (voicebot/hexapod) |

### Build thủ công

```powershell
cd D:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new

# === VoiceBot (COM4) ===
.\select_board.bat voicebot
# Clean build nếu cần:
#   rmdir /S /Q build && del sdkconfig
idf.py set-target esp32s3
idf.py build
idf.py -p COM4 -b 921600 flash

# === Hexapod Bot (COM10) ===
.\select_board.bat hexapod
idf.py set-target esp32s3
idf.py build
idf.py -p COM10 -b 921600 flash
```

### Monitor

```powershell
# VoiceBot
idf.py -p COM4 monitor

# Hexapod Bot
idf.py -p COM10 monitor

# Thoát monitor: Ctrl+]
```

---

## 🌐 Truy cập & Cấu hình

### Wi-Fi Configuration
1. Lần đầu boot, board tạo AP (Access Point)
2. Kết nối vào mạng AP của board
3. Truy cập `http://192.168.4.1` để cấu hình Wi-Fi
4. Nhập SSID + Password → board tự kết nối

### UART Bridge (Master ↔ Slave)
```
VoiceBot GPIO43 (TX) ──── HexapodBot GPIO44 (RX)
VoiceBot GPIO44 (RX) ◄──── HexapodBot GPIO43 (TX)
GND ──────────────────────────── GND
Baud rate: 921600
```

### Protocol JSON qua UART

**VoiceBot → Bot (Command):**
```json
{"cmd":"motion", "action":"forward", "speed":50, "duration":2000}
{"cmd":"motion", "action":"jump", "intensity":80}
{"cmd":"motion", "action":"dance", "intensity":60}
{"cmd":"emotion", "name":"happy"}
{"cmd":"camera", "action":"capture"}
{"cmd":"status"}
```

**Bot → VoiceBot (Telemetry):**
```json
{"status":"ok", "motion":"walking", "battery":85, "servos":18}
```

---

## ⚠️ Vấn đề cần khắc phục

### 🔴 Critical (Board không hoạt động)

| # | Vấn đề | Board | Mô tả | Giải pháp đề xuất |
|---|--------|-------|--------|-------------------|
| 1 | **ST7735 display treo** | COM10 (Bot) | `esp_lcd_new_panel_st7789` gửi lệnh ST7789 vào chip ST7735 → crash | Khôi phục init commands ST7735S thủ công (như bản gốc) nhưng vẫn dùng SpiLcdDisplay. Cần gửi chuỗi SWRESET→SLPOUT→FRMCTR→COLMOD→DISPON trước khi gọi `esp_lcd_panel_init()` |

### 🟡 Medium (Hoạt động nhưng chưa hoàn thiện)

| # | Vấn đề | File | Mô tả |
|---|--------|------|--------|
| 2 | **Emotion Display** | `hexapod_emotion_display.cc` | Chỉ là stub (TODO), không render thực tế. Các hàm ShowText/Clear/AnimateEmotion chỉ log, không vẽ |
| 3 | **Camera capture** | `hexapod_server.cc:372` | TODO: chưa implement camera capture/stream qua WebSocket |
| 4 | **Legacy WebSocket server** | `hexapod_server.cc`, `hexapod_protocol.cc` | Đã thay bằng UART bridge, code cũ vẫn compile nhưng có thể không dùng |
| 5 | **Firmware cũ trong /firmware** | `firmware/` | File firmware backup cũ, nên update sau khi fix |

### 🟢 Low (Nice to have)

| # | Vấn đề | Mô tả |
|---|--------|--------|
| 6 | **Power save mode** | Cả 2 board đều có TODO cho power save |
| 7 | **hexapod-dual board** | Board tổng hợp 1 mạch, dùng EmoteDisplay cũ (cần update) |
| 8 | **Status display** | `hexapod_status_display.cc` — có renderer nhưng không được gọi từ bot board |
| 9 | **Audio output rate** | VoiceBot đang dùng 16kHz in/out — nên nâng output lên 24kHz cho TTS tốt hơn |

---

## 📊 GPIO Allocation Quick Reference

### VoiceBot (COM4)
| Chức năng | GPIO |
|-----------|------|
| UART TX/RX → Bot | 43 / 44 |
| TFT ST7789 SPI | MOSI=11, CLK=12, CS=9, DC=8, RST=18, BL=46 |
| MIC I2S (INMP441) | WS=4, SCK=5, DIN=6 |
| SPK I2S (MAX98357A) | DOUT=7, BCLK=15, LRCK=16 |
| Buttons | Boot=0, Vol+=3, Vol-=17 |
| LED | 48 |

### Hexapod Bot (COM10)
| Chức năng | GPIO |
|-----------|------|
| UART TX/RX ← VoiceBot | 43 / 44 |
| Servo I2C (PCA9685) | SDA=41, SCL=42 |
| Camera I2C (OV5640) | SDA=39, SCL=40 |
| Camera DVP | D0-D7: 37,38,19,20,22,23,24,25 |
| Camera Ctrl | VSYNC=34, HREF=35, PCLK=36, PWDN=47, RST=21 |
| TFT ST7735 SPI | SCL=1, SDA=2, RES=11, DC=10, CS=3, BLK=14 |
| LED | 48 |

---

## 🚀 Roadmap gợi ý

### Phase 1: Fix Critical (Ưu tiên cao)
- [ ] Sửa ST7735 init trên COM10 — khôi phục manual init commands
- [ ] Test UART bridge 2 boards giao tiếp thành công
- [ ] Verify MCP tools hoạt động qua lệnh giọng nói

### Phase 2: Hoàn thiện
- [ ] Implement emotion display thực tế trên ST7735
- [ ] Implement camera capture/stream
- [ ] Dọn dẹp legacy code (hexapod_server/protocol nếu không dùng)
- [ ] Update firmware backup trong /firmware/

### Phase 3: Nâng cấp
- [ ] Nâng audio output lên 24kHz
- [ ] Power save mode
- [ ] Calibrate servo offset cho từng chân
- [ ] Thêm sensor (battery voltage, IMU)
