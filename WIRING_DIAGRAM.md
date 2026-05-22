# 🔌 SƠ ĐỒ NỐI CHÂN HEXAPOD SYSTEM

## 📋 DANH SÁCH THIẾT BỊ

| # | Thiết bị | Số lượng | Ghi chú |
|----|---------|---------|--------|
| 1 | ESP32-S3 N16R8 (VoiceBot) | 1 | Chính + TFT + Camera |
| 2 | ESP32-S3 N16R8 (Hexapod) | 1 | Phụ - Servo controller |
| 3 | TFT ST7789 240×240 | 1 | SPI display (VoiceBot) |
| 4 | INMP441 | 1 | I2S digital microphone |
| 5 | MAX98357A | 1 | I2S amplifier |
| 6 | OV5640 | 1 | Camera 5MP (Hexapod) |
| 7 | PCA9685 | 2 | 16-ch PWM servo controller @ 0x40, 0x41 |
| 8 | MG90S Servo | 18 | 3-wire: GND, +5V, Signal |
| 9 | SSD1306 OLED 128×64 | 1 | I2C emotion display (Hexapod) |
| 10 | AMS1117 5V | 1 | Voltage regulator 7.4V → 5V |

---

## 🎯 VOICEBOT (ESP32-S3 N16R8) - MASTER

### I2S AUDIO (Microphone + Amplifier)

```
┌─ MICROPHONE (INMP441) ────────────────────────┐
│  Pin Name        Pin #    ESP32-S3 GPIO       │
├──────────────────────────────────────────────┤
│  GND             1        GND                 │
│  L/R             2        GND (mono)          │
│  WS              3        GPIO_NUM_4          │
│  SCK             4        GPIO_NUM_5          │
│  SD              5        GPIO_NUM_6          │
│  VDD             6        +3.3V               │
└──────────────────────────────────────────────┘

┌─ AMPLIFIER (MAX98357A) ────────────────────────┐
│  Pin Name        Pin #    ESP32-S3 GPIO        │
├───────────────────────────────────────────────┤
│  GND             1        GND                  │
│  DIN             2        GPIO_NUM_7           │
│  BCLK            3        GPIO_NUM_15          │
│  LRCLK           4        GPIO_NUM_16          │
│  SD              5        +3.3V (Enable)       │
│  GAIN            6        GND (24dB)           │
│  VDD             7        +5V                  │
│  OUT- (GND)      8        GND via 10kΩ        │
│  OUT+            8        Speaker+ (8Ω)       │
│  GND             9        GND                  │
└───────────────────────────────────────────────┘
```

### TFT DISPLAY (ST7789 240×240 via SPI)

```
┌─ TFT ST7789 ──────────────────────────────────┐
│  Pin Name        Pin #    ESP32-S3 GPIO        │
├───────────────────────────────────────────────┤
│  GND             1        GND                  │
│  VCC             2        +3.3V                │
│  SCL (CLK)       3        GPIO_NUM_12          │
│  SDA (MOSI)      4        GPIO_NUM_11          │
│  RES (RESET)     5        GPIO_NUM_18          │
│  DC              6        GPIO_NUM_8           │
│  CS              7        GPIO_NUM_9           │
│  BLK (Backlight) 8        GPIO_NUM_46          │
│  MISO            9        GPIO_NUM_13 (N/A)    │
└───────────────────────────────────────────────┘
```

### CAMERA (OV5640 - DVP Interface)

```
┌─ OV5640 CAMERA MODULE ────────────────────────┐
│  Pin Name        Pin #    ESP32-S3 GPIO        │
├───────────────────────────────────────────────┤
│  GND             -        GND                  │
│  VCC             -        +3.3V                │
│  SDA (I2C)       -        GPIO_NUM_39          │
│  SCL (I2C)       -        GPIO_NUM_40          │
│  PWDN            -        GPIO_NUM_47          │
│  RESET           -        GPIO_NUM_21          │
│  PCLK            -        GPIO_NUM_36          │
│  VSYNC           -        GPIO_NUM_34          │
│  HREF            -        GPIO_NUM_35          │
│  D0-D7 (Data)    -        GPIO_NUM_37,38,     │
│                            19,20,22,23,24,25  │
│  XCLK            -        (Internal OSC)      │
└───────────────────────────────────────────────┘

I2C Camera Configuration:
├─ SDA: GPIO 39
├─ SCL: GPIO 40
├─ Freq: 400 kHz
└─ Addr: 0x30
```

