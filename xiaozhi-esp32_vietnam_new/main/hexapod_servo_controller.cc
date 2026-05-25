#include "hexapod_servo_controller.h"
#include "hexapod_constants.h"
#include <esp_log.h>
#include <cmath>
#include <algorithm>
#include <esp_timer.h>
#include <cstring>

#define TAG "ServoController"

// PCA9685 Register addresses
#define PCA9685_MODE1       0x00
#define PCA9685_MODE2       0x01
#define PCA9685_SUBADR1     0x02
#define PCA9685_SUBADR2     0x03
#define PCA9685_SUBADR3     0x04
#define PCA9685_ALLCALLADR  0x05
#define PCA9685_LED0_ON_L   0x06
#define PCA9685_LED0_ON_H   0x07
#define PCA9685_LED0_OFF_L  0x08
#define PCA9685_LED0_OFF_H  0x09
#define PCA9685_LED_ON_L(n)    (4*n+6)
#define PCA9685_LED_OFF_L(n)   (4*n+8)
#define PCA9685_PRESCALE    0xFE
#define PCA9685_TESTMODE    0xFF

// PCA9685 I2C addresses
#define PCA9685_I2C_ADDR_1  HexapodConst::PCA9685_ADDR_1  // 0x40
#define PCA9685_I2C_ADDR_2  HexapodConst::PCA9685_ADDR_2  // 0x41

// ============================================================================
// ServoController Implementation
// ============================================================================

ServoController& ServoController::GetInstance() {
    static ServoController instance;
    return instance;
}

ServoController::ServoController() {
    pca9685_[0] = nullptr;
    pca9685_[1] = nullptr;
    current_angles_.fill(90.0f);  // Initialize all to neutral
}

bool ServoController::Initialize(i2c_master_bus_handle_t i2c_bus_handle) {
    if (i2c_bus_handle == nullptr) {
        ESP_LOGE(TAG, "Invalid I2C bus handle");
        return false;
    }

    i2c_bus_handle_ = i2c_bus_handle;

    // Create I2C device for each PCA9685
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .scl_speed_hz = 50000,  // Giảm xuống 50 kHz để giảm nhiễu cho màn hình SPI
        .scl_wait_us = 0,
    };

    // PCA9685 #1 @ 0x40
    dev_cfg.device_address = PCA9685_I2C_ADDR_1;
    if (i2c_master_bus_add_device(i2c_bus_handle_, &dev_cfg, &pca9685_[0]) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add PCA9685 #1 (0x40) to I2C bus");
        return false;
    }
    ESP_LOGI(TAG, "PCA9685 #1 (0x40) added to I2C bus");

    // PCA9685 #2 @ 0x41
    dev_cfg.device_address = PCA9685_I2C_ADDR_2;
    if (i2c_master_bus_add_device(i2c_bus_handle_, &dev_cfg, &pca9685_[1]) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add PCA9685 #2 (0x41) to I2C bus");
        return false;
    }
    ESP_LOGI(TAG, "PCA9685 #2 (0x41) added to I2C bus");

    // Configure PCA9685 boards. Board #2 is optional during bring-up.
    bool pca1_ok = ConfigurePca9685(0);
    bool pca2_ok = ConfigurePca9685(1);
    if (!pca1_ok) {
        ESP_LOGE(TAG, "PCA9685 #1 is required but failed to configure");
        return false;
    }
    if (!pca2_ok) {
        ESP_LOGW(TAG, "PCA9685 #2 not detected/configured; servos 9-17 will be skipped");
        pca9685_[1] = nullptr;
    }

    // Set all servos to neutral position (90 degrees)
    SetNeutral();

    ESP_LOGI(TAG, "Servo controller initialized successfully");
    return true;
}

bool ServoController::ConfigurePca9685(uint8_t pca_index) {
    if (pca_index >= 2) {
        return false;
    }

    ESP_LOGI(TAG, "Configuring PCA9685 #%d", pca_index + 1);

    // Put PCA9685 to sleep (bit 4 of MODE1)
    if (!WritePca9685Register(pca_index, PCA9685_MODE1, 0x10)) {
        return false;
    }

    // Set prescaler for 50 Hz (for servo control)
    // prescale = round(osc_freq / (4096 * 50Hz)) - 1
    // osc_freq = 25MHz (typical for PCA9685)
    // prescale = round(25000000 / (4096 * 50)) - 1 = 121 (0x79)
    uint8_t prescale = 121;  // 50 Hz frequency
    if (!WritePca9685Register(pca_index, PCA9685_PRESCALE, prescale)) {
        return false;
    }

    // Wake up PCA9685 and enable Auto-Increment (bit 5 of MODE1)
    if (!WritePca9685Register(pca_index, PCA9685_MODE1, 0x20)) {
        return false;
    }

    // Wait for oscillator to stabilize
    esp_rom_delay_us(500);

    // Set MODE2 for PWM output
    if (!WritePca9685Register(pca_index, PCA9685_MODE2, 0x04)) {
        return false;
    }

    ESP_LOGI(TAG, "PCA9685 #%d configured for 50Hz", pca_index + 1);
    return true;
}

