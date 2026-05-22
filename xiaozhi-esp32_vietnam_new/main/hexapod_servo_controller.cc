#include "hexapod_servo_controller.h"
#include "boards/hexapod_bot/config.h"
#include <esp_log.h>
#include <cmath>
#include <algorithm>

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
#define PCA9685_I2C_ADDR_1  PCA9685_ADDR_1  // 0x40
#define PCA9685_I2C_ADDR_2  PCA9685_ADDR_2  // 0x41

// ============================================================================
// ServoController Implementation
// ============================================================================

ServoController& ServoController::GetInstance() {
    static ServoController instance;
    return instance;
}

ServoController::ServoController() : current_angles_(SERVO_COUNT, 90.0f) {
    pca9685_[0] = nullptr;
    pca9685_[1] = nullptr;
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
        .scl_speed_hz = 100000,  // 100 kHz (xuống mức an toàn cho breadboard)
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

    // Wake up PCA9685
    if (!WritePca9685Register(pca_index, PCA9685_MODE1, 0x00)) {
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
    // Clamp angle to valid range
    angle = std::max(0.0f, std::min(180.0f, angle));

    // Linear mapping: 0Â° = 1000Âµs, 90Â° = 1500Âµs, 180Â° = 2000Âµs
    // PWM_us = 1000 + (angle / 180) * 1000
    uint16_t pwm_us = static_cast<uint16_t>(SERVO_MIN_PULSE_US + (angle / 180.0f) * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US));

    return pwm_us;
}

bool ServoController::SetPwm(uint8_t servo_id, uint16_t on_time) {
    if (servo_id >= SERVO_COUNT) {
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
    uint8_t on_l = on_count & 0xFF;
    uint8_t on_h = (on_count >> 8) & 0x0F;
    uint8_t off_l = (on_count + 1) & 0xFF;  // Offset off by 1 count
    uint8_t off_h = ((on_count + 1) >> 8) & 0x0F;

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
    if (servo_id >= SERVO_COUNT) {
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

bool ServoController::SetServoAngles(const std::map<uint8_t, float>& angles) {
    bool all_success = true;

    for (const auto& [servo_id, angle] : angles) {
        if (!SetServoAngle(servo_id, angle)) {
            ESP_LOGE(TAG, "Failed to set servo %d", servo_id);
            all_success = false;
        }
    }

    return all_success;
}

float ServoController::GetServoAngle(uint8_t servo_id) const {
    if (servo_id >= SERVO_COUNT) {
        return -1.0f;
    }
    return current_angles_[servo_id];
}

void ServoController::MoveServo(uint8_t servo_id, float target_angle, uint8_t speed,
                               std::function<void()> callback) {
    if (servo_id >= SERVO_COUNT) {
        return;
    }

    speed = std::max(1u, std::min(100u, (unsigned)speed));

    ServoMovement movement;
    movement.target_angle = target_angle;
    movement.current_angle = current_angles_[servo_id];
    movement.speed = speed;
    movement.start_time_ms = esp_log_timestamp();

    movements_[servo_id] = movement;

    // Immediately set to target (simplified: no interpolation)
    SetServoAngle(servo_id, target_angle);

    if (callback) {
        callback();
    }
}

void ServoController::SetNeutral() {
    ESP_LOGI(TAG, "Setting all servos to neutral (90Â°)");

    for (uint8_t i = 0; i < SERVO_COUNT; ++i) {
        SetServoAngle(i, 90.0f);
    }
}

void ServoController::StopAll() {
    movements_.clear();
    SetNeutral();
}
