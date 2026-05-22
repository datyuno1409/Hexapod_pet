# 📊 STRUCTURE ANALYSIS & IMPLEMENTATION REPORT

## ✅ PHASE 0: ARCHITECTURE ALIGNMENT (Completed)

### **Learned from xingzhi-cube-1.54tft-wifi Analysis:**

#### **1. Board Pattern (Singleton + Inheritance)**
```
Standard xingzhi Pattern:
├── WifiBoard (abstract base)
│   └── Initialization in constructor:
│       1. I2C/SPI buses
│       2. Peripheral devices (display, buttons, audio)
│       3. Power management
│
│   Methods called by Application::Start():
│   ├── GetAudioCodec()
│   ├── GetDisplay()
│   ├── GetBacklight()
│   └── GetBatteryLevel()

Our Hexapod Pattern:
├── HexapodBotBoard (similar structure)
│   └── Initialization:
│       1. Servo I2C Bus (GPIO 41/42)
│       2. Camera I2C Bus (GPIO 39/40)
│       3. ServoController
│       4. GaitGenerator ← NEW
│       5. AttackPatterns ← NEW
```

#### **2. Component Initialization Order (Critical)**
```
MUST FOLLOW:
Step 1: GPIO + Bus Configuration
  ├─ spi_bus_initialize() or i2c_master_bus_new()
  └─ Returns bus handle for next steps

Step 2: Device Drivers (Low-level)
  ├─ esp_lcd_panel_io_spi() - Display
  ├─ i2c_master_add_device() - PCA9685
  └─ Returns device handles

Step 3: Application Controllers (Mid-level)
  ├─ ServoController::Initialize(bus_handle)
  ├─ GaitGenerator::Initialize(servo_controller)
  └─ AttackPatterns::Initialize(gait_gen, servo_ctrl)

Step 4: Expose via Board Interface
  └─ GetGaitGenerator(), GetAttackPatterns()
```

#### **3. Button/Power/Timer Pattern (xingzhi uses)**
```cpp
// From xingzhi-cube-1.54tft-wifi.cc
InitializeButtons() {
    boot_button_.OnClick([this]() {
        power_save_timer_->WakeUp();
        app.ToggleChatState();
    });
}

InitializePowerSaveTimer() {
    power_save_timer_ = new PowerSaveTimer(-1, 60, 300);
    power_save_timer_->OnEnterSleepMode([this]() {
        GetDisplay()->SetPowerSaveMode(true);
    });
}

// We DON'T need this complexity for hexapod!
// Hexapod is ALWAYS powered, always running motion
```

---

## 🎯 OUR STRUCTURE PLAN

### **Directory Layout**

```
d:/New folder/Hexapod_pet/xiaozhi-esp32-lxdata/main/
│
├── boards/
│   ├── hexapod_voicebot/
│   │   ├── config.h              (GPIO: TFT, audio, buttons)
│   │   ├── config.json
│   │   └── hexapod_voicebot_board.cc
│   │
│   └── hexapod_bot/
│       ├── config.h              (GPIO: servo, camera; gait params)
│       ├── config.json
│       └── hexapod_bot_board.cc
│
├── hexapod_servo_controller.h    (Low-level: PCA9685 PWM)
├── hexapod_servo_controller.cc   (~200 lines)
│   └── Functions:
│       ├── Initialize(i2c_handle)
│       ├── SetServoAngle(id, angle)
│       ├── SetServoAngles(map<id, angle>)
│       ├── GetServoAngle(id)
│       └── Internal: IsPca9685Ready(), WritePwm()
│
├── hexapod_gait_generator.h      ← NEW (Phase 2)
├── hexapod_gait_generator.cc     (~350 lines)
│   └── Functions:
│       ├── Initialize(servo_ctrl)
│       ├── Walk(gait_type, speed, dir, duration)
│       ├── Stop/Pause/Resume()
│       ├── Private: UpdateGaitPhase(), ComputeServoAngles()
│       └── Private: ComputeLegIK()
│
├── gaits/
│   ├── tripod_gait.h             ← NEW
│   ├── ripple_gait.h             ← NEW
│   └── wave_gait.h               ← NEW
│
├── hexapod_attack_patterns.h     ← NEW (Phase 2)
├── hexapod_attack_patterns.cc    (~300 lines)
│   └── Functions:
│       ├── Initialize(gait_gen, servo_ctrl)
│       ├── ExecuteAttack(type, intensity)
│       ├── StopAttack()
│       ├── UpdateFrame(elapsed_ms)
│       └── Private: UpdateStrikeFrame(), UpdateLungeFrame()
│
├── attacks/
│   ├── strike_attack.h           ← NEW
│   └── lunge_attack.h            ← NEW
│
└── hexapod_motion.h/cc           (Existing - high-level behaviors)
```

