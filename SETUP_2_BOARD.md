# Hexapod Robot - 2 Board Setup Guide

## Hardware Setup

### Board Assignment

| Board | Port | Role | Features |
|-------|------|------|----------|
| **ESP32-S3 (no cam)** | COM4 | VoiceBot (Primary) | WiFi, Audio, Display, MCP Client |
| **ESP32-S3 + Cam 5MP** | COM10 | Hexapod Bot | Servos (18x), Camera OV5640, Motion Control |

### UART Connection

Connect the UART bridge between the two boards:

```
┌─────────────────────────┐         ┌─────────────────────────┐
│   VoiceBot (COM4)       │         │   Hexapod Bot (COM10)   │
│   ESP32-S3              │         │   ESP32-S3              │
├─────────────────────────┤         ├─────────────────────────┤
│                         │         │                         │
│  GPIO 43 (UART TX) ────┼────────►│  GPIO 44 (UART RX)       │
│  GPIO 44 (UART RX) ◄───┼─────────┼── GPIO 43 (UART TX)      │
│                         │         │                         │
│  GND ───────────────────┼─────────┼── GND                   │
│                         │         │                         │
└─────────────────────────┘         └─────────────────────────┘
```

**Note:** Make sure to connect GND between both boards for common reference!

## Software Configuration

### VoiceBot Board (COM4)

**Features:**
- ✅ WiFi connection
- ✅ WebSocket/MCP client
- ✅ Audio I/O (INMP441 mic + MAX98357A amp) - Fixed simplex I2S
- ✅ TFT Display 240x240 (ST7789)
- ✅ Buttons (boot, volume up/down)
- ✅ UART Bridge as MASTER
- ❌ No camera (uses camera from Bot via UART)
- ❌ No servo control (sends commands via UART)

**UART Bridge:**
- Starts automatically on boot as MASTER role
- Sends motion/emotion commands via UART at 921600 baud
- Receives telemetry from Bot

### Hexapod Bot Board (COM10)

**Features:**
- ✅ 18 Servos via 2x PCA9685 (I2C @ GPIO 41/42)
- ✅ Camera OV5640 (5MP, DVP interface) - GPIO 39-40, 19-25, 34-38, 47
- ✅ Motion control (GaitGenerator, AttackPatterns)
- ✅ Emotion Display 80x160 (ST7735) - Landscape 160x80
- ✅ UART Bridge as SLAVE
- ❌ No audio (audio is on VoiceBot)

**UART Bridge:**
- Starts automatically on boot as SLAVE role
- Listens for commands from VoiceBot
- Executes motion, gait, pose, emotion commands
- Sends telemetry back to VoiceBot

## Flashing Instructions

### 1. Flash VoiceBot (COM4)

Use the board selection script:
```bash
cd xiaozhi-esp32_vietnam_new
.\select_board.bat voicebot
idf.py -p COM4 flash monitor
```

Or manually:
```bash
cd xiaozhi-esp32_vietnam_new
idf.py set_target BOARD_TYPE_HEXAPOD_VOICEBOT
idf.py -p COM4 flash monitor
```

### 2. Flash Hexapod Bot (COM10)

Use the board selection script:
```bash
cd xiaozhi-esp32_vietnam_new
.\select_board.bat hexapod
idf.py -p COM10 flash monitor
```

Or manually:
```bash
cd xiaozhi-esp32_vietnam_new
idf.py set_target BOARD_TYPE_HEXAPOD_BOT
idf.py -p COM10 flash monitor
```

## Protocol Overview

### UART Messages (JSON)

**Commands (VoiceBOT → Bot):**
```json
{"cmd": "motion", "action": "forward", "speed": 50, "duration_ms": 1000}
{"cmd": "motion", "action": "stop"}
{"cmd": "emotion", "emotion": "happy", "text": "Hello!"}
{"cmd": "camera", "action": "snapshot"}
```

**Telemetry (Bot → VoiceBOT):**
```json
{"link": "uart", "motion": "walking", "servos_active": true, "camera_ready": true}
```

## Expected Behavior

1. **Boot Sequence:**
   - VoiceBot boots, connects to WiFi
   - Hexapod Bot boots, initializes servos and camera
   - UART bridge establishes connection (921600 baud)
   - VoiceBot sends status request to Bot

2. **Voice Control:**
   - User speaks wake word on VoiceBot
   - VoiceBot processes via MCP/AI server
   - VoiceBot sends motion commands to Bot via UART
   - Bot executes motion and sends telemetry back