bool ServoController::WritePca9685Register(uint8_t pca_index, uint8_t reg_addr, uint8_t value) {
    if (pca_index >= 2 || pca9685_[pca_index] == nullptr) {
        return false;
    }

    uint8_t data[2] = {reg_addr, value};
    return i2c_master_transmit(pca9685_[pca_index], data, sizeof(data), 100) == ESP_OK;
}

bool ServoController::ReadPca9685Register(uint8_t pca_index, uint8_t reg_addr, uint8_t& value) {
    if (pca_index >= 2 || pca9685_[pca_index] == nullptr) {
        return false;
    }

    return i2c_master_transmit_receive(pca9685_[pca_index], &reg_addr, 1, &value, 1, 100) == ESP_OK;
}

void ServoController::GetPcaChannelMapping(uint8_t servo_id, uint8_t& pca_index, uint8_t& channel) {
    // 18 servos: first 9 on PCA9685 #1, next 9 on PCA9685 #2
    if (servo_id < 9) {
        pca_index = 0;
        channel = servo_id;
    } else {
        pca_index = 1;
        channel = servo_id - 9;
    }
}

uint16_t ServoController::AngleToPwm(float angle) {
    // Clamp angle to valid range using constants
    angle = std::max(HexapodConst::SERVO_MIN_ANGLE, std::min(HexapodConst::SERVO_MAX_ANGLE, angle));

    // Linear mapping: 0° = PWM_MIN_PULSE_US, 90° = PWM_MID_PULSE_US, 180° = PWM_MAX_PULSE_US
    // PWM_us = PWM_MIN_PULSE_US + (angle / 180) * (PWM_MAX_PULSE_US - PWM_MIN_PULSE_US)
    uint16_t pwm_us = static_cast<uint16_t>(HexapodConst::PWM_MIN_PULSE_US +
                                            (angle / 180.0f) * (HexapodConst::PWM_MAX_PULSE_US -
                                                                HexapodConst::PWM_MIN_PULSE_US));

    return pwm_us;
}

bool ServoController::SetPwm(uint8_t servo_id, uint16_t on_time) {
    if (servo_id >= HexapodConst::NUM_SERVOS) {
        ESP_LOGE(TAG, "Invalid servo ID: %d", servo_id);
        return false;
    }

    uint8_t pca_index, channel;
    GetPcaChannelMapping(servo_id, pca_index, channel);

    // PCA9685 PWM calculation:
    // 4096 counts per period, period = 20ms for 50Hz
    // on_count = (on_time_us / 20000) * 4096
    uint16_t on_count = static_cast<uint16_t>((on_time / 20000.0f) * 4096);
    on_count = static_cast<uint16_t>(std::min(4095UL, static_cast<uint32_t>(on_count)));  // Clamp to 12 bits

    // Set ON and OFF times
    // For standard servo PWM: ON starts at 0, OFF at on_count
    uint8_t on_l = 0x00;
    uint8_t on_h = 0x00;
    uint8_t off_l = on_count & 0xFF;
    uint8_t off_h = (on_count >> 8) & 0x0F;

    uint8_t led_on_l_reg = PCA9685_LED_ON_L(channel);
    uint8_t led_off_l_reg = PCA9685_LED_OFF_L(channel);

    if (!WritePca9685Register(pca_index, led_on_l_reg, on_l)) {
        return false;
    }
    if (!WritePca9685Register(pca_index, led_on_l_reg + 1, on_h)) {
        return false;
    }
    if (!WritePca9685Register(pca_index, led_off_l_reg, off_l)) {
        return false;
    }
    if (!WritePca9685Register(pca_index, led_off_l_reg + 1, off_h)) {
        return false;
    }

    return true;
}

bool ServoController::SetServoAngle(uint8_t servo_id, float angle) {
    if (servo_id >= HexapodConst::NUM_SERVOS) {
        return false;
    }

    uint16_t pwm_us = AngleToPwm(angle);
    bool result = SetPwm(servo_id, pwm_us);

    if (result) {
        current_angles_[servo_id] = angle;
        ESP_LOGV(TAG, "Servo %d set to %.1fÂ° (PWM: %uÂµs)", servo_id, angle, pwm_us);
    }

    return result;
}

bool ServoController::SetServoAngles(const float angles[18]) {
    return SetServoAnglesBatched(angles);
}

/**
 * @brief Set all 18 servos using batched I2C writes
 * Optimized: 2 I2C transactions total (1 per PCA9685) instead of 72
 */
