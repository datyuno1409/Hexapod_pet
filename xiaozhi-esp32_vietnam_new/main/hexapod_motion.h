#ifndef HEXAPOD_MOTION_H
#define HEXAPOD_MOTION_H

#include <cstdint>
#include <map>

class ServoController; // Forward declaration

/**
 * @brief Hexapod Motion Controller
 *
 * Manages predefined motion sequences for hexapod robot.
 * Supports walking, jumping, dancing, and other behaviors.
 */
class HexapodMotion {
public:
    static HexapodMotion& GetInstance();

    /**
     * @brief Move forward
     * @param speed Speed 1-100
     * @param duration_ms Duration in milliseconds (0 = continuous)
     */
    void MoveForward(uint8_t speed = 50, uint32_t duration_ms = 0);

    /**
     * @brief Move backward
     * @param speed Speed 1-100
     * @param duration_ms Duration in milliseconds (0 = continuous)
     */
    void MoveBackward(uint8_t speed = 50, uint32_t duration_ms = 0);

    /**
     * @brief Turn left
     * @param speed Speed 1-100
     * @param duration_ms Duration in milliseconds (0 = continuous)
     */
    void TurnLeft(uint8_t speed = 50, uint32_t duration_ms = 0);

    /**
     * @brief Turn right
     * @param speed Speed 1-100
     * @param duration_ms Duration in milliseconds (0 = continuous)
     */
    void TurnRight(uint8_t speed = 50, uint32_t duration_ms = 0);

    /**
     * @brief Jump
     * @param intensity Intensity 1-100
     */
    void Jump(uint8_t intensity = 80);

    /**
     * @brief Dance
     * @param intensity Intensity 1-100
     * @param duration_ms Duration in milliseconds (0 = default)
     */
    void Dance(uint8_t intensity = 60, uint32_t duration_ms = 0);

    /**
     * @brief Stand up (neutral position)
     */
    void Stand();

    /**
     * @brief Sit down
     */
    void Sit();

    /**
     * @brief Stop current motion and return to neutral
     */
    void Stop();

    /**
     * @brief Set current speed for ongoing motion
     * @param speed Speed 1-100
     */
    void SetSpeed(uint8_t speed);

    /**
     * @brief Initialize dependency injection (call once at startup)
     * @param servo_ctrl ServoController instance to use
     */
    static void Init(ServoController& servo_ctrl);

private:
    HexapodMotion();
    ~HexapodMotion() = default;

    HexapodMotion(const HexapodMotion&) = delete;
    HexapodMotion& operator=(const HexapodMotion&) = delete;

    ServoController* servo_controller_ = nullptr;  // Injected dependency
    uint8_t current_speed_ = 50;
    bool is_moving_ = false;

    // Internal motion generation functions
    void GenerateWalkingGait(uint8_t speed, bool forward);
    void GenerateTurningGait(uint8_t speed, bool left);
    void GenerateJumpMotion(uint8_t intensity);
    void GenerateDanceMotion(uint8_t intensity);
};

#endif  // HEXAPOD_MOTION_H
