# 🔌 BLOCK DIAGRAM - HEXAPOD SYSTEM

## VOICEBOT COMPLETE SYSTEM

```
┌─────────────────────────────────────────────────────────────────────┐
│                        VOICEBOT (Master)                            │
│                      ESP32-S3 N16R8                                 │
└─────────────────────────────────────────────────────────────────────┘

                    ┌─────────────────────────┐
                    │   ESP32-S3 N16R8        │
                    │   (3.3V Logic)          │
                    └─────────────────────────┘
                    /       |        |        \
          ┌─────────┴───┬───┴────┬──┴──┬──────┴──────┐
          |             |        |     |             |
      [AUDIO]       [DISPLAY] [I2C] [GPIO]       [CAMERA]
          |             |        |     |             |
    ┌─────┴──────┐  ┌────┴───┐  |  ┌──┴──┐         ┌┴──────┐
    |I2S Mic Out |  |SPI TFT │  |  │Btns │         │Camera │
    │GPIO 4,5,6  │  │ST7789  │  |  │GPIO │       I2C 39,40
    └─────┬──────┘  │240x240 │  |  │0,3,4│       OV5640
          │         └────┬───┘  |  └─────┘         └┬──────┘
          │              │      |
    ┌─────▼──────┐       │   ┌──┴──┐
    │ INMP441    │       │   │Nút  │
    │Microphone  │       │   │Bấm  │
    │I2S Digital │       │   │3 nút│
    └────────────┘       │   └─────┘
          │              │
          │         ┌────▼───────────┐
    ┌─────▼─────────┤ SPI Interface  │
    │ GPIO 7,15,16  │ + DC Pin GPIO8 │
    │               │ + CS Pin GPIO9 │
    │               │ + BL GPIO46    │
    │               └────┬───────────┘
    │                    │
    │              ┌─────▼────────┐
    │              │  TFT ST7789  │
    │              │  240×240 px  │
    │              │  0.96" color │
    └──────────────┤ LVGL Support │
                   └──────────────┘

Audio Output Flow:
┌──────────┐     ┌─────────────┐      ┌──────────────┐      ┌─────┐
│ Speaker │◄────┤MAX98357A    │◄─────┤ ESP32-S3    │◄─────┤ MIC │
│  8Ω    │     │ Amplifier   │      │ I2S Out     │      │     │
│ +/-    │     │ +5V         │      │ GPIO 7      │      └─────┘
└──────────┘     └─────────────┘      └──────────────┘
   OUT+/OUT-            VDD                 PWM
```

---

## HEXAPOD BOT COMPLETE SYSTEM

```
┌──────────────────────────────────────────────────────────────────┐
│                   HEXAPOD BOT (Slave)                            │
│                  ESP32-S3 N16R8                                  │
└──────────────────────────────────────────────────────────────────┘

                   ┌────────────────────┐
                   │  ESP32-S3 N16R8    │
                   │  (3.3V Logic)      │
                   └────────────────────┘
                  /        |    |    |  \
         ┌────────┴───┬────┴┬───┴────┴──┬──┴────┐
         |            |    |   |       |       |
     [I2C Bus 0]  [I2C Bus 1] | [I2C Bus 2] [SPI] [Status]
         |            |    |  |   |       |     |
    ┌────┴──┐   ┌─────┴────┐ |  |  ┌──────┘   ┌─┴──┐
    │Camera │   │PCA9685 #1│ |  │  │OLED  TFT │LED │
    │OV5640 │   │+ #2      │ |  │  │Disp  Disp│G48 │
    │SDA39  │   │I2C41,42  │ |  │  │128×64 0.96
    │SCL40  │   │Freq 400k │ |  │  │I2C 8,9 SPI
    │0x30   │   │0x40,0x41 │ |  │  │         1,50
    └───┬───┘   └──┬───────┘ |  │  └──────┬──┐ │
        │          │         |  │         │  │ │
   ┌────▼──────┐   │    ┌────┴──┴──┐  ┌──┴──┴─▼─┐
   │  Camera   │   │    │   OLED   │  │  TFT   │
   │  Driver   │   │    │  Primary │  │Secondary
   │   DVP I/O │   │    │ Emotion  │  │  Status │
   │ GPIO      │   │    │ Display  │  │ 80×160 │
   │ 34-38,19, │   │    │         │  │ ST7735 │
   │ 20,22-25  │   │    └────┬─────┘  │  0.96" │
   └───────────┘   │         │        └────┬────┘
                   │    ┌────▼────┐       │
                   │    │  Faces: │       │
                   │    │ Happy   │  ┌────▼────┐
                   │    │ Sad     │  │ Battery │
                   │    │ Curious │  │ Motion  │
                   │    │ Excited │  │ State   │
                   │    │ Neutral │  │ Signals │
                   │    └─────────┘  └─────────┘
                   │
        ┌──────────┴──────────────┐
        |                         |
    ┌───▼─────────┐      ┌─────────▼──────┐
    │ PCA9685 #1  │      │ PCA9685 #2     │
    │ (0x40)      │      │ (0x41)         │
    │ 16 PWM CH   │      │ 16 PWM CH      │
    │ +5V VCC     │      │ +5V VCC        │
    │ 100µF cap   │      │ 100µF cap      │
    └────┬────────┘      └────────┬───────┘
         │                        │
  ┌──────┴──────┐          ┌──────┴──────┐
  |OUT 0-8      |          |OUT 0-8      |
  |Servos 0-8   |          |Servos 9-17  |
  └──────┬──────┘          └──────┬──────┘
         │                        │
 ┌───────┴────────────────────────┴───────┐
 │                                        │
 │  SERVO CONTROL (18x MG90S)            │
 │  All +5V common, GND common            │
 │  PWM signals from PCA9685             │
 │                                        │
 │  ┌─────────┐  ┌─────────┐             │
 │  │ Front   │  │ Middle  │             │
 │  │ Legs    │  │ Legs    │  ┌────────┐│
 │  │ (FL,FR) │  │(ML,MR)  │  │Back    ││
 │  │ Srv0-5  │  │Srv6-11  │  │Legs   ││
 │  │3DOF/leg │  │3DOF/leg │  │(BL,BR)││
 │  │         │  │         │  │Srv12-17
 │  │Coxa     │  │Coxa     │  │3DOF/leg
 │  │Femur    │  │Femur    │  │
 │  │Tibia    │  │Tibia    │  │
 │  └─────────┘  └─────────┘  └────────┘
 │                                        │
 │  Each servo:                          │
 │  ├─ Red: +5V                          │
 │  ├─ Black: GND                        │
 │  └─ Yellow: PWM signal (0-17)         │
 └────────────────────────────────────────┘
```

