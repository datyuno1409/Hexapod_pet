# Hexapod Robot - Hướng dẫn khởi động nhanh

## Tổng quan kiến trúc 2-board

```
┌─────────────────────┐         ┌─────────────────────┐
│   VoiceBot (COM4)   │◄──UART──┤  Hexapod Bot (COM10)│
│   - Audio I/O       │ 921600  │  - 18 Servos        │
│   - TFT Display     │         │  - Camera OV5640    │
│   - MCP Client      │         │  - Motion Control   │
│   - MASTER role     │         │  - SLAVE role       │
└─────────────────────┘         └─────────────────────┘
```

## Kết nối phần cứng

```
VoiceBot (COM4)         Hexapod Bot (COM10)
GPIO 43 (TX)    ──────►  GPIO 44 (RX)
GPIO 44 (RX)    ◄──────  GPIO 43 (TX)
GND           ─────────  GND
```

## Các bước setup

### 1. Build cho VoiceBot (COM4)

```bash
cd xiaozhi-esp32_vietnam_new

# Chọn board type
.\select_board.bat voicebot

# Build và flash
idf.py -p COM4 build flash monitor
```

### 2. Build cho Hexapod Bot (COM10)

```bash
cd xiaozhi-esp32_vietnam_new

# Chọn board type
.\select_board.bat hexapod

# Build và flash
idf.py -p COM10 build flash monitor
```

## Kiểm tra hoạt động

### VoiceBot (COM4)
Khi boot thành công, bạn sẽ thấy log:
```
I (XXXX) HexapodVoicebotBoard: UART bridge initialized as MASTER for Hexapod Bot communication
I (XXXX) HexapodVoicebotBoard: Hexapod VoiceBot board initialized
```

### Hexapod Bot (COM10)
Khi boot thành công, bạn sẽ thấy log:
```
I (XXXX) HexapodBotBoard: UART bridge initialized as SLAVE for VoiceBot communication
I (XXXX) HexapodBotBoard: Hexapod Bot board initialized successfully
```

## Kết nối UART Bridge

Sau khi cả 2 board boot xong, UART bridge sẽ tự động kết nối:
```
I (XXXX) HexapodUartBridge: UART bridge started as MASTER (GPIO TX=43 RX=44 baud=921600)
I (XXXX) HexapodUartBridge: UART bridge started as SLAVE (GPIO TX=43 RX=44 baud=921600)
```

## Fix audio không phát ra tiếng

Nếu VoiceBot không phát ra âm thanh, kiểm tra:
1. Audio codec đang dùng `NoAudioCodecSimplex` với 8 tham số
2. GPIO âm thanh: MIC (4/5/6), SPK (7/15/16)
3. Sample rate: Input 16kHz, Output 24kHz

## Switch giữa các board

Để chuyển đổi giữa VoiceBot và Bot:

```bash
# Chuyển sang VoiceBot
.\select_board.bat voicebot

# Chuyển sang Bot
.\select_board.bat hexapod
```

Script sẽ tự động:
- Cập nhật `sdkconfig.defaults.esp32s3`
- Xóa folder `build` cũ
- Xóa file `sdkconfig` cũ

## GPIO Reference

### VoiceBot (COM4)
| Chức năng | GPIO |
|-----------|------|
| UART TX/RX | 43/44 |
| TFT Display | 8, 9, 11, 12, 13, 18, 46 |
| Audio I2S | 4, 5, 6, 7, 15, 16 |
| Buttons | 0, 3, 17 |
| LED | 48 |

### Hexapod Bot (COM10)
| Chức năng | GPIO |
|-----------|------|
| UART TX/RX | 43/44 |
| Servo I2C | 41 (SDA), 42 (SCL) |
| Camera I2C | 39 (SDA), 40 (SCL) |
| Camera DVP | 19-25, 34-38, 47 |
| Emotion Display | 1, 2, 3, 10, 11, 14 |
| LED | 48 |