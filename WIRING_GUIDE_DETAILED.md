# ðŸ”Œ HÆ¯á»šNG DáºªN Ná»I CHI TIáº¾T - HEXAPOD ROBOT SYSTEM

## ðŸ“‹ DANH SÃCH THIáº¾T Bá»Š Äáº¦Y Äá»¦

```
VOICEBOT (Master - Äiá»u khiá»ƒn chÃ­nh):
â”œâ”€ ESP32-S3 N16R8 (khÃ´ng cÃ³ camera)
â”œâ”€ TFT ST7789 1.54" 240Ã—240 (mÃ n hÃ¬nh chÃ­nh)
â”œâ”€ INMP441 (microphone I2S)
â”œâ”€ MAX98357A (amplifier I2S)
â”œâ”€ NÃºt báº¥m Ã— 3 (Boot, Vol+, Vol-)
â””â”€ DÃ¢y ná»‘i power/ground

HEXAPODBOT (Slave - Äiá»u khiá»ƒn robot):
â”œâ”€ ESP32-S3 N16R8 + Camera 5MP (tÃ­ch há»£p)
â”œâ”€ TFT ST7735 0.96" 80Ã—160 (mÃ n hÃ¬nh status)
â”œâ”€ PCA9685 Ã— 2 (servo controller)
â”œâ”€ Servo MG90S Ã— 18 (chÃ¢n robot)
â”œâ”€ DÃ¢y ná»‘i power/ground
â””â”€ Connector ná»‘i servo

NGUá»’N/POWER:
â”œâ”€ Pin 2S Li-ion 7.4V 3000mAh
â”œâ”€ Máº¡ch AMS1117 5V (giáº£m Ã¡p)
â”œâ”€ Tá»¥ Ä‘iá»‡n 1000ÂµF/10V (bá»™ lá»c)
â””â”€ DÃ¢y nguá»“n AWG 16-18
```

---

## ðŸ”Œ PHáº¦N 1: VOICEBOT - MASTER CONTROLLER

### ðŸ“± ESP32-S3 ChÃ¢n ngoÃ i (Pinout)

```
ESP32-S3 N16R8
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚ Top Row (TrÃ¡i sang pháº£i):           â”‚
â”‚ GND, GPIO48, GPIO47, GPIO21, GPIO20 â”‚
â”‚ GPIO19, GPIO18, GPIO17, GPIO16      â”‚
â”‚ GPIO15, GPIO14, GPIO13, GPIO12      â”‚
â”‚ GPIO11, GPIO10, GPIO9, GPIO8        â”‚
â”œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¤
â”‚ USB-C (Giá»¯a)                        â”‚
â”œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¤
â”‚ Bottom Row (Pháº£i sang trÃ¡i):        â”‚
â”‚ GPIO7, GPIO6, GPIO5, GPIO4, GPIO3   â”‚
â”‚ GPIO2, GPIO1, GPIO0, GND, GND       â”‚
â”‚ 3V3, GND (Power)                    â”‚
â”‚ 5V (náº¿u cÃ³)                         â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
```

### ðŸ”§ Ná»I CHI TIáº¾T VOICEBOT

#### **1ï¸âƒ£ TFT ST7789 1.54" 240Ã—240**

```
MÃ n hÃ¬nh TFT (8 chÃ¢n)
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”

ChÃ¢n TFT         â†’  ChÃ¢n ESP32-S3
â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
1. GND           â†’  GND (hÃ ng dÆ°á»›i, bÃªn trÃ¡i)
2. VCC (+3.3V)   â†’  3V3 (hÃ ng dÆ°á»›i, pháº£i)
3. SCL (CLK)     â†’  GPIO 12
4. SDA (MOSI)    â†’  GPIO 11
5. RES (RESET)   â†’  GPIO 18
6. DC            â†’  GPIO 8
7. CS            â†’  GPIO 9
8. BLK           â†’  GPIO 46
(MISO GPIO 13 - khÃ´ng cáº§n)

DÃ¢y ná»‘i:
â”œâ”€ DÃ¢y Ä‘en: GND
â”œâ”€ DÃ¢y Ä‘á»: 3V3
â”œâ”€ DÃ¢y cam: GPIO 12
â”œâ”€ DÃ¢y vÃ ng: GPIO 11
â”œâ”€ DÃ¢y xanh dÆ°Æ¡ng: GPIO 8
â”œâ”€ DÃ¢y xanh lÃ¡: GPIO 9
â”œâ”€ DÃ¢y tÃ­m: GPIO 46
â””â”€ DÃ¢y há»“ng: GPIO 18 (RST)
```

