# 📌 HEXAPOD GPIO ALLOCATION - UPDATED WITH 2nd DISPLAY

## 🎯 VOICEBOT (ESP32-S3) - GPIO SUMMARY

```
GPIO Assignment:

Audio (I2S):
├─ GPIO 4  : INMP441 WS (microphone word select)
├─ GPIO 5  : INMP441 SCK (microphone clock)
├─ GPIO 6  : INMP441 SD (microphone data)
├─ GPIO 7  : MAX98357A DIN (speaker data)
├─ GPIO 15 : MAX98357A BCLK (speaker bit clock)
└─ GPIO 16 : MAX98357A LRCK (speaker LR clock)

Display (SPI):
├─ GPIO 8  : TFT DC (Data/Command)
├─ GPIO 9  : TFT CS (Chip Select)
├─ GPIO 11 : TFT MOSI (Data)
├─ GPIO 12 : TFT CLK (Clock)
├─ GPIO 13 : TFT MISO (Read - optional)
├─ GPIO 18 : TFT RES (Reset)
└─ GPIO 46 : TFT BL (Backlight PWM)

Camera (DVP + I2C):
├─ GPIO 34 : VSYNC
├─ GPIO 35 : HREF
├─ GPIO 36 : PCLK
├─ GPIO 37 : D0
├─ GPIO 38 : D1
├─ GPIO 19 : D2
├─ GPIO 20 : D3
├─ GPIO 22 : D4
├─ GPIO 23 : D5
├─ GPIO 24 : D6
├─ GPIO 25 : D7
├─ GPIO 39 : I2C SDA (camera control)
├─ GPIO 40 : I2C SCL (camera control)
├─ GPIO 21 : Camera RESET
└─ GPIO 47 : Camera PWDN

Buttons:
├─ GPIO 0  : WAKE UP button
├─ GPIO 3  : VOL UP
└─ GPIO 17 : VOL DOWN

LED:
└─ GPIO 48 : Status LED
```

---

## 🤖 HEXAPOD (ESP32-S3) - GPIO SUMMARY (UPDATED)

```
GPIO Assignment:

I2C Bus 0 (Servo Control):
├─ GPIO 41 : SDA (PCA9685 #1 @0x40, #2 @0x41)
└─ GPIO 42 : SCL

I2C Bus 1 (Camera):
├─ GPIO 39 : SDA (OV5640 @0x30)
└─ GPIO 40 : SCL

I2C Bus 2 (Emotion Display):
├─ GPIO 8  : SDA (SSD1306 @0x3C)
└─ GPIO 9  : SCL

SPI Bus (Secondary Status Display - NEW):
├─ GPIO 1  : CLK (ST7735)
├─ GPIO 50 : MOSI (SDA)
├─ GPIO 3  : CS (Chip Select)
├─ GPIO 10 : DC (Data/Command)
├─ GPIO 11 : RST (Reset)
├─ GPIO 14 : BL (Backlight)
└─ GPIO 2  : MISO (optional, read-only)

Camera (DVP):
├─ GPIO 34 : VSYNC
├─ GPIO 35 : HREF
├─ GPIO 36 : PCLK
├─ GPIO 37 : D0
├─ GPIO 38 : D1
├─ GPIO 19 : D2
├─ GPIO 20 : D3
├─ GPIO 22 : D4
├─ GPIO 23 : D5
├─ GPIO 24 : D6
├─ GPIO 25 : D7
├─ GPIO 21 : Camera RESET
└─ GPIO 47 : Camera PWDN

LED:
└─ GPIO 48 : Status LED

Available for expansion:
├─ GPIO 12, 13 (not used)
├─ GPIO 15, 16 (not used)
├─ GPIO 17, 18 (not used)
└─ GPIO 26, 27, 28, 29, 30, 31, 32, 33 (not used)
```

---

## 📊 DISPLAY COMPARISON

| Feature | VoiceBot TFT | Emotion OLED | Status TFT (NEW) |
|---------|-------------|-------------|------------------|
| **Model** | ST7789 | SSD1306 | ST7735 |
| **Resolution** | 240×240 | 128×64 | 80×160 |
| **Diagonal** | 1.54" | 0.96" | 0.96" |
| **Colors** | 16M (RGB565) | 2 (B/W) | 65K (RGB565) |
| **Interface** | SPI | I2C | SPI |
| **Purpose** | UI + Graphics | Emotion face | Status/Info |
| **GPIO Pins** | 8 (SPI+DC+BL) | 2 (I2C) | 7 (SPI+DC+RST+BL) |
| **Power** | 3.3V 100mA | 3.3V 50mA | 3.3V 80mA |

---

## 🔌 MULTI-DISPLAY WIRING DIAGRAM (HEXAPOD)

