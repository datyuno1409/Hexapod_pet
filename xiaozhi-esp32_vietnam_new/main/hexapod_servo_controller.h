#ifndef HEXAPOD_SERVO_CONTROLLER_H
#define HEXAPOD_SERVO_CONTROLLER_H

#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <driver/i2c_master.h>

/**
 * @brief Hexapod Servo Controller
 *
 * Manages 18 servo motors via 2 PCA9685 I2C PWM controllers.
 * Each PCA9685 controls 9 servos.
 *
 * Servo layout (18 total):
 *   Front Left (FL):   0, 1, 2
 *   Front Right (FR):  3, 4, 5
 *   Middle Left (ML):  6, 7, 8
 *   Middle Right (MR): 9, 10, 11
 *   Back Left (BL):    12, 13, 14
 *   Back Right (BR):   15, 16, 17
 */
class ServoController {
public:
    static ServoController& GetInstance();

    /**
     * @brief Initialize servo controller with I2C handle
     * @param i2c_bus_handle I2C master bus handle for PCA9685
     * @return true if initialization successful
     */
    bool Initialize(i2c_master_bus_handle_t i2c_bus_handle);

    /**
     * @brief Set servo angle
     * @param servo_id Servo index (0-17)
     * @param angle Angle in degrees (0-180)
     * @return true if successful
     */
    bool SetServoAngle(uint8_t servo_id, float angle);

    /**
     * @brief Set multiple servos at once
     * @param angles Map of servo_id -> angle
     * @return true if all successful
     */
    bool SetServoAngles(const std::map<uint8_t, float>& angles);

    /**
     * @brief Get current servo angle
     * @param servo_id Servo index (0-17)
     * @return Current angle in degrees, or -1 if error
     */
    float GetServoAngle(uint8_t servo_id) const;

    /**
     * @brief Move servo with speed control
     * @param servo_id Servo index (0-17)
     * @param target_angle Target angle (0-180)
     * @param speed Speed 1-100 (1 = slowest, 100 = fastest)
     * @param callback Optional callback when movement completes
     */
    void MoveServo(uint8_t servo_id, float target_angle, uint8_t speed = 50,
                   std::function<void()> callback = nullptr);

    /**
     * @brief Set all servos to neutral (90 degrees) - standing position
     */
    void SetNeutral();

    /**
     * @brief Stop all servo movements
     */
    void StopAll();

private:
    ServoController();
    ~ServoController() = default;

    // Prevent copy/move
    ServoController(const ServoController&) = delete;
    ServoController& operator=(const ServoController&) = delete;

    // PCA9685 I2C device handles
    i2c_master_dev_handle_t pca9685_[2];  // Two PCA9685 @ 0x40, 0x41

    // Current servo angles (cache)
    std::vector<float> current_angles_;

    // Movement tracking
    struct ServoMovement {
        float target_angle;
        float current_angle;
        uint8_t speed;
        uint32_t start_time_ms;
    };
    std::map<uint8_t, ServoMovement> movements_;

    // I2C handle
    i2c_master_bus_handle_t i2c_bus_handle_ = nullptr;

    /**
     * @brief Calculate PWM value for angle
     * @param angle Angle in degrees (0-180)
     * @return PWM on-time in microseconds
     */
    uint16_t AngleToPwm(float angle);

    /**
     * @brief Set raw PWM value on PCA9685 channel
     * @param servo_id Servo index (0-17)
     * @param on_time PWM on-time in microseconds
     * @return true if successful
     */
    bool SetPwm(uint8_t servo_id, uint16_t on_time);

    /**
     * @brief Get PCA9685 address and channel for servo
     * @param servo_id Servo index (0-17)
     * @param[out] pca_index PCA9685 index (0 or 1)
     * @param[out] channel PCA9685 channel (0-15)
     */
    void GetPcaChannelMapping(uint8_t servo_id, uint8_t& pca_index, uint8_t& channel);

    /**
     * @brief Configure PCA9685 frequency and settings
     * @param pca_index PCA9685 index (0 or 1)
     * @return true if successful
     */
    bool ConfigurePca9685(uint8_t pca_index);

    /**
     * @brief Write command to PCA9685
     * @param pca_index PCA9685 index (0 or 1)
     * @param reg_addr Register address
     * @param value Value to write
     * @return true if successful
     */
    bool WritePca9685Register(uint8_t pca_index, uint8_t reg_addr, uint8_t value);

    /**
     * @brief Read from PCA9685
     * @param pca_index PCA9685 index (0 or 1)
     * @param reg_addr Register address
     * @param[out] value Value read
     * @return true if successful
     */
    bool ReadPca9685Register(uint8_t pca_index, uint8_t reg_addr, uint8_t& value);
};

#endif  // HEXAPOD_SERVO_CONTROLLER_H