#### **2ï¸âƒ£ INMP441 Microphone (I2S)**

```
INMP441 Microphone (5 chÃ¢n)
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”

ChÃ¢n INMP441      â†’  ChÃ¢n ESP32-S3
â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
1. GND            â†’  GND
2. L/R            â†’  GND (mono mode)
3. WS             â†’  GPIO 4
4. SCK            â†’  GPIO 5
5. SD             â†’  GPIO 6
6. VDD (+3.3V)    â†’  3V3

DÃ¢y ná»‘i:
â”œâ”€ DÃ¢y Ä‘en: GND
â”œâ”€ DÃ¢y Ä‘á»: 3V3
â”œâ”€ DÃ¢y cam: GPIO 4 (WS)
â”œâ”€ DÃ¢y vÃ ng: GPIO 5 (SCK)
â””â”€ DÃ¢y xanh: GPIO 6 (SD)
```

#### **3ï¸âƒ£ MAX98357A Amplifier (I2S)**

```
MAX98357A (Loa khuáº¿ch Ä‘áº¡i) - 10 chÃ¢n
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”

ChÃ¢n MAX98357A   â†’  ChÃ¢n ESP32-S3
â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
1. GND           â†’  GND
2. DIN           â†’  GPIO 7
3. BCLK          â†’  GPIO 15
4. LRCLK         â†’  GPIO 16
5. SD (enable)   â†’  3V3 (luÃ´n báº­t)
6. GAIN          â†’  GND (24dB)
7. VDD (+5V)     â†’  +5V (tá»« regulat)
8-9. OUT- & OUT+ â†’  Loa 8Î©

DÃ¢y ná»‘i:
â”œâ”€ DÃ¢y Ä‘en: GND
â”œâ”€ DÃ¢y Ä‘á»: 5V
â”œâ”€ DÃ¢y cam: GPIO 7 (DIN)
â”œâ”€ DÃ¢y vÃ ng: GPIO 15 (BCLK)
â”œâ”€ DÃ¢y xanh: GPIO 16 (LRCLK)
â””â”€ DÃ¢y tÃ­m: 3V3 (SD enable)

Loa:
â”œâ”€ ChÃ¢n + (tráº¯ng/Ä‘á») â†’ OUT+ (MAX98357A chÃ¢n 8)
â””â”€ ChÃ¢n - (Ä‘en)      â†’ OUT- (MAX98357A chÃ¢n 9)
```

#### **4ï¸âƒ£ NÃºt Báº¥m Ã— 3**

```
NÃºt Báº¥m (Button)
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”

NÃºt 1: WAKE UP
â”œâ”€ ChÃ¢n 1 â†’ GPIO 0
â””â”€ ChÃ¢n 2 â†’ GND (qua resistor 10kÎ© optional)

NÃºt 2: Volume UP
â”œâ”€ ChÃ¢n 1 â†’ GPIO 3
â””â”€ ChÃ¢n 2 â†’ GND

NÃºt 3: Volume DOWN
â”œâ”€ ChÃ¢n 1 â†’ GPIO 17
â””â”€ ChÃ¢n 2 â†’ GND

DÃ¢y ná»‘i:
â”œâ”€ NÃºt 1 xanh: GPIO 0 (WAKE UP)
â”œâ”€ NÃºt 2 vÃ ng: GPIO 3 (VOL UP)
â”œâ”€ NÃºt 3 cam: GPIO 17 (VOL DOWN)
â””â”€ Táº¥t cáº£ GND: GND chung
```