---

## 📝 FILE DETAILS & FUNCTION BREAKDOWN

### **1. hexapod_servo_controller.h** (Already exists, review structure)

```cpp
#pragma once
#include <vector>
#include <map>
#include <esp_err.h>
#include <driver/i2c_master.h>

class ServoController {
public:
    static ServoController& GetInstance();
    
    // Lifecycle
    bool Initialize(i2c_master_bus_handle_t i2c_bus);
    void Shutdown();
    
    // Servo control
    bool SetServoAngle(uint8_t servo_id, float angle_deg);
    bool SetServoAngles(const std::map<uint8_t, float>& angles);
    float GetServoAngle(uint8_t servo_id) const;
    
    // Status
    bool IsInitialized() const { return initialized_; }
    bool IsPca9685Ready(uint8_t pca_index) const;
    
private:
    ServoController();
    
    // Internal PWM calculation
    // Servo pulse: 1000-2000 µs = 0-180°
    // PCA9685 frequency: 50Hz (20ms period)
    // Resolution: 4096 steps per period
    uint16_t AngleToPwmValue(float angle_deg);
    
    // Hardware I2C communication
    void WritePca9685Register(uint8_t pca_index, uint8_t reg, uint8_t value);
    
    // Members
    i2c_master_bus_handle_t i2c_bus_;
    i2c_master_dev_handle_t pca9685_[2];  // Two devices @ 0x40, 0x41
    
    // Cached servo positions (for smooth updates)
    std::vector<float> current_angles_;
    
    bool initialized_;
};
```

### **2. hexapod_gait_generator.h** (NEW - Phase 2)

```cpp
#pragma once
#include <cstdint>
#include <string>
#include <esp_timer.h>
#include "hexapod_servo_controller.h"

// Forward declarations
struct LegPose;

class GaitGenerator {
public:
    enum GaitType {
        TRIPOD,     // 3 legs swinging, 3 on ground
        RIPPLE,     // 1 leg swinging, 5 on ground
        WAVE,       // Sequential wave motion
    };
    
    enum Direction {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
    };
    
    static GaitGenerator& GetInstance();
    
    // Lifecycle
    bool Initialize(ServoController* servo_ctrl);
    void Shutdown();
    
    // High-level motion commands
    void Walk(GaitType gait, uint8_t speed, Direction dir, uint32_t duration_ms);
    void Stop();
    void Pause();
    void Resume();
    
    // Status queries
    bool IsWalking() const { return is_walking_; }
    GaitType GetCurrentGait() const { return current_gait_; }
    uint32_t GetElapsedTime() const;
    
    // Called by Application main loop (~20Hz)
    void Update();
    
private:
    GaitGenerator();
    
    // Gait phase computation
    // Called every Update() cycle
    void UpdateGaitPhase();
    void ComputeServoAngles();
    
    // Inverse Kinematics - convert foot position to servo angles
    LegPose ComputeLegIK(int leg_id, float phase_0_to_1);
    
    // Gait-specific phase functions
    float GetTripodPhase(float norm_time);  // 0-1 mapping
    float GetRipplePhase(float norm_time);
    float GetWavePhase(float norm_time);
    
    // Animation curve helpers
    float EaseInOutQuad(float t);  // For smooth servo transitions
    float SmoothStep(float t);
    
    // Members
    ServoController* servo_controller_;
    
    // Motion state
    GaitType current_gait_;
    Direction current_dir_;
    uint8_t speed_percent_;        // 0-100
    uint32_t motion_start_time_;   // esp_timer_get_time()
    uint32_t motion_duration_ms_;
    uint32_t gait_cycle_time_ms_;  // TRIPOD:600, RIPPLE:1000, WAVE:1500
    
    bool is_walking_;
    bool is_paused_;
    
    // Cached servo angles (updated each cycle)
    std::map<uint8_t, float> target_angles_;
};

// Kinematics data structure
struct LegPose {
    float coxa;    // Hip rotation: ±45° from center
    float femur;   // Shoulder: 0-180° (forward/back)
    float tibia;   // Knee: 0-180° (down/up)
};
```

