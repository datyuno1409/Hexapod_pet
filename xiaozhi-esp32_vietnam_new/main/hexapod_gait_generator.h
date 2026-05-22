#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <esp_timer.h>
#include "hexapod_servo_controller.h"

// Forward declaration of kinematics data structure
struct LegPose;

/**
 * GaitGenerator - High-level biological gait controller
 *
 * PURPOSE:
 *   Generates realistic hexapod locomotion using different gait patterns.
 *   Gaits are pre-defined movement sequences where different combinations
 *   of legs swing (in air) while others push (on ground) to move the body.
 *
 * ARCHITECTURE:
 *   - Receives high-level commands: Walk(gait_type, speed, direction)
 *   - Computes gait phase based on elapsed time (0-1 normalized)
 *   - Calculates target servo angles for each leg using inverse kinematics
 *   - Commands ServoController to move servos
 *   - Called periodically (~20Hz) to update motion smoothly
 *
 * GAIT TYPES:
 *   TRIPOD: 3 legs swinging, 3 on ground (fastest, very stable)
 *           Cycle: 600ms
 *
 *   RIPPLE: 1 leg swinging, 5 on ground (smooth, balanced speed)
 *           Cycle: 1000ms
 *
 *   WAVE:   Sequential wave motion (slowest, maximum stability)
 *           Cycle: 1500ms
 *
 * USAGE:
 *   1. Initialize: gait_gen.Initialize(servo_controller_ptr)
 *   2. Command:    gait_gen.Walk(TRIPOD, 50, FORWARD, 3000)  // 3 seconds
 *   3. Update:     gait_gen.Update()  // Call every 50ms in main loop
 *   4. Stop:       gait_gen.Stop()
 */
class GaitGenerator {
public:
    // Gait type determines which legs swing vs push
    enum GaitType {
        TRIPOD,     // Fast: 3 legs swing, 3 push
        RIPPLE,     // Balanced: 1 leg swings, 5 push (sequential)
        WAVE,       // Slow: Full sequential wave
    };

    // Direction of motion
    enum Direction {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
    };

    // Singleton access
    static GaitGenerator& GetInstance();

    // ============ LIFECYCLE ============

    /**
     * Initialize the gait generator with servo controller reference.
     * Must be called once before any motion commands.
     *
     * @param servo_ctrl Pointer to initialized ServoController
     * @return true if initialization successful, false if servo_ctrl is null
     *
     * WHY:
     *   We need the servo controller to send commands to actual servos.
     *   We store the pointer for later use during motion updates.
     */
    bool Initialize(ServoController* servo_ctrl);

    /**
     * Cleanup and shutdown gait generator.
     * Stop any ongoing motion and release resources.
     */
    void Shutdown();

    // ============ HIGH-LEVEL MOTION COMMANDS ============

    /**
     * Start walking with specified gait pattern.
     *
     * @param gait The gait type (TRIPOD, RIPPLE, WAVE)
     * @param speed Motion speed: 0-100 (0=no motion, 100=full speed)
     * @param dir Direction to move
     * @param duration_ms How long to walk (milliseconds)
     *
     * ALGORITHM:
     *   1. Store motion parameters (gait, speed, direction, duration)
     *   2. Record current timestamp as motion_start_time_
     *   3. Set is_walking_ = true
     *   4. Next Update() calls will compute and send servo commands
     *
     * WHY:
     *   We don't move immediately. Instead, we mark "motion requested"
     *   and let the main Update() loop compute smooth servo trajectories.
     *   This prevents jerky motion and allows for smooth speed scaling.
     */
    void Walk(GaitType gait, uint8_t speed, Direction dir, uint32_t duration_ms);

    /**
     * Stop ongoing motion immediately.
     * Robot returns to neutral standing position.
     * WHY: Safety - need immediate stop without waiting for motion to finish.
     */
    void Stop();

    /**
     * Pause current motion without stopping.
     * Robot maintains current position, can be resumed later.
     * WHY: For temporary pauses (e.g., obstacle detection).
     */
    void Pause();

    /**
     * Resume paused motion from current position.
     * WHY: Continue motion after pause.
     */
    void Resume();

    // ============ STATUS QUERIES ============

    /**
     * Is the robot currently walking?
     * @return true if motion is active and not paused
     */
    bool IsWalking() const { return is_walking_ && !is_paused_; }

    /**
     * Get the current gait type being used.
     * @return Current GaitType
     */
    GaitType GetCurrentGait() const { return current_gait_; }

    /**
     * Get elapsed time of current motion.
     * @return Milliseconds since motion started
     * WHY: For status display, debugging, or stopping based on time.
     */
    uint32_t GetElapsedTime() const;

    // ============ MAIN UPDATE FUNCTION ============

    /**
     * Update gait phase and compute servo angles.
     * MUST be called periodically (~20Hz = every 50ms) to generate smooth motion.
     *
     * INTERNAL ALGORITHM:
     *   1. Check if motion_duration has expired → Stop if done
     *   2. Compute normalized gait phase (0.0 to 1.0):
     *      phase = (elapsed_time % gait_cycle_time) / gait_cycle_time
     *   3. For each leg (0-5):
     *      a. Determine if leg is in swing or stance phase (depends on gait type)
     *      b. Call ComputeLegIK() to get target servo angles
     *      c. Scale angles by speed parameter for smooth motion
     *   4. Send all target angles to ServoController via SetServoAngles()
     *
     * WHY THIS APPROACH:
     *   - Modular: Each leg calculation is independent
     *   - Smooth: Phase-based calculation creates smooth transitions
     *   - Scalable: Speed parameter scales motion without changing timing
     *   - Efficient: Update only happens when walking, not constantly
     */
    void Update();

private:
    // ============ PRIVATE CONSTRUCTOR ============

