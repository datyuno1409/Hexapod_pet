# 🔌 HƯỚNG DẪN ĐẶT TỤ VÀ RESISTOR - HEXAPOD SYSTEM

## 📋 DANH SÁCH TỤ ĐIỆN (CAPACITORS)

### 🔋 POWER SUPPLY (Tổng quát)

#### **1. Tụ Lọc Chính - 1000µF/10V** ⭐ QUAN TRỌNG
```
FROM: AMS1117 5V Regulator (Output pin)
   ↓
[1000µF/10V Electrolytic]
   ↓
TO: GND (Battery GND)

Vị trí vật lý: Đặt sát cạnh output của AMS1117
Mục đích: Lọc ripple từ regulator, ổn định 5V rail
```

**Chi tiết:**
- **Cực dương (+):** Cắm vào 5V output của regulator
- **Cực âm (-):** Cắm vào GND
- **Đặt gần nhất:** Trong 5cm từ regulator

---

### 🎙️ VOICEBOT (Master ESP32-S3)

#### **2. Tụ Decoupling MAX98357A - 100µF/10V**
```
Chân VDD của MAX98357A (+5V)
   ↓
[100µF/10V Electrolytic]
   ↓
GND (Audio GND)

Vị trí: Ngay bên cạnh chân VDD của MAX98357A (< 5cm)
Mục đích: Ổn định nguồn 5V cho amplifier, giảm noise audio
```

**Chi tiết:**
- **Cực dương (+):** Nối thẳng đến VDD của MAX98357A
- **Cực âm (-):** Nối thẳng đến GND (liên quan đến audio)
- **Đặt gần:** Trên bảng mạch, sát với MAX98357A

---

#### **3. Tụ Bypass MAX98357A - 0.1µF (100nF)**
```
Cùng vị trí với tụ 100µF ở trên:

MAX98357A VDD
   ↓
   ├─[100µF/10V]──→ GND
   └─[0.1µF/10V]──→ GND (song song)

Mục đích: Lọc cao tần, noise EMI từ switching amplifier
```

**Chi tiết:**
- **Loại:** Ceramic capacitor 0.1µF (100nF), 10V
- **Đặt:** Ngay cạnh tụ 100µF
- **Cực dương:** VDD
- **Cực âm:** GND

---

#### **4. Tụ I2S Decoupling - 100µF/10V (INMP441)**
```
Chân VDD của INMP441 (+3.3V)
   ↓
[100µF/10V Electrolytic]
   ↓
GND (Audio GND)

Vị trí: Sát chân VDD của INMP441 (< 5cm)
Mục đích: Ổn định 3.3V cho microphone
```

**Chi tiết:**
- **Cực dương (+):** Đến VDD của INMP441
- **Cực âm (-):** Đến GND
- **Loại:** Electrolytic, 10V rating

---

#### **5. Tụ TFT Display Decoupling - 100µF/10V**
```
Chân VCC của TFT ST7789 (+3.3V)
   ↓
[100µF/10V Electrolytic]
   ↓
GND

Vị trí: Đặt gần đầu nối TFT (< 5cm)
Mục đích: Ổn định cung cấp 3.3V cho display
```

---

### 🤖 HEXAPODBOT (Slave ESP32-S3)

#### **6. Tụ PCA9685 #1 Decoupling - 100µF/10V** ⭐ QUAN TRỌNG
```
PCA9685 #1 (0x40) - Chân VCC (+5V)
   ↓
[100µF/10V Electrolytic]
   ↓
GND (Servo GND)

Vị trí: NGAY CẠNH chân VCC của PCA9685 #1 (< 3cm)
Mục đích: Cấp nguồn ổn định cho 9 servo (FL, FR, ML)
```

**Chi tiết:**
- **Cực dương (+):** VCC của PCA9685 #1
- **Cực âm (-):** GND
- **RẤT QUAN TRỌNG:** Phải đặt gần để giảm inductance của dây

---

#### **7. Tụ PCA9685 #2 Decoupling - 100µF/10V** ⭐ QUAN TRỌNG
```
PCA9685 #2 (0x41) - Chân VCC (+5V)
   ↓
[100µF/10V Electrolytic]
   ↓
GND (Servo GND)

Vị trí: NGAY CẠNH chân VCC của PCA9685 #2 (< 3cm)
Mục đích: Cấp nguồn ổn định cho 9 servo (MR, BL, BR)
```

---

#### **8. Tụ Servo Rail Decoupling - 100µF/10V**
```
5V Servo Power Rail (chỗ servo nối vào)
   ↓
[100µF/10V Electrolytic]
   ↓
GND Rail

Vị trí: Đặt ở gần ĐIỂM CHÍNH nơi servo kết nối (power connector)
Số lượng: Ít nhất 1 cái, tối ưu 2-3 cái nếu servo nối dài
Mục đích: Chống servo twitch, ổn định dòng điện đột ngột
```