### **3. tripod_gait.h** (NEW - Phase 2)

```cpp
#pragma once
#include <cmath>
#include "hexapod_gait_generator.h"

/**
 * Tripod Gait Kinematics
 * 
 * Pattern: 3 legs on ground, 3 swinging
 * Cycle: 600ms (fast but stable)
 * 
 * Phase breakdown:
 * 0-200ms: Legs 0,2,4 swing forward | Legs 1,3,5 push
 * 200-400ms: Transition
 * 400-600ms: Legs 1,3,5 swing forward | Legs 0,2,4 push
 */
namespace gaits {
namespace tripod {

// Determine if a leg is in swing or stance phase
inline bool IsLegSwinging(int leg_id, float phase_0_to_1) {
    // In tripod, half the cycle swings legs 0,2,4 and half swings 1,3,5
    bool first_half = (phase_0_to_1 < 0.5);
    bool leg_in_first_group = (leg_id % 2 == 0);
    
    return first_half == leg_in_first_group;
}

// Compute leg position in tripod gait
inline LegPose ComputeStepPosition(int leg_id, float phase_0_to_1, float step_height) {
    LegPose pose;
    
    bool swinging = IsLegSwinging(leg_id, phase_0_to_1);
    float local_phase = (phase_0_to_1 < 0.5) ? phase_0_to_1 * 2.0f : (phase_0_to_1 - 0.5f) * 2.0f;
    
    if (swinging) {
        // Swing phase: leg moves forward through air
        // Use smooth curve: sine wave for natural motion
        float swing_forward = 20.0f * std::sin(local_phase * M_PI);  // ±20° forward
        float swing_height = 30.0f * std::sin(local_phase * M_PI);   // ±30° height
        
        pose.coxa = 0.0f;      // Center
        pose.femur = 90.0f + swing_forward;
        pose.tibia = 90.0f - swing_height;
    } else {
        // Stance phase: leg on ground, pushes body
        float push_back = -15.0f * std::sin(local_phase * M_PI);  // -15° back
        
        pose.coxa = 0.0f;
        pose.femur = 75.0f + push_back;
        pose.tibia = 105.0f;  // Slightly bent for stability
    }
    
    return pose;
}

} // namespace tripod
} // namespace gaits
```

### **4. hexapod_attack_patterns.h** (NEW - Phase 2)

```cpp
#pragma once
#include <cstdint>
#include <esp_timer.h>
#include "hexapod_gait_generator.h"
#include "hexapod_servo_controller.h"

class AttackPatterns {
public:
    enum AttackType {
        STRIKE,     // Quick jab with front legs
        LUNGE,      // Charge forward
        DODGE,      // Quick sidestep
    };
    
    static AttackPatterns& GetInstance();
    
    // Lifecycle
    bool Initialize(GaitGenerator* gait_gen, ServoController* servo_ctrl);
    void Shutdown();
    
    // Attack execution
    void ExecuteAttack(AttackType attack, uint8_t intensity);
    void StopAttack();
    void CancelAttack();
    
    // Status
    bool IsAttacking() const { return is_attacking_; }
    AttackType GetCurrentAttack() const { return current_attack_; }
    
    // Called by Application main loop (~20Hz)
    void UpdateFrame();
    
private:
    AttackPatterns();
    
    // Attack-specific update functions
    // Each called from UpdateFrame() based on attack type
    void UpdateStrikeFrame();
    void UpdateLungeFrame();
    void UpdateDodgeFrame();
    
    // Animation phase helpers
    // Returns 0.0-1.0 normalized progress
    float GetAttackProgress() const;
    
    // Members
    ServoController* servo_controller_;
    GaitGenerator* gait_generator_;
    
    AttackType current_attack_;
    uint32_t attack_start_time_;  // esp_timer_get_time()
    uint8_t attack_intensity_;     // 0-100
    uint32_t attack_duration_ms_;  // STRIKE:600, LUNGE:1100
    
    bool is_attacking_;
};
```