#### **5ï¸âƒ£ Power Supply (VOICEBOT)**

```
Tá»« Battery 7.4V â†’ AMS1117 5V Regulator

AMS1117 5V (3 chÃ¢n)
â”œâ”€ ChÃ¢n 1 (IN):  7.4V tá»« battery
â”œâ”€ ChÃ¢n 2 (GND): GND
â””â”€ ChÃ¢n 3 (OUT): 5V

Tá»¥ Ä‘iá»‡n lá»c:
â”œâ”€ 1000ÂµF/10V (cá»±c dÆ°Æ¡ng) â†’ 5V output
â”œâ”€ Cá»±c Ã¢m â†’ GND
â””â”€ 100ÂµF/10V (cá»±c dÆ°Æ¡ng) â†’ 5V output (gáº§n MAX98357A)

3V3 tá»« ESP32:
â”œâ”€ Tá»« chÃ¢n 3V3 ESP32
â”œâ”€ Cung cáº¥p cho: TFT, INMP441, MAX98357A (pin SD)
â””â”€ Tá»¥ 100ÂµF/10V (náº¿u cáº§n)

GND chung:
â””â”€ Táº¥t cáº£ GND ná»‘i chung (battery, regulat, ESP32, linh kiá»‡n)
```

---

## ðŸ¤– PHáº¦N 2: HEXAPODBOT - SLAVE CONTROLLER

### ðŸ“± ESP32-S3 vá»›i Camera tÃ­ch há»£p

```
ESP32-S3 N16R8 + Camera 5MP (tÃ­ch há»£p)
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚ Camera Ä‘Ã£ náº±m trÃªn máº¡ch - KHÃ”NG Cáº¦N â”‚
â”‚ ná»‘i DVP pins hoáº·c I2C camera!       â”‚
â”‚ âœ“ Tá»± Ä‘á»™ng Ä‘Æ°á»£c cáº¥p nguá»“n           â”‚
â”‚ âœ“ Data pins Ä‘Ã£ káº¿t ná»‘i             â”‚
â”‚ âœ“ Chá»‰ cáº§n I2C bus vÃ  reset pin    â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
```

### ðŸ”§ Ná»I CHI TIáº¾T HEXAPODBOT

#### **1ï¸âƒ£ TFT ST7735 0.96" 80Ã—160 (Status Display)**

```
MÃ n hÃ¬nh TFT nhá» (8 chÃ¢n)
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”

ChÃ¢n TFT         â†’  ChÃ¢n ESP32-S3
â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
1. GND           â†’  GND
2. VCC (+3.3V)   â†’  3V3
3. SCL (CLK)     â†’  GPIO 1
4. SDA (MOSI)    â†’  GPIO 50
5. RES (RESET)   â†’  GPIO 11
6. DC            â†’  GPIO 10
7. CS            â†’  GPIO 3
8. BLK           â†’  GPIO 14
(MISO GPIO 2 - khÃ´ng báº¯t buá»™c)

DÃ¢y ná»‘i:
â”œâ”€ DÃ¢y Ä‘en: GND
â”œâ”€ DÃ¢y Ä‘á»: 3V3
â”œâ”€ DÃ¢y cam: GPIO 1 (CLK)
â”œâ”€ DÃ¢y vÃ ng: GPIO 50 (MOSI/SDA)
â”œâ”€ DÃ¢y xanh dÆ°Æ¡ng: GPIO 10 (DC)
â”œâ”€ DÃ¢y xanh lÃ¡: GPIO 3 (CS)
â”œâ”€ DÃ¢y tÃ­m: GPIO 11 (RST)
â””â”€ DÃ¢y há»“ng: GPIO 14 (BL)
```

#### **2ï¸âƒ£ PCA9685 Servo Controller #1 (Äá»ƒ Ä‘iá»u khiá»ƒn servo 0-8)**

