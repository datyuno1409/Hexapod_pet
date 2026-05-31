#pragma once

#include <cstdint>
#include <cmath>

/**
 * @brief Hexapod Constants
 *
 * Centralized all magic numbers and hardcoded values.
 * Benefits:
 *   - Single source of truth for tunable parameters
 *   - Easier to adjust robot geometry and motion parameters
 *   - Better documentation (names explain purpose)
 *   - Type safety with constexpr
 */

namespace HexapodConst {

// ============================================================================
// SERVO & PWM CONFIGURATION
// ============================================================================

/// PWM pulse width for 0Â° servo position (microseconds)
constexpr uint16_t PWM_MIN_PULSE_US = 500;

/// PWM pulse width for 90Â° servo position (microseconds)
constexpr uint16_t PWM_MID_PULSE_US = 1500;

/// PWM pulse width for 180Â° servo position (microseconds)
constexpr uint16_t PWM_MAX_PULSE_US = 2500;

/// Default servo neutral angle (degrees)
constexpr float SERVO_NEUTRAL_ANGLE = 90.0f;

/// PCA9685 PWM frequency for servos (Hz)
constexpr uint16_t PCA9685_FREQUENCY_HZ = 50;

// ============================================================================
// SERVO LAYOUT
// ============================================================================

/// Total number of servos (6 legs Ã— 3 DOF)
constexpr uint8_t NUM_SERVOS = 18;

/// Number of servos per PCA9685 board
constexpr uint8_t SERVOS_PER_PCA9685 = 9;

/// Number of PCA9685 boards
constexpr uint8_t PCA9685_COUNT = 2;

/// I2C addresses for PCA9685 boards
constexpr uint8_t PCA9685_ADDR_1 = 0x40;
constexpr uint8_t PCA9685_ADDR_2 = 0x41;

// ============================================================================
// LEG GEOMETRY (millimeters)
// ============================================================================

/// Coxa (hip rotation) segment length
constexpr float COXA_LENGTH_MM = 30.0f;

/// Femur (upper leg) segment length
constexpr float FEMUR_LENGTH_MM = 50.0f;

/// Tibia (lower leg) segment length
constexpr float TIBIA_LENGTH_MM = 70.0f;

// ============================================================================
// NEUTRAL/STANDING POSITION
// ============================================================================

/// Default standing posture: all joints at 90Â°
constexpr float NEUTRAL_ANGLE_COXA = 90.0f;
constexpr float NEUTRAL_ANGLE_FEMUR = 90.0f;
constexpr float NEUTRAL_ANGLE_TIBIA = 90.0f;

/// Neutral foot position offsets from body center (mm)
constexpr float NEUTRAL_X_OFFSET_MM = 0.0f;    ///< Forward/back from center (0 = straight out)
constexpr float NEUTRAL_Y_OFFSET_MM = 60.0f;   ///< Left/right from center
constexpr float NEUTRAL_Z_OFFSET_MM = -50.0f;  ///< Vertical (negative = down)

// ============================================================================
// GAIT PARAMETERS - TRIPOD
// ============================================================================

constexpr uint32_t TRIPOD_CYCLE_TIME_MS = 600;

/// Maximum forward stride length per step (mm)
constexpr float TRIPOD_STRIDE_LENGTH_MM = 80.0f;

/// Maximum foot lift height during swing (mm)
constexpr float TRIPOD_LIFT_HEIGHT_MM = 40.0f;

// ============================================================================
// GAIT PARAMETERS - RIPPLE
// ============================================================================

constexpr uint32_t RIPPLE_CYCLE_TIME_MS = 1000;
constexpr float RIPPLE_STRIDE_LENGTH_MM = 30.0f;
constexpr float RIPPLE_LIFT_HEIGHT_MM = 20.0f;

// ============================================================================
// GAIT PARAMETERS - WAVE
// ============================================================================

constexpr uint32_t WAVE_CYCLE_TIME_MS = 1500;
constexpr float WAVE_STRIDE_LENGTH_MM = 25.0f;
constexpr float WAVE_LIFT_HEIGHT_MM = 25.0f;

// ============================================================================
// MOTION GENERATION - BASIC POSES
// ============================================================================

/// Femur angle for forward position (degrees)
constexpr float FEMUR_ANGLE_FORWARD = 110.0f;

/// Femur angle for backward position (degrees)
constexpr float FEMUR_ANGLE_BACKWARD = 70.0f;

/// Femur angle for neutral position
constexpr float FEMUR_ANGLE_NEUTRAL = 90.0f;

/// Tibia angle when lifted (foot in air)
constexpr float TIBIA_ANGLE_LIFTED = 60.0f;

/// Tibia angle when planted (foot on ground)
constexpr float TIBIA_ANGLE_PLANTED = 120.0f;

/// Tibia angle for fully extended leg (jumping)
constexpr float TIBIA_ANGLE_EXTENDED = 30.0f;

/// Tibia angle for crouching (jump preparation)
constexpr float TIBIA_ANGLE_CROUCH_MIN = 60.0f;   ///< Minimum crouch angle
constexpr float TIBIA_ANGLE_CROUCH_MAX = 90.0f;   ///< Maximum crouch angle (intensity=100)

/// Femur angle for maximum upward extension (jump)
constexpr float FEMUR_ANGLE_UP = 120.0f;

/// Femur angle for crouch position
constexpr float FEMUR_ANGLE_CROUCH = 60.0f;

// ============================================================================
// MOTION GENERATION - TURNING
// ============================================================================

/// Hip (coxa) rotation offset during turn (degrees)
constexpr float COXA_TURN_OFFSET_DEG = 20.0f;

/// Femur angle during turn
constexpr float FEMUR_ANGLE_TURN = 90.0f;

/// Tibia angle during turn
constexpr float TIBIA_ANGLE_TURN = 110.0f;

// ============================================================================
// MOTION GENERATION - DANCE
// ============================================================================

/// Maximum coxa sway during dance (degrees from neutral)
constexpr float DANCE_COXA_SWAY_MAX_DEG = 30.0f;

// ============================================================================
// GAIT GENERATOR (Simplified ComputeLegIK - for fallback gait)
// ============================================================================

/// Swing amplitude for simplified gait (degrees)
constexpr float SWING_AMPLITUDE_DEG = 30.0f;

/// Swing height amplitude for foot lift (degrees) - separate from forward swing
constexpr float SWING_HEIGHT_AMPLITUDE_DEG = 20.0f;

/// Stance push amplitude for simplified gait (degrees)
constexpr float STANCE_AMPLITUDE_DEG = 15.0f;

// ============================================================================
// SPEED & INTENSITY SCALING
// ============================================================================

/// Default motion speed (0-100)
constexpr uint8_t DEFAULT_MOTION_SPEED = 50;

/// Maximum speed value
constexpr uint8_t MAX_SPEED = 100;

/// Minimum speed value
constexpr uint8_t MIN_SPEED = 1;

/// Default motion intensity (0-100)
constexpr uint8_t DEFAULT_MOTION_INTENSITY = 60;

/// Jump intensity scaling factor (maps 0-100 to crouch angle range)
constexpr float JUMP_INTENSITY_SCALE = 0.3f;  ///< (90-60) / 100

// ============================================================================
// INVERSE KINEMATICS
// ============================================================================

/// Servo angle limits (degrees)
constexpr float SERVO_MIN_ANGLE = 0.0f;
constexpr float SERVO_MAX_ANGLE = 180.0f;

/// Femur/tibia safe angle limits (avoid mechanical limits)
constexpr float FEMUR_MIN_ANGLE = 30.0f;
constexpr float FEMUR_MAX_ANGLE = 150.0f;
constexpr float TIBIA_MIN_ANGLE = 30.0f;
constexpr float TIBIA_MAX_ANGLE = 150.0f;

/// Coxa angle limits (hip rotation)
constexpr float COXA_MIN_ANGLE = 45.0f;
constexpr float COXA_MAX_ANGLE = 135.0f;

/// IK reachability margin (0.95 = 95% of max reach to avoid singularities)
constexpr float IK_REACHABILITY_MARGIN = 0.95f;

// ============================================================================
// COMMUNICATION
// ============================================================================

/// Default WebSocket/HTTP server port
constexpr uint16_t DEFAULT_SERVER_PORT = 8081;

/// Default command timeout (milliseconds)
constexpr uint32_t COMMAND_TIMEOUT_MS = 2000;

/// UART baud rate between VoiceBot and HexapodBot
constexpr uint32_t UART_BAUD_RATE = 921600;

// ============================================================================
// TASK & TIMING
// ============================================================================

/// Gait update frequency (Hz) - target ~20Hz
constexpr uint32_t GAIT_UPDATE_FREQ_HZ = 20;

/// Gait update period (milliseconds)
constexpr uint32_t GAIT_UPDATE_PERIOD_MS = 50;

/// Camera stream task priorities
constexpr uint8_t CAMERA_CAPTURE_TASK_PRIORITY = 6;
constexpr uint8_t CAMERA_ENCODE_TASK_PRIORITY = 5;
constexpr uint8_t CAMERA_SEND_TASK_PRIORITY = 4;

/// Camera stream task stack size (increased to 8192 for high-res JPEG encoding)
constexpr uint32_t CAMERA_STREAM_TASK_STACK_SIZE = 8192;

/// Camera stream task core (core 1 to avoid motion interference)
constexpr uint8_t CAMERA_STREAM_TASK_CORE = 1;

/// Camera stream FPS (stable default for OV5640 on COM10)
constexpr uint8_t CAMERA_STREAM_FPS = 12;

/// Camera stream JPEG quality (1-100)
constexpr uint8_t CAMERA_STREAM_QUALITY = 24;

/// Camera stream JPEG quality minimum (for slider)
constexpr uint8_t CAMERA_STREAM_QUALITY_MIN = 18;

/// Camera stream JPEG quality maximum (for slider)
constexpr uint8_t CAMERA_STREAM_QUALITY_MAX = 60;

/// Default stream resolution for COM10 OV5640 (stable + smooth)
constexpr uint16_t CAMERA_STREAM_DEFAULT_WIDTH = 320;
constexpr uint16_t CAMERA_STREAM_DEFAULT_HEIGHT = 240;

/// Higher preview modes allowed only when explicitly requested
constexpr uint16_t CAMERA_STREAM_PREVIEW_WIDTH = 640;
constexpr uint16_t CAMERA_STREAM_PREVIEW_HEIGHT = 480;

/// Safe XCLK for OV5640 on COM10 (12 MHz for minimal noise/streaks)
constexpr uint32_t CAMERA_XCLK_FREQ_HZ = 12000000;

/// Default camera capture resolution
constexpr const char* CAMERA_DEFAULT_RESOLUTION = "640x480";

/// Camera capture JPEG quality for high-quality snapshots
constexpr uint8_t CAMERA_CAPTURE_QUALITY = 90;

// ============================================================================
// DISPLAY
// ============================================================================

/// TFT display SPI frequency (Hz)
constexpr uint32_t TFT_SPI_FREQUENCY_HZ = 40'000'000;

/// Small TFT display SPI frequency (Hz) - more conservative for stability
constexpr uint32_t TFT_SMALL_SPI_FREQUENCY_HZ = 10'000'000;

// ============================================================================
// EMOTION DISPLAY
// ============================================================================

/// Default emotion (neutral)
constexpr const char* DEFAULT_EMOTION = "neutral";

/// Supported emotion types
constexpr const char* EMOTION_HAPPY = "happy";
constexpr const char* EMOTION_SAD = "sad";
constexpr const char* EMOTION_CURIOUS = "curious";
constexpr const char* EMOTION_EXCITED = "excited";
constexpr const char* EMOTION_NEUTRAL = "neutral";
constexpr const char* EMOTION_CONFUSED = "confused";
constexpr const char* EMOTION_ANGRY = "angry";

}  // namespace HexapodConst