### NÚT BẤM (VoiceBot)

```
┌─ BUTTONS ─────────────────────────────────────┐
│  Button          ESP32-S3 GPIO     Pull-up     │
├───────────────────────────────────────────────┤
│  WAKE UP         GPIO_NUM_0        Internal    │
│  VOL UP          GPIO_NUM_3        Internal    │
│  VOL DOWN        GPIO_NUM_18       Internal    │
│  to GND with 10kΩ resistor                    │
└───────────────────────────────────────────────┘
```

---

## 🦿 HEXAPOD BOT (ESP32-S3 N16R8) - SLAVE

### I2C BUS 1: PCA9685 SERVO CONTROLLERS

```
┌─ I2C BUS 1 (Servo) ───────────────────────────┐
│  Device          Address   GPIO                │
├───────────────────────────────────────────────┤
│  PCA9685 #1      0x40      SDA=GPIO_NUM_41    │
│  PCA9685 #2      0x41      SCL=GPIO_NUM_42    │
│  Frequency: 400 kHz                           │
│  Pullup: Internal enabled                     │
└───────────────────────────────────────────────┘

┌─ PCA9685 #1 (Servos 0-8) ─────────────────────┐
│  Channel  Servo  Leg      DOF          Pin    │
├───────────────────────────────────────────────┤
│  0        0      FL_COXA  Hip rotation  OUT0  │
│  1        1      FL_FEMUR Forward/back  OUT1  │
│  2        2      FL_TIBIA Up/down       OUT2  │
│  3        3      FR_COXA  Hip rotation  OUT3  │
│  4        4      FR_FEMUR Forward/back  OUT4  │
│  5        5      FR_TIBIA Up/down       OUT5  │
│  6        6      ML_COXA  Hip rotation  OUT6  │
│  7        7      ML_FEMUR Forward/back  OUT7  │
│  8        8      ML_TIBIA Up/down       OUT8  │
└───────────────────────────────────────────────┘

┌─ PCA9685 #2 (Servos 9-17) ────────────────────┐
│  Channel  Servo  Leg      DOF          Pin    │
├───────────────────────────────────────────────┤
│  0        9      MR_COXA  Hip rotation  OUT0  │
│  1        10     MR_FEMUR Forward/back  OUT1  │
│  2        11     MR_TIBIA Up/down       OUT2  │
│  3        12     BL_COXA  Hip rotation  OUT3  │
│  4        13     BL_FEMUR Forward/back  OUT4  │
│  5        14     BL_TIBIA Up/down       OUT5  │
│  6        15     BR_COXA  Hip rotation  OUT6  │
│  7        16     BR_FEMUR Forward/back  OUT7  │
│  8        17     BR_TIBIA Up/down       OUT8  │
└───────────────────────────────────────────────┘

PCA9685 PINOUT (Both #1 and #2):
┌─ PCA9685 Connector ───────────────┐
│  GND    Power      I2C            │
│  ├─ GND  ├─ +5V    ├─ SDA (both)  │
│  └─ OUT0-OUT15    └─ SCL (both)   │
└───────────────────────────────────┘
```

### SERVO MG90S (3-wire x18)

```
┌─ MG90S SERVO ─────────────────┐
│  Wire       Color    Connect  │
├─────────────────────────────┤
│  GND        Brown/Blk  GND   │
│  +5V        Red        +5V   │
│  Signal     Yellow/Wht OUT0-17 (PCA9685) │
└─────────────────────────────┘

SERVO PLACEMENT on PCA9685:
FL: Servos 0,1,2  → PCA9685 #1 OUT0,1,2
FR: Servos 3,4,5  → PCA9685 #1 OUT3,4,5
ML: Servos 6,7,8  → PCA9685 #1 OUT6,7,8
MR: Servos 9,10,11 → PCA9685 #2 OUT0,1,2
BL: Servos 12,13,14 → PCA9685 #2 OUT3,4,5
BR: Servos 15,16,17 → PCA9685 #2 OUT6,7,8
```

### I2C BUS 0: CAMERA + OLED

