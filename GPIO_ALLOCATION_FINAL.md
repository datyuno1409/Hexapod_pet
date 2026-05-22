# 📌 FINAL GPIO ALLOCATION - 2 TFT DISPLAYS ONLY

## 🎯 VOICEBOT (ESP32-S3 N16R8) - MAIN DISPLAY

```
┌─ TFT ST7789 240×240 (1.54") ──────────┐
│  SPI Master Display                   │
│  Resolution: 240×240 px               │
│  Colors: 16M (RGB565)                 │
│  Purpose: Main UI + Graphics          │
│  Interface: SPI 40MHz                 │
└───────────────────────────────────────┘

GPIO Connections:
├─ GPIO 8   : DC (Data/Command)
├─ GPIO 9   : CS (Chip Select)
├─ GPIO 11  : MOSI (Data)
├─ GPIO 12  : CLK (Clock)
├─ GPIO 13  : MISO (Read - optional)
├─ GPIO 18  : RES (Reset)
└─ GPIO 46  : BL (Backlight PWM)

Full GPIO Map (VoiceBot):

Audio I2S:
├─ GPIO 4  : INMP441 WS (microphone)
├─ GPIO 5  : INMP441 SCK
├─ GPIO 6  : INMP441 SD
├─ GPIO 7  : MAX98357A DIN (speaker)
├─ GPIO 15 : MAX98357A BCLK
└─ GPIO 16 : MAX98357A LRCK

Display SPI:
├─ GPIO 8  : TFT DC
├─ GPIO 9  : TFT CS
├─ GPIO 11 : TFT MOSI
├─ GPIO 12 : TFT CLK
├─ GPIO 13 : TFT MISO (opt)
├─ GPIO 18 : TFT RES
└─ GPIO 46 : TFT BL

Camera DVP + I2C:
├─ GPIO 34 : VSYNC
├─ GPIO 35 : HREF
├─ GPIO 36 : PCLK
├─ GPIO 37-38, 19-20, 22-25: D0-D7
├─ GPIO 39 : I2C SDA (camera)
├─ GPIO 40 : I2C SCL (camera)
├─ GPIO 21 : Camera RESET
└─ GPIO 47 : Camera PWDN

Buttons:
├─ GPIO 0  : WAKE UP
├─ GPIO 3  : VOL UP
└─ GPIO 17 : VOL DOWN

LED:
└─ GPIO 48 : Status LED
```

---

## 🤖 HEXAPOD (ESP32-S3 N16R8) - STATUS DISPLAY

```
┌─ TFT ST7735 0.96" 80×160 ─────────────┐
│  SPI Slave Display                    │
│  Resolution: 80×160 px                │
│  Colors: 65K (RGB565)                 │
│  Purpose: Status/Info Display         │
│  Interface: SPI 40MHz                 │
└───────────────────────────────────────┘

GPIO Connections:
├─ GPIO 1  : CLK (Clock)
├─ GPIO 50 : MOSI (SDA - Data)
├─ GPIO 3  : CS (Chip Select)
├─ GPIO 10 : DC (Data/Command)
├─ GPIO 11 : RST (Reset)
├─ GPIO 14 : BL (Backlight)
└─ GPIO 2  : MISO (opt - Read)

Full GPIO Map (Hexapod):

I2C Bus 0 (Servo Control):
├─ GPIO 41 : SDA (PCA9685 #1 @0x40, #2 @0x41)
└─ GPIO 42 : SCL

I2C Bus 1 (Camera):
├─ GPIO 39 : SDA (OV5640 @0x30)
└─ GPIO 40 : SCL

SPI Bus 1 (Display):
├─ GPIO 1  : CLK
├─ GPIO 50 : MOSI (SDA)
├─ GPIO 3  : CS
├─ GPIO 10 : DC
├─ GPIO 11 : RST
└─ GPIO 14 : BL

Camera DVP:
├─ GPIO 34 : VSYNC
├─ GPIO 35 : HREF
├─ GPIO 36 : PCLK
├─ GPIO 37-38, 19-20, 22-25: D0-D7
├─ GPIO 39 : I2C SDA (camera control)
├─ GPIO 40 : I2C SCL (camera control)
├─ GPIO 21 : Camera RESET
└─ GPIO 47 : Camera PWDN

LED:
└─ GPIO 48 : Status LED

Available Expansion:
├─ GPIO 12, 13, 15, 16, 17, 18
└─ GPIO 26-33 (11 more GPIO)
```

---

## 📊 DISPLAY COMPARISON

| Aspect | VoiceBot | HexapodBot |
|--------|----------|-----------|
| **Model** | ST7789 | ST7735 |
| **Size** | 1.54" | 0.96" |
| **Resolution** | 240×240 | 80×160 |
| **Colors** | 16M RGB | 65K RGB |
| **Interface** | SPI | SPI |
| **Update Speed** | 40MHz | 40MHz |
| **Power** | 100mA | 80mA |
| **Purpose** | Main UI | Status Info |
| **GPIO Pins** | 8 | 7 |

---

## 🔌 WIRING CHECKLIST

### VoiceBot TFT (1.54" 240×240)
```
TFT Pin          ESP32-S3 GPIO
GND              GND
+3.3V            +3.3V
SCL (CLK)        GPIO 12
SDA (MOSI)       GPIO 11
RES (RESET)      -1 (no connect)
DC               GPIO 8
CS               GPIO 9
BLK              GPIO 46
MISO             GPIO 13 (optional)
```