---

## POWER DISTRIBUTION DIAGRAM

```
┌────────────────────────────────────┐
│  Battery Pack                      │
│  2S Li-ion (7.4V)                  │
│  3000mAh                           │
│  (max 4A discharge)                │
└─────────────┬──────────────────────┘
              │ 7.4V
              │
         ┌────▼─────┐
         │ Switch   │
         │ ON/OFF   │
         └────┬─────┘
              │ 7.4V
         ┌────▼────────────────┐
         │ AMS1117 5V Reg      │
         │ 1.5A capacity       │
         │ IN: 7.4V            │
         │ OUT: 5V ±5%         │
         │ 1000µF filter cap   │
         └────┬─────────────────┘
              │ 5V
    ┌─────────┴──────────────┬─────────┐
    │                        │         │
┌───▼──────┐         ┌───────▼────┐ ┌─▼────────┐
│PCA9685 #1│         │PCA9685 #2  │ │MAX98357A│
│100µF cap │         │100µF cap   │ │Speaker  │
│+5V -GND  │         │+5V -GND    │ │Amplifier
└───┬──────┘         └────┬───────┘ └─┬────────┘
    │                     │           │ 500mA
 18 Servos            18 Servos      │
 (9 each)             (9 each)    +5V -GND
 3-4A peak            ├─────────────────┐
                      │                 │
                 ┌────▼────┐      ┌─────▼──────┐
                 │ Servo   │      │ Speaker    │
                 │ Motors  │      │ 8Ω 3W      │
                 │ +5V -GND│      │  +  -      │
                 └─────────┘      └────────────┘

Voltage Rails:
├─ 7.4V: Battery direct (no devices)
├─ 5V: Regulators → PCA9685, MAX98357A, Servos (4A combined)
└─ 3.3V: ESP32 LDO (onboard) for logic + peripherals
```

---

## I2C ADDRESSING SCHEME & SPI INTERFACES

```
VOICEBOT (I2C Bus 1 - GPIO 39/40):
┌────────────────────────────────┐
│ I2C Address Map                │
├────────────────────────────────┤
│ 0x30  │ OV5640 Camera          │
└────────────────────────────────┘

HEXAPOD (Multi-bus):
┌────────────────────────────────────────┐
│ I2C Bus 0 (GPIO 41/42) - Servo:        │
├────────────────────────────────────────┤
│ 0x40  │ PCA9685 #1 (OUT 0-15)          │
│ 0x41  │ PCA9685 #2 (OUT 0-15)          │
│ Freq: 400 kHz                          │
└────────────────────────────────────────┘

┌────────────────────────────────────────┐
│ I2C Bus 1 (GPIO 39/40) - Camera:       │
├────────────────────────────────────────┤
│ 0x30  │ OV5640 Camera                  │
│ Freq: 400 kHz                          │
└────────────────────────────────────────┘

┌────────────────────────────────────────┐
│ I2C Bus 2 (GPIO 8/9) - Emotion Display:│
├────────────────────────────────────────┤
│ 0x3C  │ SSD1306 OLED (128×64)          │
│       │ Primary emotion display         │
│ Freq: 400 kHz                          │
└────────────────────────────────────────┘

┌────────────────────────────────────────┐
│ SPI Bus 1 (GPIO 1,50,10,3) - Status:   │
├────────────────────────────────────────┤
│ ST7735 │ TFT 0.96" 80×160             │
│        │ Secondary status display      │
│ Freq:  │ 40 MHz                        │
│ Pins:  │ CLK=GPIO1, MOSI=GPIO50       │
│        │ DC=GPIO10, CS=GPIO3           │
│        │ RST=GPIO11, BL=GPIO14         │
└────────────────────────────────────────┘
```