### **5. strike_attack.h** (NEW - Phase 2)

```cpp
#pragma once
#include <cmath>
#include "hexapod_attack_patterns.h"

/**
 * Strike Attack Animation
 * 
 * Motion: Quick jab with front two legs
 * Duration: 600ms total
 * 
 * Phases:
 * 0-300ms: Prepare (shift weight back)
 * 300-400ms: Strike (fast forward jab)
 * 400-600ms: Recovery (return to neutral)
 */
namespace attacks {
namespace strike {

// Called per-frame (~20Hz) during strike animation
void ComputeStrikeFrame(
    float progress_0_to_1,  // Current position in animation (0.0-1.0)
    uint8_t intensity,      // Attack intensity (0-100)
    std::map<uint8_t, float>& servo_angles  // Output: target servo angles
) {
    // Animation broken into 3 phases
    float prepare_phase = 0.5f;   // 0-300ms = 50% of animation
    float strike_phase = 0.167f;  // 300-400ms = 16.7%
    float recovery_phase = 0.333f; // 400-600ms = 33.3%
    
    if (progress_0_to_1 < prepare_phase) {
        // Phase 1: Prepare (0-300ms)
        // Rear legs bend, front legs raise
        float local_t = progress_0_to_1 / prepare_phase;
        
        // Rear legs (4, 5) bend deeper for stability
        servo_angles[12] = 90.0f + 20.0f * local_t;   // Rear_left coxa
        servo_angles[13] = 60.0f - 20.0f * local_t;   // Rear_left femur
        servo_angles[14] = 120.0f + 20.0f * local_t;  // Rear_left tibia
        
        // Front legs (0, 1) raise up
        servo_angles[0] = 90.0f;
        servo_angles[1] = 90.0f + 30.0f * local_t;    // Rise up
        servo_angles[2] = 90.0f - 30.0f * local_t;    // Knee bend
        
    } else if (progress_0_to_1 < prepare_phase + strike_phase) {
        // Phase 2: Strike (300-400ms)
        // Front legs SNAP forward with high speed
        float local_t = (progress_0_to_1 - prepare_phase) / strike_phase;
        
        // Speed parameter (intensity affects servo speed)
        // intensity 100 = very fast snap
        // intensity 50 = moderate snap
        float intensity_scale = intensity / 100.0f;
        
        // Front legs extend forward in quick motion
        servo_angles[0] = 90.0f + 45.0f * local_t * intensity_scale;  // Coxa snap
        servo_angles[1] = 120.0f + 40.0f * std::pow(local_t, 2.0f);   // Femur extend
        servo_angles[2] = 60.0f - 40.0f * std::pow(local_t, 2.0f);    // Tibia snap
        
    } else {
        // Phase 3: Recovery (400-600ms)
        float local_t = (progress_0_to_1 - prepare_phase - strike_phase) / recovery_phase;
        
        // Return all legs to neutral smoothly
        servo_angles[0] = 90.0f + 45.0f * intensity_scale * (1.0f - local_t);
        servo_angles[1] = 90.0f + 30.0f * (1.0f - local_t);
        servo_angles[2] = 90.0f - 30.0f * (1.0f - local_t);
        
        servo_angles[12] = 90.0f + 20.0f * (1.0f - local_t);
        servo_angles[13] = 60.0f - 20.0f * (1.0f - local_t);
        servo_angles[14] = 120.0f + 20.0f * (1.0f - local_t);
    }
}

} // namespace strike
} // namespace attacks
```

---

## 🧪 TESTING STRATEGY (Step-by-step)

### **Test 1: Servo Hardware Verification**
```
Goal: Verify all 18 servos respond to commands
Steps:
1. Build hexapod_bot board with ServoController only
2. Run: ServoController::Initialize()
3. Sweep each servo 0→180→0
   - Visually verify motion
   - Check for i2c_detect finding 0x40, 0x41
4. Expected: All 18 servos move smoothly

Success Criteria:
☐ All servos move
☐ No servo jitter/stuttering
☐ I2C addresses detected on bus
☐ Servo angles match command (within ±5°)
```