```
PCA9685 #1 I2C (6 chÃ¢n I2C, 16 chÃ¢n OUT)
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”

I2C Connections:
ChÃ¢n PCA9685      â†’  ChÃ¢n ESP32-S3
â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
GND               â†’  GND
VCC (+5V)         â†’  +5V (tá»« regulator)
SDA               â†’  GPIO 41
SCL               â†’  GPIO 42
A0 (Address)      â†’  GND (Ä‘á»ƒ Ä‘á»‹a chá»‰ = 0x40)
A1 (Address)      â†’  GND
A2 (Address)      â†’  GND

Power Filter:
â”œâ”€ 100ÂµF/10V (cá»±c +) â†’ VCC
â””â”€ Cá»±c - â†’ GND

Servo Outputs (9 servo):
OUT0 â†’ Servo 0 (FL_COXA)
OUT1 â†’ Servo 1 (FL_FEMUR)
OUT2 â†’ Servo 2 (FL_TIBIA)
OUT3 â†’ Servo 3 (FR_COXA)
OUT4 â†’ Servo 4 (FR_FEMUR)
OUT5 â†’ Servo 5 (FR_TIBIA)
OUT6 â†’ Servo 6 (ML_COXA)
OUT7 â†’ Servo 7 (ML_FEMUR)
OUT8 â†’ Servo 8 (ML_TIBIA)

DÃ¢y ná»‘i:
â”œâ”€ DÃ¢y Ä‘en: GND
â”œâ”€ DÃ¢y Ä‘á»: 5V (+ 100ÂµF tá»¥)
â”œâ”€ DÃ¢y xanh: GPIO 41 (SDA)
â”œâ”€ DÃ¢y vÃ ng: GPIO 42 (SCL)
â””â”€ Address pins: GND
```

#### **3ï¸âƒ£ PCA9685 Servo Controller #2 (Äá»ƒ Ä‘iá»u khiá»ƒn servo 9-17)**

```
PCA9685 #2 I2C (6 chÃ¢n I2C, 16 chÃ¢n OUT)
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”

I2C Connections (CÃ™NG BUS vá»›i #1):
ChÃ¢n PCA9685      â†’  ChÃ¢n ESP32-S3
â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
GND               â†’  GND
VCC (+5V)         â†’  +5V (tá»« regulator)
SDA               â†’  GPIO 41 (CÃ™NG vá»›i #1)
SCL               â†’  GPIO 42 (CÃ™NG vá»›i #1)
A0 (Address)      â†’  +5V (Ä‘á»ƒ Ä‘á»‹a chá»‰ = 0x41) â† KHÃC vá»›i #1
A1 (Address)      â†’  GND
A2 (Address)      â†’  GND

Power Filter:
â”œâ”€ 100ÂµF/10V (cá»±c +) â†’ VCC
â””â”€ Cá»±c - â†’ GND

Servo Outputs (9 servo):
OUT0 â†’ Servo 9 (MR_COXA)
OUT1 â†’ Servo 10 (MR_FEMUR)
OUT2 â†’ Servo 11 (MR_TIBIA)
OUT3 â†’ Servo 12 (BL_COXA)
OUT4 â†’ Servo 13 (BL_FEMUR)
OUT5 â†’ Servo 14 (BL_TIBIA)
OUT6 â†’ Servo 15 (BR_COXA)
OUT7 â†’ Servo 16 (BR_FEMUR)
OUT8 â†’ Servo 17 (BR_TIBIA)

DÃ¢y ná»‘i:
â”œâ”€ DÃ¢y Ä‘en: GND
â”œâ”€ DÃ¢y Ä‘á»: 5V (+ 100ÂµF tá»¥)
â”œâ”€ DÃ¢y xanh: GPIO 41 (SDA) - CHUNG vá»›i #1
â”œâ”€ DÃ¢y vÃ ng: GPIO 42 (SCL) - CHUNG vá»›i #1
â””â”€ A0 pin: 5V (Ä‘á»ƒ set address 0x41)
```