bool ServoController::SetServoAnglesBatched(const float angles[18]) {
    // Update cached angles first (for GetServoAngle queries)
    for (int i = 0; i < HexapodConst::NUM_SERVOS; i++) {
        current_angles_[i] = angles[i];
    }

    // Measure I2C transaction time
    uint64_t i2c_start = esp_timer_get_time();

    // Batch I2C writes: 2 transactions total (one per PCA9685)
    bool all_success = true;

    for (uint8_t pca_index = 0; pca_index < HexapodConst::PCA9685_COUNT; pca_index++) {
        if (pca9685_[pca_index] == nullptr) {
            continue;  // Board not present (e.g., second PCA9685 optional)
        }

        // Build I2C payload: 36 bytes (9 servos × 4 bytes/channel)
        // PCA9685 register layout per channel:
        //   LEDx_ON_L, LEDx_ON_H, LEDx_OFF_L, LEDx_OFF_H
        // We want all OFF registers starting from LED0_OFF_L (0x08)
        uint8_t buffer[36];

        for (uint8_t channel = 0; channel < HexapodConst::SERVOS_PER_PCA9685; channel++) {
            uint8_t servo_id = pca_index * HexapodConst::SERVOS_PER_PCA9685 + channel;
            if (servo_id >= HexapodConst::NUM_SERVOS) {
                continue;
            }

            // Convert angle to PWM pulse width (microseconds)
            uint16_t pwm_us = AngleToPwm(angles[servo_id]);

            // Convert to 12-bit PWM count (4096 counts per 20ms period)
            uint16_t pwm_count = static_cast<uint16_t>((pwm_us / 20000.0f) * 4096);
            pwm_count = std::min<uint16_t>(4095, pwm_count);

            // For servos we want: ON=0, OFF=pwm_count
            // So pulse is from 0 to pwm_count
            uint8_t off_l = pwm_count & 0xFF;
            uint8_t off_h = (pwm_count >> 8) & 0x0F;  // Only 4 bits

            // PCA9685 expects: ON_L, ON_H, OFF_L, OFF_H
            uint8_t idx = channel * 4;
            buffer[idx + 0] = 0x00;  // ON_L
            buffer[idx + 1] = 0x00;  // ON_H
            buffer[idx + 2] = off_l; // OFF_L
            buffer[idx + 3] = off_h; // OFF_H
        }

        // Write all 9 channels in one I2C transaction
        // Start at LED0_ON_L register (0x06)
        uint8_t start_reg = PCA9685_LED_ON_L(0);  // 0x06

        // Construct I2C packet: [start_reg] + [36 bytes buffer]
        uint8_t packet[37];
        packet[0] = start_reg;
        memcpy(&packet[1], buffer, 36);

        esp_err_t err = i2c_master_transmit(pca9685_[pca_index], packet, sizeof(packet), 100);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "I2C batch write failed to PCA9685 #%d", pca_index + 1);
            all_success = false;
        }
    }

    uint64_t i2c_end = esp_timer_get_time();
    last_i2c_us_ = i2c_end - i2c_start;
    if (last_i2c_us_ > max_i2c_us_) {
        max_i2c_us_ = last_i2c_us_;
    }
    i2c_count_++;

    // Log I2C stats periodically (every 100 calls)
    if (i2c_count_ % 100 == 0) {
        ESP_LOGI(TAG, "I2C perf: batch=%.1fms (max=%.1fms) count=%u",
                 last_i2c_us_ / 1000.0, max_i2c_us_ / 1000.0, i2c_count_);
    }

    return all_success;
}

float ServoController::GetServoAngle(uint8_t servo_id) const {
    if (servo_id >= HexapodConst::NUM_SERVOS) {
        return -1.0f;
    }
    return current_angles_[servo_id];
}

void ServoController::MoveServo(uint8_t servo_id, float target_angle, uint8_t speed,
                               std::function<void()> callback) {
    if (servo_id >= HexapodConst::NUM_SERVOS) {
        return;
    }

    speed = std::max(1u, std::min(100u, (unsigned)speed));

    movements_[servo_id].active = true;
    movements_[servo_id].target_angle = target_angle;
    movements_[servo_id].current_angle = current_angles_[servo_id];
    movements_[servo_id].speed = speed;
    movements_[servo_id].start_time_ms = esp_log_timestamp();

    // Immediately set to target (simplified: no interpolation)
    SetServoAngle(servo_id, target_angle);

    if (callback) {
        callback();
    }
}

void ServoController::SetNeutral() {
    ESP_LOGD(TAG, "Setting all servos to neutral (%.1fÂ°)", HexapodConst::SERVO_NEUTRAL_ANGLE);

    // Tối ưu: dùng batched write thay vì 18 lần I2C riêng lẻ
    float neutral_angles[HexapodConst::NUM_SERVOS];
    for (uint8_t i = 0; i < HexapodConst::NUM_SERVOS; ++i) {
        neutral_angles[i] = HexapodConst::SERVO_NEUTRAL_ANGLE;
    }
    SetServoAnglesBatched(neutral_angles);
}

void ServoController::StopAll() {
    for (auto& m : movements_) {
        m.active = false;
    }
    SetNeutral();
}