**Chi tiết:**
- Nếu servo dây dài (>30cm): thêm 1 tụ ở giữa
- Nếu dây rất dài (>50cm): thêm tụ ở mỗi 20-30cm

---

#### **9. Tụ TFT Display Decoupling - 100µF/10V**
```
TFT ST7735 (0.96" 80×160) - Chân VCC (+3.3V)
   ↓
[100µF/10V Electrolytic]
   ↓
GND

Vị trí: Sát chân VCC của TFT display
Mục đích: Ổn định cung cấp 3.3V cho display nhỏ
```

---

## 📊 BẢNG TÓM TẮT TỤ ĐIỆN

| # | Vị trí | Giá trị | Loại | Điện áp | Mục đích |
|----|--------|--------|------|---------|----------|
| 1 | AMS1117 output | 1000µF | Electrolytic | 10V | Lọc chính 5V |
| 2 | MAX98357A VCC | 100µF | Electrolytic | 10V | Amplifier power |
| 3 | MAX98357A VCC | 0.1µF | Ceramic | 10V | Bypass cao tần |
| 4 | INMP441 VDD | 100µF | Electrolytic | 10V | Microphone |
| 5 | TFT 240×240 VCC | 100µF | Electrolytic | 10V | Display |
| 6 | PCA9685 #1 VCC | 100µF | Electrolytic | 10V | Servo #1 |
| 7 | PCA9685 #2 VCC | 100µF | Electrolytic | 10V | Servo #2 |
| 8 | Servo 5V Rail | 100µF | Electrolytic | 10V | Anti-twitch |
| 9 | TFT 80×160 VCC | 100µF | Electrolytic | 10V | Display nhỏ |

**Tổng:** 9 tụ điện

---

## 🔗 RESISTOR PLACEMENT

### 📍 I2C PULLUP RESISTORS (QUAN TRỌNG)

#### **Hexapod I2C Servo Bus (GPIO 41, 42)**
```
+3.3V
  ├─[4.7kΩ]──→ GPIO 41 (SDA)
  └─[4.7kΩ]──→ GPIO 42 (SCL)
       ↓
   [PCA9685 #1 & #2 trên bus này]
       ↓
      GND

Vị trí: Gần ESP32-S3 (< 10cm)
Mục đích: Kéo I2C line lên HIGH (I2C open-drain)
```

**Chi tiết:**
- **Loại:** Metal film resistor 1/4W
- **Giá trị:** 4.7kΩ (hoặc 10kΩ nếu không có 4.7k)
- **Đặt:** 2 resistor, mỗi cái 1 chân vào GPIO, 1 chân vào +3.3V
- **Đặt gần:** Cách ESP32 < 10cm

---

### 🔘 BUTTON PULLDOWN/PULLUP RESISTORS (Optional)

#### **Nút WAKE UP (GPIO 0)**
```
GPIO 0
  ↓
[10kΩ]──→ GND
  ↓
Nút bấm ──→ +3.3V (hoặc GND)

Vị trí: Sát chân GPIO 0 của ESP32
```

**Chi tiết:**
- **Loại:** 1/4W resistor
- **Giá trị:** 10kΩ (tiêu chuẩn)
- **Mục đích:** Debouncing + pull-down (tùy cấu hình firmware)
- **Ghi chú:** ESP32 có internal pull-up/down, resistor này là optional nhưng khuyến nghị

---

#### **Nút VOL UP (GPIO 3) & VOL DOWN (GPIO 18)** (Optional)
```
GPIO 3 / GPIO 18
        ↓
    [10kΩ]──→ GND
        ↓
   Nút bấm ──→ GND
```

**Chi tiết:**
- **Loại:** 1/4W resistor
- **Giá trị:** 10kΩ
- **Số lượng:** 2 cái (một cho VOL UP, một cho VOL DOWN)
- **Optional:** Có thể bỏ qua nếu dùng internal pull-up

---

### 🔊 AUDIO OUTPUT RESISTOR (MAX98357A)

#### **OUT- Biasing Resistor - 10kΩ**
```
MAX98357A
  ├─ OUT+ ──→ Speaker +
  │
  └─ OUT- ──┬──[10kΩ]──→ GND
            │
            └──→ Speaker -

Vị trí: Gần chân OUT- của MAX98357A (< 5cm)
```

**Chi tiết:**
- **Loại:** 1/4W carbon/metal film resistor
- **Giá trị:** 10kΩ (tiêu chuẩn cho 8Ω speaker)
- **Mục đích:** Biasing DC offset cho output
- **Ghi chú:** Nếu dùng transformer coupling, có thể bỏ

---

## 📊 BẢNG TÓM TẮT RESISTOR