### HexapodBot TFT (0.96" 80×160)
```
TFT Pin          ESP32-S3 GPIO
GND              GND
+3.3V            +3.3V
SCL (CLK)        GPIO 1
SDA (MOSI)       GPIO 50
RES (RESET)      GPIO 11
DC               GPIO 10
CS               GPIO 3
BLK              GPIO 14
MISO             GPIO 2 (optional)
```

---

## 📋 COMPLETE HARDWARE LIST

| Component | Model | Qty | Purpose |
|-----------|-------|-----|---------|
| ESP32-S3 N16R8 | VoiceBot | 1 | Master + TFT 240×240 |
| ESP32-S3 N16R8 | HexapodBot | 1 | Servo controller + TFT 80×160 |
| TFT Display | ST7789 1.54" 240×240 | 1 | Main UI (VoiceBot) |
| TFT Display | ST7735 0.96" 80×160 | 1 | Status (HexapodBot) |
| Microphone | INMP441 I2S | 1 | Audio input |
| Amplifier | MAX98357A I2S | 1 | Speaker output |
| Camera | OV5640 5MP | 1 | Vision (Hexapod) |
| PWM Driver | PCA9685 | 2 | Servo control @0x40,0x41 |
| Servo | MG90S | 18 | 6-leg hexapod (3 DOF each) |
| Voltage Reg | AMS1117 5V | 1 | 7.4V → 5V conversion |
| Battery | 2S Li-ion 3000mAh | 1 | 7.4V power source |

---

## ⚡ POWER DISTRIBUTION

```
Battery (7.4V, 3000mAh)
    ↓
    ├─→ AMS1117 5V Regulator (1.5A)
    │       ↓
    │   ┌───┴──────┬──────────┬──────────┐
    │   ├─ 5V      ├─ 5V     ├─ 5V     ├─ 5V
    │   │          │         │         │
    │   ↓          ↓         ↓         ↓
    │  MAX98357A  PCA9685#1 PCA9685#2 18x Servo
    │  (500mA)    (100mA)   (100mA)   (2-3A avg)
    │
    ├─→ ESP32-S3 LDO (onboard) → 3.3V
            ├─ TFT Display 240×240 (100mA)
            ├─ TFT Display 80×160 (80mA)
            ├─ OV5640 Camera (200mA)
            ├─ INMP441 Micro (50mA)
            └─ I2C devices (50mA)

Total 3.3V draw: ~480mA (within ESP32 LDO 600mA limit)
Total 5V draw: ~2.8A average, 4A peak (servos)
```

---

## 🎨 DISPLAY USAGE

### VoiceBot (TFT 1.54" 240×240)
```
┌─────────────────────────┐
│ ╔═════════════════════╗ │ Landscape orientation
│ ║   VOICEBOT UI       ║ │ Shows: 
│ ║  ┌─────────────────┐║ │ • Waveform
│ ║  │ ~~~▄▄▄▄▄▄▄~~  ││ │ • Status
│ ║  │ ▄▄▀▀▀▀▀▀▀▀▀▀▀▄▄│ │ • Time
│ ║  │ ▀           ▀ │ │ • Temp
│ ║  │ Connected!   ││ │
│ ║  └─────────────────┘║ │
│ ║ Vol: ████░░░░░░ 60% ║ │
│ ╚═════════════════════╝ │
└─────────────────────────┘
```

### HexapodBot (TFT 0.96" 80×160)
```
┌────────────────┐
│ ╔════════════╗ │ Landscape orientation
│ ║  HEXAPOD   ║ │ Shows real-time:
│ ║ Batt: 85%  ║ │ • Battery level
│ ║ Motion: Run║ │ • Current motion
│ ║ FPS: 30    ║ │ • Frame rate
│ ║ Temp: 42°C ║ │ • Temperature
│ ║ Latency:45 ║ │ • Network latency
│ ║ Signal: ████║ │ • Wi-Fi signal
│ ╚════════════╝ │
└────────────────┘
```

---

## 📝 CONFIG FILE SUMMARY

### VoiceBot (`hexapod_voicebot/config.h`)
```cpp
// TFT Display (Main UI)
#define TFT_SPI_MOSI GPIO_NUM_11
#define TFT_SPI_CLK  GPIO_NUM_12
#define TFT_SPI_CS   GPIO_NUM_9
#define TFT_DC_PIN   GPIO_NUM_8
#define TFT_BL_PIN   GPIO_NUM_46
#define TFT_WIDTH    240
#define TFT_HEIGHT   240
```

### HexapodBot (`hexapod_bot/config.h`)
```cpp
// TFT Display (Status Info)
#define TFT_SMALL_SPI_CLK    GPIO_NUM_1
#define TFT_SMALL_SPI_MOSI   GPIO_NUM_50
#define TFT_SMALL_DC_PIN     GPIO_NUM_10
#define TFT_SMALL_SPI_CS     GPIO_NUM_3
#define TFT_SMALL_RST_PIN    GPIO_NUM_11
#define TFT_SMALL_BL_PIN     GPIO_NUM_14
#define TFT_SMALL_WIDTH      80
#define TFT_SMALL_HEIGHT     160
```

---

## ✅ SUMMARY

✨ **Clean Configuration:**
- 2 ESP32-S3 boards
- 2 TFT displays (1.54" + 0.96")
- 18 servo motors
- 1 camera
- 1 microphone + amplifier
- 1 battery

✨ **No Conflicts:**
- Different SPI buses for displays
- Separate I2C buses for peripherals
- 40+ GPIO available for expansion

✨ **Ready to Build:**
- All GPIO allocated
- Power distribution planned
- Wiring checklist ready
- No unused complexity

---

**GPIO Allocation: FINAL ✓**
