#pragma once

#include <cstdint>
#include <cmath>

/**
 * LUNGE ATTACK ANIMATION
 *
 * WHAT IS LUNGE?
 *   A forward charging attack where the robot accelerates toward target.
 *   Uses tripod gait at high speed, then impacts with front legs extended.
 *   Covers distance (5-15cm depending on intensity).
 *
 * ANIMATION TIMELINE (1100ms total):
 *
 *   0ms - 400ms: WEIGHT SHIFT PHASE
 *     - Body tilts forward
 *     - Front legs step forward
 *     - Rear legs prepare to push
 *
 *   400ms - 700ms: ACCELERATE PHASE  ← CRITICAL (300ms)
 *     - Tripod gait at 80% speed
 *     - 2-3 rapid steps forward
 *     - Body momentum builds
 *
 *   700ms - 800ms: IMPACT PHASE (100ms)
 *     - Front legs extend sharply
 *     - Body stops abruptly (simulated collision)
 *     - Rear legs lock for stability
 *
 *   800ms - 1100ms: RECOVERY PHASE
 *     - Return to neutral standing position
 *     - Weight re-centers
 *
 * INTENSITY PARAMETER (0-100):
 *   - Controls DISTANCE and SPEED of lunge
 *   - 100: Maximum distance (~15cm), very fast
 *   - 50: Moderate lunge (~8cm)
 *   - 0: No movement
 *
 * DISTANCE CALCULATION:
 *   distance_cm = (intensity / 100) * 15cm
 *   Example: intensity=80 → 12cm forward
 */

namespace LungeAttack {

// Total animation duration
constexpr uint32_t TOTAL_DURATION_MS = 1100;

// Phase breakpoints (0.0 to 1.0)
constexpr float SHIFT_END       = 0.364f;  // Weight shift ends at 400ms
constexpr float ACCELERATE_END  = 0.636f;  // Accelerate ends at 700ms
constexpr float IMPACT_END      = 0.727f;  // Impact ends at 800ms
// Recovery: 72.7% to 100%

// Motion parameters
constexpr float MAX_LUNGE_DISTANCE_CM = 15.0f;  // Maximum forward travel
constexpr float TRIPOD_STRIDE_MM = 40.0f;       // Stride during accelerate
constexpr float IMPACT_EXTEND_DEG = 35.0f;      // Front leg extension at impact

/**
 * Ease-in-out curve for smooth acceleration.
 *
 * WHY ease-in-out?
 *   Starts slow (weight shift)
 *   Speeds up in middle (acceleration)
 *   Slows at end (impact control)
 *
 * @param t Progress 0.0 to 1.0
 * @return Curved output 0.0 to 1.0
 */
inline float SmoothEase(float t) {
    return t * t * (3.0f - 2.0f * t);  // Smoothstep
}

/**
 * Sharp ease-in for impact (fast deceleration).
 *
 * WHY sharp?
 *   Simulates sudden stop when hitting target
 *   Creates "thud" effect
 *
 * @param t Progress 0.0 to 1.0
 * @return Curved output 0.0 to 1.0
 */
inline float ImpactEase(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);  // Cubic ease-out
}

/**
 * Compute servo angles for WEIGHT SHIFT phase.
 *
 * @param progress Phase progress (0.0 to 1.0)
 * @param intensity Attack intensity (0-100)
 * @param angles Output array: angles[18]
 *
 * WHAT HAPPENS:
 *   Body tilts forward ~10-15 degrees
 *   Front legs step forward slightly
 *   Rear legs prepare to push (slight bend)
 *   Center of mass shifts toward front
 */
inline void ComputeShiftAngles(float progress, float intensity,
                                float angles[18]) {
    float scale = intensity / 100.0f;
    float t = SmoothEase(progress);

    // Front legs step forward
    constexpr float FRONT_STEP_DEG = 15.0f;

    // Front-Left (servos 0,1,2)
    angles[0] = 90.0f - FRONT_STEP_DEG * t * scale;  // Coxa: rotate forward
    angles[1] = 90.0f + FRONT_STEP_DEG * t * scale;  // Femur: swing forward
    angles[2] = 90.0f - FRONT_STEP_DEG * t * scale;  // Tibia: compensate

    // Front-Right (servos 3,4,5)
    angles[3] = 90.0f + FRONT_STEP_DEG * t * scale;
    angles[4] = 90.0f + FRONT_STEP_DEG * t * scale;
    angles[5] = 90.0f - FRONT_STEP_DEG * t * scale;

    // Middle legs stay neutral
    for (int i = 6; i < 12; i++) {
        angles[i] = 90.0f;
    }

    // Rear legs prepare to push (slight bend)
    constexpr float REAR_BEND_DEG = 10.0f;

    // Back-Left (servos 12,13,14)
    angles[12] = 90.0f;
    angles[13] = 90.0f - REAR_BEND_DEG * t * scale;  // Femur: bend down
    angles[14] = 90.0f + REAR_BEND_DEG * t * scale;  // Tibia: compensate

    // Back-Right (servos 15,16,17)
    angles[15] = 90.0f;
    angles[16] = 90.0f - REAR_BEND_DEG * t * scale;
    angles[17] = 90.0f + REAR_BEND_DEG * t * scale;
}

/**
 * Compute servo angles for ACCELERATE phase.
 *
 * @param progress Phase progress (0.0 to 1.0)
 * @param intensity Attack intensity (0-100)
 * @param angles Output array: angles[18]
 *
 * WHAT HAPPENS:
 *   Uses tripod gait pattern at high speed
 *   2-3 rapid steps forward
 *   Body moves forward ~10-12cm
 *
 * ALGORITHM:
 *   Simulate tripod gait with compressed cycle time
 *   Group A (0,2,4) and Group B (1,3,5) alternate
 *   Each group swings for 50% of phase
 */