#### **4ï¸âƒ£ Servo MG90S Ã— 18 (ChÃ¢n robot)**

```
Má»—i Servo MG90S (3 dÃ¢y)
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”

DÃ¢y servo:
â”œâ”€ DÃ¢y nÃ¢u/Ä‘en: GND
â”œâ”€ DÃ¢y Ä‘á»: +5V
â””â”€ DÃ¢y vÃ ng/tráº¯ng: Signal (PWM)

Ná»‘i:
â”œâ”€ GND: Táº¥t cáº£ servo GND â†’ GND chung (rail 5V)
â”œâ”€ VCC: Táº¥t cáº£ servo +5V â†’ +5V chung (rail 5V, cáº§n tá»¥ Ä‘iá»‡n á»Ÿ Ä‘áº§u)
â””â”€ Signal:
   â”œâ”€ Servo 0-8: PCA9685 #1 OUT0-8
   â”œâ”€ Servo 9-17: PCA9685 #2 OUT0-8

VÃ­ dá»¥ Servo 0 (FL_COXA):
â”œâ”€ NÃ¢u â†’ GND rail
â”œâ”€ Äá» â†’ 5V rail (cáº§n tá»¥ 100ÂµF gáº§n)
â””â”€ VÃ ng â†’ PCA9685 #1 OUT0

VÃ­ dá»¥ Servo 15 (BR_COXA):
â”œâ”€ NÃ¢u â†’ GND rail
â”œâ”€ Äá» â†’ 5V rail
â””â”€ VÃ ng â†’ PCA9685 #2 OUT6
```

#### **5ï¸âƒ£ Camera 5MP (TÃ­ch há»£p trÃªn máº¡ch)**

```
Camera OV5640 (TÃ­ch há»£p sáºµn)
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
âœ“ KHÃ”NG Cáº¦N Ná»I gÃ¬ cáº£!
âœ“ Tá»± Ä‘á»™ng Ä‘Æ°á»£c cáº¥p nguá»“n 3.3V
âœ“ Data lines Ä‘Ã£ káº¿t ná»‘i bÃªn trong

Chá»‰ cáº§n:
â”œâ”€ Äáº£m báº£o pin 3V3 cá»§a ESP32 á»•n Ä‘á»‹nh
â””â”€ KhÃ´ng bá»‹ ká»³ kÃ­ch hoáº·c ngáº¯n máº¡ch
```

#### **6ï¸âƒ£ Power Supply (HEXAPODBOT)**

```
Tá»« Battery 7.4V â†’ AMS1117 5V Regulator

5V Rail (Cáº¥p cho PCA9685 + 18 servo):
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
Regulator 5V output
â”œâ”€ Tá»¥ 1000ÂµF/10V (cá»±c dÆ°Æ¡ng) â†’ 5V
â”œâ”€ Cá»±c Ã¢m â†’ GND
â”œâ”€ 100ÂµF/10V gáº§n PCA9685 #1
â”œâ”€ 100ÂµF/10V gáº§n PCA9685 #2
â””â”€ 100ÂµF/10V á»Ÿ Ä‘áº§u servo rail

3V3 Rail (Tá»« ESP32 LDO):
â”œâ”€ Cáº¥p cho: TFT display nhá»
â”œâ”€ Camera tÃ­ch há»£p (tá»± Ä‘á»™ng)
â””â”€ Tá»¥ 100ÂµF/10V (náº¿u cáº§n)

GND Chung:
â””â”€ Táº¥t cáº£ GND ná»‘i chung (battery, regulator, ESP32, PCA9685, servo)

âš ï¸ LÆ°u Ã½ Power:
â”œâ”€ Servo peak current: 3-4A
â”œâ”€ Average current: 2A
â”œâ”€ DÃ¢y AWG 16-18 cho rail 5V
â”œâ”€ DÃ¢y AWG 18-20 cho GND
â””â”€ Servo GND PHáº¢I ná»‘i trá»±c tiáº¿p vá» battery GND (khÃ´ng qua regulator)
```