```
┌─ ESP32-S3 N16R8 ──────────────────────────────────┐
│                                                   │
│  ┌──────────────────────────────────────────┐    │
│  │  I2C Bus 2  (GPIO 8, 9)                  │    │
│  │  400kHz                                   │    │
│  └──────────────┬───────────────────────────┘    │
│                 │                                 │
│        ┌────────▼──────────┐                     │
│        │ SSD1306 OLED      │                     │
│        │ 128×64            │                     │
│        │ Emotion Display   │                     │
│        │ I2C @ 0x3C        │                     │
│        │ (PRIMARY)         │                     │
│        └───────────────────┘                     │
│                                                   │
│  ┌──────────────────────────────────────────┐    │
│  │  SPI Bus 1  (GPIO 1, 50, 3, 10, 11, 14)│    │
│  │  40MHz                                    │    │
│  └──────────────┬───────────────────────────┘    │
│                 │                                 │
│        ┌────────▼──────────────┐                 │
│        │ ST7735 TFT            │                 │
│        │ 0.96" 80×160          │                 │
│        │ Status Display        │                 │
│        │ RGB565 Color          │                 │
│        │ (SECONDARY)           │                 │
│        │                       │                 │
│        │ Shows:                │                 │
│        │ • Battery %           │                 │
│        │ • Motion state        │                 │
│        │ • Connection status   │                 │
│        │ • Real-time stats     │                 │
│        └──────────────────────┘                 │
│                                                   │
└───────────────────────────────────────────────────┘

Display Layout on Robot:
┌──────────────────────┐
│ ┌──────────────────┐ │ Front View
│ │  OLED 128×64     │ │ (Face)
│ │  Emotion Display │ │
│ └──────────────────┘ │
│                      │
│ ┌──────────────────┐ │ Side View
│ │ TFT 80×160       │ │ (Status)
│ │ Status Info      │ │
│ └──────────────────┘ │
└──────────────────────┘
```

---

## 📋 WIRING CHECKLIST - UPDATED

### Hexapod Secondary Display (TFT 0.96" 80×160)

- [ ] TFT VCC → +3.3V
- [ ] TFT GND → GND
- [ ] TFT CLK → GPIO 1
- [ ] TFT MOSI (SDA) → GPIO 50
- [ ] TFT DC → GPIO 10
- [ ] TFT CS → GPIO 3
- [ ] TFT RST → GPIO 11
- [ ] TFT BL → GPIO 14
- [ ] TFT MISO → GPIO 2 (optional)

---

## 🎯 DISPLAY USAGE STRATEGY

### OLED Display (Primary - Emotion)
```
Animation loop:
├─ Happy face (anime style)
├─ Sad expression
├─ Curious raised eyebrows
├─ Excited wide eyes
├─ Neutral resting state
└─ Angry furrowed brow
```

### TFT Display (Secondary - Status)
```
Real-time information:
├─ Battery voltage: [████████░░] 85%
├─ Motion: Walking / Sitting / Jumping
├─ FPS: Camera 30fps
├─ Latency: 45ms
├─ Temp: 42°C
└─ Signal: Wi-Fi [████░░░░░]
```

---

## ⚡ POWER DISTRIBUTION (2 DISPLAYS)

```
3.3V Rail:
├─ ESP32-S3: 150mA
├─ SSD1306 OLED: 50mA
├─ ST7735 TFT: 80mA
├─ OV5640 Camera: 200mA
└─ Total 3.3V: ~480mA (within ESP32 LDO limit)

SPI Bus Considerations:
├─ 2 devices on different buses (no conflict)
├─ SPI1 for servo/status display (40MHz)
├─ Only one at a time active
└─ Both can idle independently
```

---

## 🔧 FIRMWARE CONFIGURATION

### In `main/boards/hexapod_bot/config.h`:

```cpp
// I2C Bus 2 (OLED Emotion Display - Primary)
#define OLED_I2C_SDA            GPIO_NUM_8
#define OLED_I2C_SCL            GPIO_NUM_9
#define OLED_I2C_ADDR           0x3C

// SPI Bus 1 (TFT Status Display - Secondary)
#define TFT_SMALL_SPI_CLK       GPIO_NUM_1
#define TFT_SMALL_SPI_MOSI      GPIO_NUM_50
#define TFT_SMALL_DC_PIN        GPIO_NUM_10
#define TFT_SMALL_SPI_CS        GPIO_NUM_3
#define TFT_SMALL_RST_PIN       GPIO_NUM_11
#define TFT_SMALL_BL_PIN        GPIO_NUM_14
#define TFT_SMALL_WIDTH         80
#define TFT_SMALL_HEIGHT        160
```

---

## 📌 HARDWARE NOTES

1. **I2C vs SPI Selection**
   - OLED on I2C Bus 2 (slower, lower power, fewer pins)
   - TFT on SPI Bus 1 (faster, parallel data, updates quicker)
   - Both active simultaneously without conflict

2. **GPIO Availability**
   - Total ESP32-S3 GPIO: 48
   - Used for displays: GPIO 1, 3, 8, 9, 10, 11, 14, 50 (8 pins)
   - Remaining: 40 GPIO for future expansion

3. **Display Power Management**
   - Both displays use 3.3V rail
   - Can implement GPIO backlight control for power saving
   - Disable TFT updates during idle to save power

4. **Latency Considerations**
   - I2C@400kHz: ~30ms for full OLED frame
   - SPI@40MHz: ~2ms for full TFT frame (faster status updates)

---

## ✅ FINAL HEXAPOD BOT SPECS

| Aspect | Specification |
|--------|---------------|
| **Microcontroller** | ESP32-S3 N16R8 |
| **Motion System** | 18x MG90S servo, 6-legged hexapod |
| **Sensors** | OV5640 5MP camera |
| **Displays** | OLED 128×64 (emotion) + TFT 80×160 (status) |
| **Communication** | Wi-Fi WebSocket (8081) + I2C + SPI |
| **Power** | 7.4V Li-ion 3000mAh → 5V regulated |
| **Weight** | ~500g (estimated with servos + battery) |
| **Battery Life** | 40-60min motion, 6-8hr idle |
| **Update Rate** | 50Hz (motion), 30Hz (camera), 10Hz (displays) |

---

Tất cả cấu hình GPIO đã cập nhật hoàn toàn! 🎉