3. **Camera Access:**
   - VoiceBot requests camera snapshot via UART
   - Bot captures image from OV5640
   - Bot sends image data back via UART

## Troubleshooting

### Audio Issues (No Sound Output)

**Symptoms:** VoiceBot doesn't produce sound

**Fixes:**
1. Verify audio codec uses 8-parameter NoAudioCodecSimplex
2. Check I2S pins: MIC (GPIO 4/5/6), SPK (GPIO 7/15/16)
3. Ensure INMP441 and MAX98357A are properly connected
4. Check sample rates: Input 16kHz, Output 24kHz

### UART Connection Issues

**Symptoms:** VoiceBot doesn't see Bot, no telemetry received

**Checks:**
1. Verify UART wiring (TX→RX, RX→TX, GND→GND)
2. Check GPIO 43/44 on both boards
3. Verify baud rate: 921600
4. Check UART bridge logs for "MASTER" and "SLAVE" roles

### Servo Issues

**Symptoms:** Servos don't move, jittering

**Checks:**
1. Verify PCA9685 addresses: 0x40, 0x41
2. Check I2C bus (GPIO 41 SDA, GPIO 42 SCL)
3. Ensure external power for servos (5V, sufficient current)

### Camera Issues

**Symptoms:** Camera not detected, no image

**Checks:**
1. Verify OV5640 connections (DVP interface)
2. Check I2C for camera (GPIO 39 SDA, GPIO 40 SCL)
3. Verify camera power supply

## GPIO Reference

### VoiceBot (COM4)

| Function | GPIO |
|----------|------|
| UART TX | 43 |
| UART RX | 44 |
| TFT MOSI | 11 |
| TFT SCK | 12 |
| TFT DC | 8 |
| TFT CS | 9 |
| TFT RST | 18 |
| TFT BL | 46 |
| Audio MIC WS | 4 |
| Audio MIC SCK | 5 |
| Audio MIC DIN | 6 |
| Audio SPK DOUT | 7 |
| Audio SPK BCLK | 15 |
| Audio SPK LRCK | 16 |
| Boot Button | 0 |
| Volume Up | 3 |
| Volume Down | 17 |
| Status LED | 48 |

### Hexapod Bot (COM10)

| Function | GPIO |
|----------|------|
| UART TX | 43 |
| UART RX | 44 |
| Servo I2C SDA | 41 |
| Servo I2C SCL | 42 |
| Camera I2C SDA | 39 |
| Camera I2C SCL | 40 |
| Camera VSYNC | 34 |
| Camera HREF | 35 |
| Camera PCLK | 36 |
| Camera D0-D7 | 37, 38, 19, 20, 22, 23, 24, 25 |
| Camera PWDN | 47 |
| Camera RESET | 21 |
| Emotion Display SCL | 1 |
| Emotion Display SDA | 2 |
| Emotion Display RES | 11 |
| Emotion Display DC | 10 |
| Emotion Display CS | 3 |
| Emotion Display BLK | 14 |
| Status LED | 48 |

## Architecture Summary

```
┌─────────────────────────────────────────────────────────────────────┐
│                        VoiceBot (COM4)                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐     │
│  │    WiFi     │  │    Audio    │  │    TFT Display 240x240  │     │
│  │  + MCP      │  │ INMP441+    │  │    (ST7789)            │     │
│  │  Client     │  │ MAX98357A   │  │                         │     │
│  └──────┬──────┘  └─────────────┘  └─────────────────────────┘     │
│         │                                                           │
│         │ UART (921600) ────────────────────┐                       │
│         ▼                                   ▼                       │
│  ┌─────────────────────────────────────────────────────────┐       │
│  │                   UART Bridge (MASTER)                   │       │
│  └─────────────────────────────────────────────────────────┘       │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              │ UART Bridge
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       Hexapod Bot (COM10)                            │
│  ┌──────────────────────┐  ┌─────────────────────────────────────┐  │
│  │  Servo Controller    │  │           Camera OV5640             │  │
│  │  18 Servos (PCA9685) │  │          (5MP DVP)                  │  │
│  │  I2C @ 0x40, 0x41    │  │                                     │  │
│  └──────────────────────┘  └─────────────────────────────────────┘  │
│         ▲                                   ▲                        │
│         │                                   │                        │
│  ┌──────┴───────────────────────────────────┴──────┐                │
│  │                UART Bridge (SLAVE)               │                │
│  │  - Receives motion/emotion commands              │                │
│  │  - Sends telemetry back                          │                │
│  └──────────────────────────────────────────────────┘                │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │       Emotion Display 80x160 (ST7735 Landscape 160x80)      │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
```