```
┌─ I2C BUS 0 (Camera) ───────────────────┐
│  Device          Address   GPIO        │
├────────────────────────────────────────┤
│  OV5640 Camera   0x30      SDA=39,SCL=40
│  Frequency: 400 kHz                    │
│  Pullup: Internal enabled              │
└────────────────────────────────────────┘

┌─ SPI BUS: TFT Status Display ──────────┐
│  TFT ST7735 0.96" 80×160               │
│  Resolution: 80×160 px                 │
│  Colors: 65K (RGB565)                  │
│  Frequency: 40 MHz                     │
│  Purpose: Real-time status info        │
│  GPIO: CLK=1, MOSI=50, DC=10,          │
│        CS=3, RST=11, BL=14             │
└────────────────────────────────────────┘
```

### LED STATUS

```
┌─ STATUS LED ──────────────────┐
│  Type            GPIO          │
├─────────────────────────────┤
│  Built-in LED    GPIO_NUM_48   │
│  to GND via 10kΩ resistor      │
└─────────────────────────────┘
```

---

## 🔋 POWER SUPPLY

```
┌─ POWER DISTRIBUTION ──────────────────────────┐
│  Source: Li-ion 2S Battery 7.4V 3000mAh       │
│                                               │
│  7.4V → AMS1117 5V Regulator → 5V 4A         │
│                                               │
│  Distribution:                                │
│  ├─ ESP32-S3 #1: +3.3V (onboard LDO)         │
│  ├─ ESP32-S3 #2: +3.3V (onboard LDO)         │
│  ├─ MAX98357A: +5V (500mA)                   │
│  ├─ PCA9685 #1: +5V (100mA)                  │
│  ├─ PCA9685 #2: +5V (100mA)                  │
│  ├─ 18x Servo: +5V (4A peak, 2A avg)         │
│  ├─ TFT Display: +3.3V (100mA)               │
│  ├─ OV5640 Camera: +3.3V (200mA)             │
│  └─ OLED Display: +3.3V (50mA)               │
│                                               │
│  Capacitors:                                  │
│  ├─ 1000µF/10V across 5V output              │
│  ├─ 100µF/10V at each PCA9685 VCC            │
│  └─ 100µF/10V at servo connector             │
└───────────────────────────────────────────────┘
```

---

## 📡 WIRELESS CONNECTION (Wi-Fi)

```
VOICEBOT ←→ HEXAPOT
Via Wi-Fi (same local network)

┌─ WebSocket Connection ────────────────┐
│  VoiceBot         Hexapod             │
│  192.168.x.100    192.168.x.101 (fixed)
│  Client → Server                     │
│  Port: 8081                          │
│  Protocol: WebSocket JSON            │
└──────────────────────────────────────┘
```

---

## 🛠️ WIRING CHECKLIST

### VOICEBOT Wiring

- [ ] **AUDIO PATH**
  - [ ] INMP441 WS → GPIO 4
  - [ ] INMP441 SCK → GPIO 5
  - [ ] INMP441 SD → GPIO 6
  - [ ] INMP441 GND → GND
  - [ ] INMP441 VDD → 3.3V
  - [ ] MAX98357A BCLK → GPIO 15
  - [ ] MAX98357A LRCK → GPIO 16
  - [ ] MAX98357A DIN → GPIO 7
  - [ ] MAX98357A VDD → 5V
  - [ ] MAX98357A GND → GND
  - [ ] Speaker to MAX98357A OUT+/OUT-

- [ ] **TFT DISPLAY**
  - [ ] TFT VCC → 3.3V
  - [ ] TFT GND → GND
  - [ ] TFT CLK → GPIO 12
  - [ ] TFT MOSI → GPIO 11
  - [ ] TFT CS → GPIO 9
  - [ ] TFT DC → GPIO 8
  - [ ] TFT RES → GPIO 18
  - [ ] TFT BL → GPIO 46
  - [ ] TFT MISO → GPIO 13 (can leave unconnected for write-only)

- [ ] **CAMERA**
  - [ ] Camera SDA → GPIO 39
  - [ ] Camera SCL → GPIO 40
  - [ ] Camera D0-D7 → GPIO 37,38,19,20,22,23,24,25
  - [ ] Camera PCLK → GPIO 36
  - [ ] Camera VSYNC → GPIO 34
  - [ ] Camera HREF → GPIO 35
  - [ ] Camera PWDN → GPIO 47
  - [ ] Camera RESET → GPIO 21