| # | Vị trí | Giá trị | Loại | Mục đích |
|----|--------|--------|------|----------|
| 1 | GPIO 41 → +3.3V | 4.7kΩ | 1/4W | I2C pullup (SDA) |
| 2 | GPIO 42 → +3.3V | 4.7kΩ | 1/4W | I2C pullup (SCL) |
| 3 | GPIO 0 → GND | 10kΩ | 1/4W | Button debounce (WAKE UP) |
| 4 | GPIO 3 → GND | 10kΩ | 1/4W | Button debounce (VOL UP) |
| 5 | GPIO 18 → GND | 10kΩ | 1/4W | Button debounce (VOL DOWN) |
| 6 | MAX98357A OUT- | 10kΩ | 1/4W | Speaker biasing |

**Tổng:** 6 resistor

---

## 🎯 DANH SÁCH MUA SẮM

### Tụ Điện (Capacitors)
```
☐ 1000µF/10V Electrolytic      × 1 (main filter)
☐ 100µF/10V Electrolytic       × 7 (PCA9685, MAX98357A, servo rails, TFT)
☐ 0.1µF/10V (100nF) Ceramic    × 1 (MAX98357A bypass)

Tổng: ~9 tụ
```

### Resistor
```
☐ 4.7kΩ 1/4W Metal Film        × 2 (I2C pullup)
☐ 10kΩ 1/4W Carbon/Metal Film   × 4 (buttons + audio)

Tổng: ~6 resistor
```

---

## 📐 WIRING DIAGRAM - TỤ & RESISTOR

### VOICEBOT Power Distribution
```
Battery 7.4V
    ↓
[AMS1117 5V Regulator]
    ↓
    ├──[1000µF/10V]──→ GND    ← MAIN FILTER
    ↓
5V Rail (chia thành)
    ├─→ MAX98357A ──[100µF/10V]──→ GND
    │                └─[0.1µF]──→ GND (parallel)
    │
    ├─→ TFT ────────[100µF/10V]──→ GND
    │
    └─→ INMP441 ────[100µF/10V]──→ GND (3.3V)

I2C Bus (3.3V)
    ├─[4.7kΩ]──→ GPIO 39 (SDA)
    └─[4.7kΩ]──→ GPIO 40 (SCL)
```

### HEXAPODBOT Power Distribution
```
Battery 7.4V
    ↓
[AMS1117 5V Regulator]
    ↓
    ├──[1000µF/10V]──→ GND    ← MAIN FILTER
    ↓
5V Rail (servo power)
    ├─→ PCA9685 #1 ──[100µF/10V]──→ GND
    ├─→ PCA9685 #2 ──[100µF/10V]──→ GND
    ├─→ Servo Rail ───[100µF/10V]──→ GND (chia điểm)
    │
    ├─→ 18x Servo (dùng 5V rail này)
    │

I2C Bus Servo (3.3V)
    ├─[4.7kΩ]──→ GPIO 41 (SDA)
    └─[4.7kΩ]──→ GPIO 42 (SCL)

3.3V Rail
    └─→ TFT ────────[100µF/10V]──→ GND
```

---

## ✅ CHECKLIST - CÒN THIẾU GÌ KHÔNG

### Capacitors
- [ ] 1000µF/10V main filter (AMS1117)
- [ ] 100µF/10V × 7 cái (MAX98357A, INMP441, TFT, PCA9685×2, servo rail, TFT nhỏ)
- [ ] 0.1µF ceramic × 1 (MAX98357A bypass)

### Resistors
- [ ] 4.7kΩ × 2 (I2C pullup)
- [ ] 10kΩ × 4 (buttons + audio)

### Optional nhưng Khuyến Nghị
- [ ] 10kΩ × 3 (button debouncing - nếu dùng firmware support)
- [ ] Thêm 100µF ở servo rail giữa nếu dây servo > 1m

---

## 🔍 TROUBLESHOOTING - THIẾU TỤ/RESISTOR

| Triệu chứng | Nguyên nhân có thể | Giải pháp |
|------------|-------------------|----------|
| Servo twitch/rung lắc | Thiếu tụ servo | Thêm 100µF ở servo rail |
| I2C không detect (0x40, 0x41) | Thiếu pullup resistor | Thêm 4.7kΩ pullup |
| Audio bị hú/noise | Thiếu tụ MAX98357A | Thêm 100µF + 0.1µF |
| Nút bấm không ổn định | Thiếu debouncing | Thêm 10kΩ pulldown |
| Display bị giật | Thiếu tụ TFT | Thêm 100µF ở VCC |
| ESP32 reboot random | Thiếu tụ chính | Thêm 1000µF ở regulator |

---

## 🛒 DANH SÁCH MUA - ALIEXPRESS/LAZADA

**Electrolytic Capacitors Kit:**
- 1000µF/10V, 100µF/10V mix (tìm "electrolytic capacitor assortment")

**Ceramic Capacitors:**
- 0.1µF/10V ceramic (tìm "ceramic capacitor 100nF")

**Resistor Kit:**
- 4.7kΩ, 10kΩ (tìm "resistor assortment 1/4W")

---

**Hết tất cả! Bây giờ bạn biết chính xác cái nào nằm ở chỗ nào rồi! 🎉**