---

## ðŸ”— PHáº¦N 3: Káº¾T Ná»I GIá»®A 2 BOARD

### Wi-Fi Connection (KhÃ´ng dÃ¢y)

```
VoiceBot â†â†’ HexapodBot
   â†“             â†“
Wi-Fi Network (cÃ¹ng SSID)

VoiceBot: Client
â””â”€ Káº¿t ná»‘i Ä‘áº¿n HexapodBot @ 192.168.1.101:8081

HexapodBot: Server
â”œâ”€ Láº¯ng nghe port 8081
â””â”€ Nháº­n WebSocket JSON commands
```

---

## ðŸ“Š Báº¢NG TÃ“M Táº®T Ná»I

### VOICEBOT (Master)

| Thiáº¿t bá»‹ | GPIO/Pin | Loáº¡i | Má»¥c Ä‘Ã­ch |
|---------|----------|------|---------|
| TFT 240Ã—240 | GPIO 8,9,11,12,46 | SPI | Main display |
| INMP441 | GPIO 4,5,6 | I2S | Microphone input |
| MAX98357A | GPIO 7,15,16 | I2S | Speaker output |
| Button BOOT | GPIO 0 | Digital | Wake/Chat toggle |
| Button VOL+ | GPIO 3 | Digital | Volume up |
| Button VOL- | GPIO 4 | Digital | Volume down |
| Status LED | GPIO 48 | Digital | Debug blink |
| 3V3 Power | 3V3 | Power | Logic supply |
| 5V Power | 5V | Power | Amplifier supply |
| GND | GND | Power | Ground |

### HEXAPODBOT (Slave)

| Thiáº¿t bá»‹ | GPIO/Pin | Loáº¡i | Má»¥c Ä‘Ã­ch |
|---------|----------|------|---------|
| TFT 80Ã—160 | GPIO 1,50,10,3,11,14 | SPI | Status display |
| PCA9685 #1 | GPIO 41,42 | I2C | Servo 0-8 control @0x40 |
| PCA9685 #2 | GPIO 41,42 | I2C | Servo 9-17 control @0x41 |
| Servo 0-8 | OUT0-8 | PWM | Front + Middle left legs |
| Servo 9-17 | OUT0-8 | PWM | Middle + Back right legs |
| Camera 5MP | (TÃ­ch há»£p) | DVP | Image capture |
| Status LED | GPIO 48 | Digital | Debug blink |
| 3V3 Power | 3V3 | Power | Logic + Camera |
| 5V Power | 5V | Power | PCA9685 + Servo |
| GND | GND | Power | Ground |

---

## ðŸ› ï¸ DANH SÃCH DÃ‚Y Ná»I Cáº¦N CHUáº¨N Bá»Š

```
VOICEBOT:
â”œâ”€ DÃ¢y Dupont 20cm Ã— 8 (TFT display)
â”œâ”€ DÃ¢y Dupont 20cm Ã— 5 (Microphone)
â”œâ”€ DÃ¢y Dupont 20cm Ã— 6 (Amplifier)
â”œâ”€ DÃ¢y Dupont 10cm Ã— 6 (NÃºt báº¥m)
â”œâ”€ DÃ¢y AWG 18 Ã— 1m (5V power)
â””â”€ DÃ¢y AWG 18 Ã— 1m (GND)

HEXAPODBOT:
â”œâ”€ DÃ¢y Dupont 20cm Ã— 6 (TFT display nhá»)
â”œâ”€ DÃ¢y Dupont 20cm Ã— 2 (PCA9685 #1 I2C)
â”œâ”€ DÃ¢y Dupont 20cm Ã— 2 (PCA9685 #2 I2C)
â”œâ”€ DÃ¢y Dupont 15cm Ã— 18 (Servo signal)
â”œâ”€ DÃ¢y AWG 16 Ã— 2m (5V servo power rail)
â”œâ”€ DÃ¢y AWG 18 Ã— 2m (GND servo rail)
â””â”€ Connector JST 3-pin Ã— 18 (Servo connectors)

Linh kiá»‡n Ä‘iá»‡n:
â”œâ”€ Tá»¥ 1000ÂµF/10V Ã— 1 (Bá»™ lá»c 5V chÃ­nh)
â”œâ”€ Tá»¥ 100ÂµF/10V Ã— 3 (Cá»¥c bá»™ PCA9685)
â”œâ”€ Tá»¥ 100ÂµF/10V Ã— 2 (Servo rail)
â”œâ”€ Resistor 10kÎ© Ã— 3 (NÃºt báº¥m - optional)
â””â”€ Resistor 1kÎ© Ã— 2 (Pull-up I2C - optional)
```