inline void ComputeAccelerateAngles(float progress, float intensity,
                                     float angles[18]) {
    float scale = intensity / 100.0f;

    // Tripod gait: 2 cycles in 300ms = 150ms per cycle (very fast)
    // Simulate 2 full tripod cycles during this phase
    float gait_phase = std::fmod(progress * 2.0f, 1.0f);  // 0-1, repeats twice

    // Determine which group is swinging
    bool group_a_swinging = (gait_phase < 0.5f);
    float leg_phase = group_a_swinging ? (gait_phase * 2.0f) : ((gait_phase - 0.5f) * 2.0f);

    // Stride parameters
    constexpr float STRIDE_DEG = 25.0f;  // Compressed stride for speed

    for (int leg = 0; leg < 6; leg++) {
        bool is_group_a = (leg % 2 == 0);
        bool is_swinging = (is_group_a == group_a_swinging);

        int servo_base = leg * 3;

        if (is_swinging) {
            // Swing: foot moves forward
            float swing_progress = leg_phase;
            angles[servo_base + 0] = 90.0f;  // Coxa: centered
            angles[servo_base + 1] = 90.0f + STRIDE_DEG * swing_progress * scale;
            angles[servo_base + 2] = 90.0f - STRIDE_DEG * swing_progress * scale;
        } else {
            // Stance: foot pushes body forward
            float stance_progress = leg_phase;
            angles[servo_base + 0] = 90.0f;
            angles[servo_base + 1] = 90.0f + STRIDE_DEG * (1.0f - stance_progress) * scale;
            angles[servo_base + 2] = 90.0f - STRIDE_DEG * (1.0f - stance_progress) * scale;
        }
    }
}

/**
 * Compute servo angles for IMPACT phase.
 *
 * @param progress Phase progress (0.0 to 1.0)
 * @param intensity Attack intensity (0-100)
 * @param angles Output array: angles[18]
 *
 * WHAT HAPPENS:
 *   Front legs extend sharply (simulating collision)
 *   Rear legs lock in place
 *   Body stops abruptly
 */
inline void ComputeImpactAngles(float progress, float intensity,
                                 float angles[18]) {
    float scale = intensity / 100.0f;
    float t = ImpactEase(progress);

    // Front legs extend forward and down
    // Front-Left (servos 0,1,2)
    angles[0] = 90.0f - IMPACT_EXTEND_DEG * t * scale;
    angles[1] = 90.0f + IMPACT_EXTEND_DEG * t * scale;
    angles[2] = 90.0f - IMPACT_EXTEND_DEG * t * scale;

    // Front-Right (servos 3,4,5)
    angles[3] = 90.0f + IMPACT_EXTEND_DEG * t * scale;
    angles[4] = 90.0f + IMPACT_EXTEND_DEG * t * scale;
    angles[5] = 90.0f - IMPACT_EXTEND_DEG * t * scale;

    // Middle and rear legs lock in neutral
    for (int i = 6; i < 18; i++) {
        angles[i] = 90.0f;
    }
}

/**
 * Compute servo angles for RECOVERY phase.
 *
 * @param progress Phase progress (0.0 to 1.0)
 * @param intensity Attack intensity (0-100)
 * @param angles Output array: angles[18]
 *
 * WHAT HAPPENS:
 *   All legs smoothly return to neutral (90°)
 *   Body weight re-centers
 */
inline void ComputeRecoveryAngles(float /*progress*/, float /*intensity*/,
                                   float angles[18]) {
    // All servos return to 90° (neutral); servo controller smooths the transition
    for (int i = 0; i < 18; i++) {
        angles[i] = 90.0f;
    }
}

/**
 * Main function: compute all servo angles for current lunge phase.
 *
 * @param elapsed_ms Time since lunge started (milliseconds)
 * @param intensity Attack intensity (0-100)
 * @param angles Output array: angles[18]
 *
 * USAGE:
 *   Call this every 50ms during lunge animation
 *   Send resulting angles to ServoController
 */
inline void ComputeLungeFrame(uint32_t elapsed_ms, uint8_t intensity,
                               float angles[18]) {
    // Normalize elapsed time to 0.0-1.0
    float global_phase = (float)elapsed_ms / (float)TOTAL_DURATION_MS;
    if (global_phase > 1.0f) {
        global_phase = 1.0f;  // Clamp
    }

    // Determine which phase we're in
    if (global_phase < SHIFT_END) {
        // Weight shift phase
        float local_progress = global_phase / SHIFT_END;
        ComputeShiftAngles(local_progress, intensity, angles);
    } else if (global_phase < ACCELERATE_END) {
        // Accelerate phase
        float local_progress = (global_phase - SHIFT_END) / (ACCELERATE_END - SHIFT_END);
        ComputeAccelerateAngles(local_progress, intensity, angles);
    } else if (global_phase < IMPACT_END) {
        // Impact phase
        float local_progress = (global_phase - ACCELERATE_END) / (IMPACT_END - ACCELERATE_END);
        ComputeImpactAngles(local_progress, intensity, angles);
    } else {
        // Recovery phase
        float local_progress = (global_phase - IMPACT_END) / (1.0f - IMPACT_END);
        ComputeRecoveryAngles(local_progress, intensity, angles);
    }
}

} // namespace LungeAttack