- [ ] **BUTTONS**
  - [ ] WAKE UP Button → GPIO 0, GND
  - [ ] VOL UP → GPIO 3, GND
  - [ ] VOL DOWN → GPIO 18, GND (use resistor divider if needed)

### HEXAPOD BOT Wiring

- [ ] **PCA9685 #1 (0x40)**
  - [ ] PCA9685 SDA → GPIO 41
  - [ ] PCA9685 SCL → GPIO 42
  - [ ] PCA9685 VCC → 5V (with 100µF cap)
  - [ ] PCA9685 GND → GND
  - [ ] PCA9685 OUT0-8 → Servo signals (FL, FR, ML legs)

- [ ] **PCA9685 #2 (0x41)**
  - [ ] Address pin A0 → +5V (to set address 0x41)
  - [ ] PCA9685 SDA → GPIO 41 (same bus as #1)
  - [ ] PCA9685 SCL → GPIO 42 (same bus as #1)
  - [ ] PCA9685 VCC → 5V (with 100µF cap)
  - [ ] PCA9685 GND → GND
  - [ ] PCA9685 OUT0-8 → Servo signals (MR, BL, BR legs)

- [ ] **18x SERVO MG90S**
  - [ ] All servo GND → GND (common)
  - [ ] All servo VCC → +5V (common)
  - [ ] Servo 0 signal → PCA9685 #1 OUT0
  - [ ] Servo 1 signal → PCA9685 #1 OUT1
  - [ ] ... (18 servos total)
  - [ ] Servo 17 signal → PCA9685 #2 OUT8

- [ ] **CAMERA**
  - [ ] Camera SDA → GPIO 39
  - [ ] Camera SCL → GPIO 40
  - [ ] Camera D0-D7 → GPIO 37,38,19,20,22,23,24,25
  - [ ] Camera PCLK → GPIO 36
  - [ ] Camera VSYNC → GPIO 34
  - [ ] Camera HREF → GPIO 35
  - [ ] Camera PWDN → GPIO 47
  - [ ] Camera RESET → GPIO 21

- [ ] **TFT DISPLAY (0.96" 80×160)**
  - [ ] TFT VCC → +3.3V
  - [ ] TFT GND → GND
  - [ ] TFT SCL (CLK) → GPIO 1
  - [ ] TFT SDA (MOSI) → GPIO 50
  - [ ] TFT DC → GPIO 10
  - [ ] TFT CS → GPIO 3
  - [ ] TFT RST → GPIO 11
  - [ ] TFT BL → GPIO 14
  - [ ] TFT MISO → GPIO 2 (optional, can leave unconnected)

- [ ] **STATUS LED**
  - [ ] LED → GPIO 48 (via 10kΩ resistor to GND)

---

## ⚡ VOLTAGE LEVELS

| Component | Supply | Logic |
|-----------|--------|-------|
| ESP32-S3 | 3.3V | 3.3V |
| TFT ST7789 | 3.3V | 3.3V |
| MAX98357A | 5V | 3.3V |
| PCA9685 | 5V | 3.3V (I2C tolerant) |
| Servo MG90S | 5V | 3.3V/5V (tolerant) |
| OV5640 | 3.3V | 3.3V |
| SSD1306 | 3.3V | 3.3V |
| INMP441 | 3.3V | 3.3V |

---

## 🔍 DEBUGGING TIPS

```python
# Verify I2C devices:
# VoiceBot: Camera @ 0x30 (I2C bus 1)
# Hexapod: PCA9685 @ 0x40, 0x41 (I2C bus 0)
#          OLED @ 0x3C (I2C bus 2)
#          Camera @ 0x30 (I2C bus 1)

# PCA9685 Address Setting:
# Default (A0-A5 to GND): 0x40
# A0 to +5V: 0x41
# Use different address pins to set multiple boards
```

---

## 📌 NOTES

1. **I2C Pullup Resistors:** ESP32 có internal pullup, nên có thể không cần thêm
2. **Servo Power:** 18 servo peak current ~3-4A, cần pin 7.4V có khả năng
3. **Decoupling Capacitors:** Quan trọng cho PCA9685 + servo rails
4. **Wire Gauge:** 
   - 5V servo rail: AWG 16-18 (để giảm voltage drop)
   - 3.3V logic: AWG 22-24
   - GND: AWG 16
5. **PCB Layout:** Đặt capacitor gần pin VCC/GND của PCA9685