### **Test 2: Gait Generator - Phase Calculation**
```
Goal: Verify gait phase computation is correct
Steps:
1. Build with GaitGenerator but no attack patterns
2. Enable ESP_LOGI debug output for gait phase
3. Run: GaitGenerator::Walk(TRIPOD, 50, FORWARD, 3000)
4. Log servo angles every 50ms
5. Verify tripod pattern: legs 0,2,4 swing while 1,3,5 push

Output expected:
Time=0ms:   Legs 0,2,4 moving (swing), Legs 1,3,5 stationary (push)
Time=300ms: Transition phase
Time=600ms: Pattern reverses

Success Criteria:
☐ Phase calculation matches expected cycle time
☐ Leg coordination correct (tripod pattern visible)
☐ No servo glitching during transitions
☐ Robot can stand and shift weight smoothly
```

### **Test 3: Strike Attack Animation**
```
Goal: Verify strike animation plays correctly
Steps:
1. Place robot on table (safe testing area)
2. Build with AttackPatterns
3. Run: AttackPatterns::ExecuteAttack(STRIKE, 100)
4. Observe: Rear legs brace → Front legs jab → Recovery
5. Repeat with intensity 50, 25 to verify scaling

Expected motion:
0-300ms: Rear legs bend, body lowers
300-400ms: Front legs SNAP forward (fast, intentional)
400-600ms: Return to neutral

Success Criteria:
☐ Motion sequence matches description
☐ Strike phase is noticeably faster than others
☐ Rear legs stay stable (not moving during strike)
☐ Recovery is smooth back to neutral
☐ Animation repeatable (no servo errors)
```

### **Test 4: Lunge Attack Animation**
```
Goal: Verify lunge moves robot forward
Steps:
1. Place robot on floor with clear space (30cm forward)
2. Run: AttackPatterns::ExecuteAttack(LUNGE, 80)
3. Measure: Distance traveled
4. Repeat with intensity 50, 30 to check scaling
5. Verify: Robot doesn't tip, motion is controlled

Expected motion:
0-400ms: Forward stepping tripod gait
400-700ms: High speed lunge
700-800ms: Sharp stop
800-1100ms: Return to standing

Success Criteria:
☐ Robot moves forward 5-10cm (for intensity 80)
☐ Motion is stable (no tipping)
☐ Distance scales with intensity
☐ Stops on command when needed
☐ Can resume standing position
```

### **Test 5: MCP Tool Integration**
```
Goal: Verify WebSocket commands trigger gait/attacks
Steps:
1. VoiceBot & HexapodBot on same Wi-Fi
2. Send MCP command: hexapod.walk("tripod", 50, "forward")
3. Verify: HexapodBot executes tripod gait
4. Send: hexapod.attack("strike", 100)
5. Verify: Robot performs strike animation

Success Criteria:
☐ Commands received via WebSocket
☐ Motion executes as intended
☐ Response sent back to VoiceBot
☐ Latency < 100ms start-to-motion
☐ Can chain commands (walk, strike, walk again)
```

---

## 📋 IMPLEMENTATION CHECKLIST

### **Phase 1: Setup & Verification**
- [ ] Copy xingzhi-cube-1.54tft-wifi structure
- [ ] Review existing hexapod_servo_controller
- [ ] Add gait timing constants to config.h
- [ ] Verify I2C pullup resistors (4.7kΩ on GPIO 41/42)

### **Phase 2: Gait Generator (Days 1-2)**
- [ ] Create hexapod_gait_generator.h/cc
- [ ] Implement tripod_gait.h kinematics
- [ ] Test: Servo phase computation
- [ ] Test: Smooth angle transitions

### **Phase 3: Attack Patterns (Day 3)**
- [ ] Create hexapod_attack_patterns.h/cc
- [ ] Implement strike_attack.h animation
- [ ] Implement lunge_attack.h animation
- [ ] Test: Frame-by-frame animation accuracy

### **Phase 4: Integration (Day 4)**
- [ ] Add InitializeMotionLayer() to hexapod_bot_board.cc
- [ ] Add MCP tools: hexapod.walk, hexapod.attack
- [ ] Test: End-to-end VoiceBot → HexapodBot

### **Phase 5: Polish & Documentation**
- [ ] Performance optimization
- [ ] Error handling & edge cases
- [ ] Final hardware testing
- [ ] User guide creation

---

**Next Step:** Start with Test 1 (Servo Hardware Verification) ✅