    /**
     * Private constructor (singleton pattern).
     * Use GetInstance() instead.
     */
    GaitGenerator();

    // ============ INTERNAL COMPUTATION ============

    /**
     * Update the normalized gait phase (0.0 to 1.0).
     * This represents where we are in the gait cycle.
     *
     * EXAMPLE (Tripod 600ms cycle):
     *   elapsed_ms=0    → phase=0.0   (start of cycle)
     *   elapsed_ms=300  → phase=0.5   (middle - legs switch)
     *   elapsed_ms=600  → phase=0.0   (cycle repeats)
     *
     * WHY:
     *   Gait computation needs a 0-1 value to know which leg lifts when.
     *   Using modulo arithmetic, we can make motion repeat endlessly.
     */
    void UpdateGaitPhase();

    /**
     * Compute all 18 servo angles based on current gait phase.
     *
     * FOR EACH LEG:
     *   1. Call ComputeLegIK() to get coxa, femur, tibia angles
     *   2. Apply speed scaling: servo_speed = base_speed * (speed_percent/100)
     *   3. Apply direction transforms (rotate/mirror angles if moving sideways)
     *   4. Store in target_angles_ map
     *
     * WHY:
     *   Centralized servo computation ensures all legs move synchronously.
     *   Speed scaling lets us slow down fast gaits for precise movement.
     */
    void ComputeServoAngles();

    /**
     * Inverse Kinematics: Convert foot position to servo angles.
     *
     * INPUT:
     *   leg_id: 0-5 (which leg)
     *   phase_0_to_1: 0.0-1.0 (where in gait cycle)
     *
     * OUTPUT:
     *   LegPose with:
     *   - coxa: Hip rotation ±45° from center
     *   - femur: Shoulder swing 0-180°
     *   - tibia: Knee bend 0-180°
     *
     * ALGORITHM (Simplified 3-link arm IK):
     *   1. Determine if leg is swinging or pushing (depends on gait phase)
     *   2. If swinging:
     *      - Use sinusoidal motion for forward/back swing
     *      - Use sinusoidal motion for up/down height
     *      - Result: smooth arc through air
     *   3. If pushing:
     *      - Keep leg extended on ground
     *      - Slightly bend knee for stability
     *      - Result: rigid contact with ground
     *
     * WHY:
     *   This is the core of realistic gait motion.
     *   Different leg positions for swing vs stance create the walking pattern.
     *   Sinusoidal curves make motion smooth and natural-looking.
     */
    LegPose ComputeLegIK(int leg_id, float phase_0_to_1);

    /**
     * Get gait-specific phase calculation.
     * Different gaits have different leg lifting patterns.
     *
     * TRIPOD PHASE:
     *   0.0-0.5: Legs 0,2,4 swing; Legs 1,3,5 push
     *   0.5-1.0: Legs 1,3,5 swing; Legs 0,2,4 push
     *
     * RIPPLE PHASE:
     *   Each leg lifts sequentially for ~150ms out of 1000ms
     *   0.0-0.167: Leg 0 swings
     *   0.167-0.333: Leg 1 swings
     *   ...and so on
     *
     * WHY:
     *   Gait phase determines which legs move when.
     *   Different gaits have completely different phase patterns.
     *   Centralizing this logic makes it easy to add new gaits later.
     */
    float GetTripodPhase(float norm_time);
    float GetRipplePhase(float norm_time);
    float GetWavePhase(float norm_time);

    /**
     * Smooth interpolation curve for natural-looking motion.
     * Linear movement looks robotic. Smooth curves look organic.
     *
     * EASING FUNCTION:
     *   Quadratic ease-in-out: smooth acceleration + deceleration
     *   Makes servo speed curve smooth, not jerky
     *
     * WHY:
     *   Raw phase values (0-1) create instant changes.
     *   Smoothing curves make transitions gradual and natural.
     */
    float EaseInOutQuad(float t);

    // ============ MEMBER VARIABLES ============

    // Reference to servo controller (we don't own it)
    ServoController* servo_controller_;

    // Current motion state
    GaitType current_gait_;
    Direction current_dir_;
    uint8_t speed_percent_;        // 0-100

    // Timing
    uint32_t motion_start_time_;   // esp_timer_get_time() when motion started
    uint32_t motion_duration_ms_;  // How long the current motion lasts
    uint32_t gait_cycle_time_ms_;  // TRIPOD:600, RIPPLE:1000, WAVE:1500

    // Motion control flags
    bool is_walking_;
    bool is_paused_;

    // Cached target servo angles (updated each cycle)
    // Key: servo_id (0-17), Value: target angle in degrees
    std::map<uint8_t, float> target_angles_;
};

/**
 * Kinematics output structure.
 * Represents the three servo angles for one leg.
 */
struct LegPose {
    float coxa;   // Hip rotation: -45° to +45° (center = 90°)
    float femur;  // Shoulder swing: 0-180°
    float tibia;  // Knee bend: 0-180°
};