---

## SPI INTERFACE (TFT ONLY)

```
┌─ TFT ST7789 SPI ────────────────┐
│ GPIO 12  │ SCL (CLK)           │
│ GPIO 11  │ SDA (MOSI)          │
│ GPIO 13  │ MISO (read-only)    │
│ GPIO 9   │ CS (Chip Select)    │
│ GPIO 8   │ DC (Data/Command)   │
│ GPIO 46  │ BL (Backlight PWM)  │
│ GPIO -1  │ RST (not connected) │
│                                │
│ Frequency: 40 MHz              │
│ Mode: 0 (CPOL=0, CPHA=0)       │
└────────────────────────────────┘

SPI Data Flow:
ESP32 MOSI (GPIO 11) → TFT SDA
ESP32 CLK  (GPIO 12) → TFT SCL
ESP32 CS   (GPIO 9)  → TFT CS (active low)
ESP32 DC   (GPIO 8)  → TFT DC (0=cmd, 1=data)
```

---

## SERVO PWM SIGNAL TIMING

```
PCA9685 PWM Output (50Hz):
┌─────────────────────────────────────┐
│ Period: 20ms (50Hz)                 │
│ Resolution: 4096 steps (12-bit)     │
│                                     │
│ Pulse Width vs Angle:               │
│ 1.0ms  → 0°   (servo full CCW)      │
│ 1.5ms  → 90°  (servo center)        │
│ 2.0ms  → 180° (servo full CW)       │
│                                     │
│ PCA9685 Count Calculation:          │
│ count = (pulse_us / 20000) × 4096   │
│                                     │
│ Example (1.5ms):                    │
│ count = (1500 / 20000) × 4096 = 307│
└─────────────────────────────────────┘

MG90S Servo Specs:
├─ Operating Voltage: 4.8-5.5V
├─ Torque: 1.8kg @ 4.8V, 2.5kg @ 6V
├─ Speed: 60°/0.12s @ 4.8V
├─ Pulse Width: 1000-2000µs
├─ Dead Band: ±5µs
└─ Connector: 3-pin (GND, VCC, Signal)
```

---

## CAMERA DVP INTERFACE

```
OV5640 DVP Parallel Interface:
┌──────────────────────────────────┐
│ Clock Inputs:                    │
│ XCLK: Internal oscillator        │
│ PCLK (GPIO 36): Pixel clock      │
│                                  │
│ Sync Signals:                    │
│ VSYNC (GPIO 34): Frame sync      │
│ HREF (GPIO 35): Line sync        │
│                                  │
│ Data Lines (8-bit):              │
│ D0 (GPIO 37)                     │
│ D1 (GPIO 38)                     │
│ D2 (GPIO 19)                     │
│ D3 (GPIO 20)                     │
│ D4 (GPIO 22)                     │
│ D5 (GPIO 23)                     │
│ D6 (GPIO 24)                     │
│ D7 (GPIO 25)                     │
│                                  │
│ Control (I2C):                   │
│ SDA (GPIO 39): Data              │
│ SCL (GPIO 40): Clock             │
│ Address: 0x30                    │
│                                  │
│ Reset Pins:                      │
│ RESET (GPIO 21): Hardware reset  │
│ PWDN (GPIO 47): Power down       │
└──────────────────────────────────┘
```

---

## DEBUGGING POINTS

```
Logic Analyzer Test Points (3.3V):
├─ I2S Audio: GPIO 4, 5, 6, 7, 15, 16 (monitor I2S frames)
├─ SPI TFT: GPIO 11, 12, 8, 9 (watch display writes)
├─ I2C Servo: GPIO 41, 42 (check PCA9685 communication)
├─ I2C Camera: GPIO 39, 40 (verify OV5640 init)
└─ Status LED: GPIO 48 (blink patterns for debug)

Voltmeter Test Points:
├─ 3.3V rail: Should be stable 3.2-3.4V
├─ 5V rail: Should be stable 4.8-5.2V (before servo peak)
├─ Servo Vcc: Should not drop below 4.8V under load
├─ TFT Vcc: 3.3V ±5%
└─ Audio Vcc: 5V ±2%

Common Issues:
├─ Servos twitch/jitter: Add capacitor across servo Vcc
├─ Camera I2C timeout: Check pull-up resistors
├─ Display flicker: Increase SPI clock frequency or check power
├─ Audio popping: Reduce output volume or add input filter
└─ I2C conflicts: Verify no address collisions (use i2cdetect)
```