---

## âœ… CHECKLIST Ná»I

### VOICEBOT Wiring Checklist

- [ ] **TFT 240Ã—240**
  - [ ] GND â†’ GND
  - [ ] VCC â†’ 3V3
  - [ ] SCL â†’ GPIO 12
  - [ ] SDA â†’ GPIO 11
  - [ ] DC â†’ GPIO 8
  - [ ] CS â†’ GPIO 9
  - [ ] BL â†’ GPIO 46

- [ ] **INMP441 Microphone**
  - [ ] GND â†’ GND
  - [ ] L/R â†’ GND
  - [ ] WS â†’ GPIO 4
  - [ ] SCK â†’ GPIO 5
  - [ ] SD â†’ GPIO 6
  - [ ] VDD â†’ 3V3

- [ ] **MAX98357A Amplifier**
  - [ ] GND â†’ GND
  - [ ] DIN â†’ GPIO 7
  - [ ] BCLK â†’ GPIO 15
  - [ ] LRCK â†’ GPIO 16
  - [ ] SD â†’ 3V3
  - [ ] GAIN â†’ GND
  - [ ] VDD â†’ 5V
  - [ ] OUT+/- â†’ Speaker

- [ ] **Buttons**
  - [ ] WAKE UP â†’ GPIO 0 & GND
  - [ ] VOL+ â†’ GPIO 3 & GND
  - [ ] VOL- â†’ GPIO 17 & GND

- [ ] **Power**
  - [ ] Battery 7.4V â†’ Regulator IN
  - [ ] Regulator OUT â†’ 5V rail
  - [ ] 1000ÂµF tá»¥ lá»c @ 5V
  - [ ] 3V3 tá»« ESP32 â†’ Logic devices
  - [ ] GND chung

### HEXAPODBOT Wiring Checklist

- [ ] **TFT 80Ã—160**
  - [ ] GND â†’ GND
  - [ ] VCC â†’ 3V3
  - [ ] CLK â†’ GPIO 1
  - [ ] SDA -> GPIO 50
  - [ ] DC â†’ GPIO 10
  - [ ] CS â†’ GPIO 3
  - [ ] RST â†’ GPIO 11
  - [ ] BL â†’ GPIO 14

- [ ] **PCA9685 #1 (0x40)**
  - [ ] GND â†’ GND
  - [ ] VCC â†’ 5V (+ 100ÂµF tá»¥)
  - [ ] SDA â†’ GPIO 41
  - [ ] SCL â†’ GPIO 42
  - [ ] A0 â†’ GND
  - [ ] OUT0-8 â†’ Servo 0-8 signal

- [ ] **PCA9685 #2 (0x41)**
  - [ ] GND â†’ GND
  - [ ] VCC â†’ 5V (+ 100ÂµF tá»¥)
  - [ ] SDA â†’ GPIO 41 (CÃ™NG #1)
  - [ ] SCL â†’ GPIO 42 (CÃ™NG #1)
  - [ ] A0 â†’ +5V (khÃ¡c #1)
  - [ ] OUT0-8 â†’ Servo 9-17 signal

- [ ] **18x Servo MG90S**
  - [ ] Táº¥t cáº£ GND â†’ GND rail
  - [ ] Táº¥t cáº£ VCC â†’ 5V rail (+ tá»¥ 100ÂµF)
  - [ ] Servo 0-8 signal â†’ PCA9685 #1 OUT0-8
  - [ ] Servo 9-17 signal â†’ PCA9685 #2 OUT0-8

- [ ] **Power**
  - [ ] Battery 7.4V â†’ Regulator IN
  - [ ] Regulator 5V â†’ 5V rail + 1000ÂµF tá»¥
  - [ ] 3V3 tá»« ESP32 â†’ TFT + cáº£m biáº¿n
  - [ ] GND chung (battery â†’ regulator â†’ táº¥t cáº£)
  - [ ] Servo GND â†’ trá»±c tiáº¿p battery GND

---

## ðŸš¨ Cáº¢NH BÃO QUAN TRá»ŒNG

```
âš ï¸  POWER DISTRIBUTION:
    â”œâ”€ KHÃ”NG ná»‘i 5V trá»±c tiáº¿p tá»« ESP32 (chá»‰ 600mA max)
    â”œâ”€ PHáº¢I dÃ¹ng AMS1117 regulator tá»« battery
    â””â”€ Servo GND pháº£i ná»‘i trá»±c tiáº¿p vá» battery GND

âš ï¸  I2C ADDRESSING:
    â”œâ”€ PCA9685 #1: A0=GND â†’ 0x40
    â”œâ”€ PCA9685 #2: A0=+5V â†’ 0x41
    â””â”€ KHÃC nhau má»›i hoáº¡t Ä‘á»™ng

âš ï¸  SERVO POWER:
    â”œâ”€ 18 servo peak: 3-4A
    â”œâ”€ DÃ¢y AWG 18 lÃ  tá»‘i thiá»ƒu
    â””â”€ Tá»‘t nháº¥t dÃ¹ng AWG 16 cho 5V main rail
```

---

## ðŸ“ GHI CHÃš THÃŠM

### Äiá»ƒm ná»‘i GND quan trá»ng
```
Battery GND
    â†“
Regulator GND
    â†“
â”œâ”€ ESP32 GND
â”œâ”€ TFT GND
â”œâ”€ PCA9685 GND (2x)
â”œâ”€ Servo GND rail
â””â”€ MAX98357A GND

âš ï¸ Táº¥t cáº£ pháº£i ná»‘i chung, KHÃ”NG Ä‘Æ°á»£c cÃ¡ch ly!
```

### Äiá»ƒm ná»‘i 5V quan trá»ng
```
Regulator 5V
    â†“
Tá»¥ 1000ÂµF
    â†“
â”œâ”€ PCA9685 #1 VCC (+ tá»¥ 100ÂµF)
â”œâ”€ PCA9685 #2 VCC (+ tá»¥ 100ÂµF)
â”œâ”€ Servo 5V rail (+ tá»¥ 100ÂµF)
â””â”€ MAX98357A VDD

âš ï¸ KHÃ”NG ná»‘i trá»±c tiáº¿p tá»« battery!
```

### Kiá»ƒm tra trÆ°á»›c khi báº­t power
```
â–¡ GND chung kháº¯p há»‡ thá»‘ng?
â–¡ 5V chá»‰ tá»« regulator, khÃ´ng tá»« ESP32?
â–¡ Servo signal pins khÃ´ng ngáº¯n máº¡ch?
â–¡ Tá»¥ Ä‘iá»‡n Ä‘Æ°á»£c ná»‘i Ä‘Ãºng cá»±c?
â–¡ DÃ¢y power khÃ´ng bá»‹ lá»™n?
â–¡ I2C pullup resistor (náº¿u thÃªm)?
â–¡ Camera pin khÃ´ng bá»‹ ká»³ kÃ­ch?
```

---

**Há»‡ thá»‘ng sáºµn sÃ ng Ä‘á»ƒ láº¯p rÃ¡p! ðŸŽ‰**

