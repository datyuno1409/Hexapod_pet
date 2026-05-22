#pragma once

#include <cstdint>
#include <cmath>

/**
 * STRIKE ATTACK ANIMATION
 *
 * WHAT IS STRIKE?
 *   A quick jab attack using the front two legs.
 *   Robot snaps front legs forward, hits target, then recovers.
 *   Fast and aggressive — optimized for speed.
 *
 * ANIMATION TIMELINE (600ms total):
 *
 *   0ms - 300ms: PREPARE PHASE
 *     - Rear legs (4,5) bend deeper to lower body
 *     - Front legs (0,1) raise up and cock back
 *     - Body weight shifts rearward
 *
 *   300ms - 400ms: STRIKE PHASE  ← CRITICAL (100ms, very fast)
 *     - Front legs SNAP forward and downward
 *     - Coxa rotates outward (±30°)
 *     - Femur drives forward and down
 *     - Tibia extends sharply
 *
 *   400ms - 600ms: RECOVERY PHASE
 *     - All legs return smoothly to neutral
 *     - Body re-centers weight
 *
 * INTENSITY PARAMETER (0-100):
 *   - Controls SPEED and RANGE of strike motion
 *   - 100: Maximum speed, maximum reach
 *   - 50: Moderate demonstration strike
 *   - 0: No movement (idle)
 *
 * SERVO LAYOUT:
 *   Leg 0 (FL): Servos 0 (coxa), 1 (femur), 2 (tibia)
 *   Leg 1 (FR): Servos 3 (coxa), 4 (femur), 5 (tibia)
 *   Leg 4 (BL): Servos 12 (coxa), 13 (femur), 14 (tibia)
 *   Leg 5 (BR): Servos 15 (coxa), 16 (femur), 17 (tibia)
 */

namespace StrikeAttack {

// Total animation duration
constexpr uint32_t TOTAL_DURATION_MS = 600;

// Phase breakpoints (0.0 to 1.0)
constexpr float PREPARE_END   = 0.50f;   // Prepare ends at 50% = 300ms
constexpr float STRIKE_END    = 0.667f;  // Strike ends at 66.7% = 400ms
// Recovery: 66.7% to 100%

// Servo angle limits for strike
constexpr float STRIKE_COXA_EXTEND    = 30.0f;  // Coxa outward rotation (degrees)
constexpr float STRIKE_FEMUR_EXTEND   = 40.0f;  // Femur forward swing
constexpr float STRIKE_TIBIA_EXTEND   = 40.0f;  // Tibia strike snap

constexpr float PREPARE_FEMUR_RAISE   = 30.0f;  // Raise femur in prepare
constexpr float PREPARE_REAR_BEND     = 20.0f;  // Rear leg bend in prepare

/**
 * Interpolate value using cubic ease-in (fast acceleration).
 *
 * WHY cubic for strike?
 *   Linear = constant speed (boring, mechanical)
 *   Cubic ease-in = slow start, then FAST at impact
 *   Creates "whip" effect that looks powerful
 *
 * @param t Progress 0.0 to 1.0
 * @return Curved output 0.0 to 1.0
 */
inline float StrikeEase(float t) {
    return t * t * t;  // Cubic: t^3
}

/**
 * Smooth ease-out for recovery (decelerate).
 *
 * WHY ease-out for recovery?
 *   Fast start coming out of strike, smooth landing in neutral
 *   Looks controlled, not abrupt
 *
 * @param t Progress 0.0 to 1.0
 * @return Curved output 0.0 to 1.0
 */
inline float RecoveryEase(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);  // Quadratic ease-out
}

/**
 * Compute servo angles for the PREPARE phase.
 *
 * @param progress Phase progress (0.0 = start, 1.0 = end of prepare)
 * @param intensity Attack intensity (0-100)
 * @param angles Output map: servo_id → target_angle
 *
 * WHAT HAPPENS:
 *   Rear legs (4,5) settle lower to act as anchor
 *   Front legs (0,1) rise up and pull back
 *   Think of a boxer pulling arm back before punch
 */
inline void ComputePrepareAngles(float progress, float intensity,
                                  float angles[18]) {
    float scale = intensity / 100.0f;

    // Smooth interpolation for prepare
    float t = progress;  // Linear is fine for preparation

    // Front-Left leg (servos 0, 1, 2)
    angles[0] = 90.0f;                                       // Coxa: centered
    angles[1] = 90.0f + PREPARE_FEMUR_RAISE * t * scale;    // Femur: raises up
    angles[2] = 90.0f - PREPARE_FEMUR_RAISE * t * scale;    // Tibia: angles back

    // Front-Right leg (servos 3, 4, 5)
    angles[3] = 90.0f;                                       // Coxa: centered
    angles[4] = 90.0f + PREPARE_FEMUR_RAISE * t * scale;    // Femur: raises up
    angles[5] = 90.0f - PREPARE_FEMUR_RAISE * t * scale;    // Tibia: angles back

    // Middle legs (servos 6-11) stay roughly neutral
    for (int i = 6; i < 12; i++) {
        angles[i] = 90.0f;
    }

    // Back-Left leg (servos 12, 13, 14)
    angles[12] = 90.0f;                                      // Coxa: centered
    angles[13] = 90.0f - PREPARE_REAR_BEND * t * scale;     // Femur: bends lower
    angles[14] = 90.0f + PREPARE_REAR_BEND * t * scale;     // Tibia: bends to compensate

    // Back-Right leg (servos 15, 16, 17)
    angles[15] = 90.0f;
    angles[16] = 90.0f - PREPARE_REAR_BEND * t * scale;
    angles[17] = 90.0f + PREPARE_REAR_BEND * t * scale;
}

/**
 * Compute servo angles for the STRIKE phase.
 *
 * @param progress Phase progress (0.0 = start of strike, 1.0 = full extension)
 * @param intensity Attack intensity (0-100)
 * @param angles Output map: servo_id → target_angle
 *
 * WHAT HAPPENS:
 *   Front legs SNAP forward with high speed (cubic ease-in)
 *   Coxa spreads slightly outward for wider reach
 *   Femur and tibia drive forward and down
 *   Rear legs LOCK in place as anchor
 */
inline void ComputeStrikeAngles(float progress, float intensity,
                                 float angles[18]) {
    float scale = intensity / 100.0f;

    // Cubic ease-in: SLOW to FAST (whip effect)
    float t = StrikeEase(progress);

    // Front-Left leg - extends and strikes
    angles[0] = 90.0f - STRIKE_COXA_EXTEND * t * scale;    // Coxa: rotates outward left
    angles[1] = 90.0f + PREPARE_FEMUR_RAISE +               // Starts raised
                STRIKE_FEMUR_EXTEND * t * scale;             // Then drives forward
    angles[2] = 90.0f - PREPARE_FEMUR_RAISE -               // Starts angled
                STRIKE_TIBIA_EXTEND * t * scale;             // Then snaps down

    // Front-Right leg - mirrors left
    angles[3] = 90.0f + STRIKE_COXA_EXTEND * t * scale;    // Coxa: rotates outward right
    angles[4] = 90.0f + PREPARE_FEMUR_RAISE +
                STRIKE_FEMUR_EXTEND * t * scale;
    angles[5] = 90.0f - PREPARE_FEMUR_RAISE -
                STRIKE_TIBIA_EXTEND * t * scale;

    // Middle legs: stay fixed (maximum stability during strike)
    for (int i = 6; i < 12; i++) {
        angles[i] = 90.0f;
    }

    // Rear legs: locked from prepare position (anchors)
    angles[12] = 90.0f;
    angles[13] = 90.0f - PREPARE_REAR_BEND * scale;
    angles[14] = 90.0f + PREPARE_REAR_BEND * scale;

    angles[15] = 90.0f;
    angles[16] = 90.0f - PREPARE_REAR_BEND * scale;
    angles[17] = 90.0f + PREPARE_REAR_BEND * scale;
}

/**
 * Compute servo angles for the RECOVERY phase.
 *
 * @param progress Phase progress (0.0 = start, 1.0 = fully recovered)
 * @param intensity Attack intensity (0-100)
 * @param angles Output map: servo_id → target_angle
 *
 * WHAT HAPPENS:
 *   All legs ease back to neutral (90°) smoothly
 *   Quadratic ease-out: fast start, gentle landing
 */
inline void ComputeRecoveryAngles(float progress, float intensity,
                                   float angles[18]) {
    float scale = intensity / 100.0f;

    // Recovery eases out toward neutral
    float t = RecoveryEase(progress);     // Curved progress
    float inv_t = 1.0f - t;              // Inverse: 1→0 (how far from neutral)

    // Front legs return to neutral from strike position
    angles[0] = 90.0f - STRIKE_COXA_EXTEND * inv_t * scale;
    angles[1] = 90.0f + PREPARE_FEMUR_RAISE * inv_t * scale +
                STRIKE_FEMUR_EXTEND * inv_t * scale;
    angles[2] = 90.0f - PREPARE_FEMUR_RAISE * inv_t * scale -
                STRIKE_TIBIA_EXTEND * inv_t * scale;

    angles[3] = 90.0f + STRIKE_COXA_EXTEND * inv_t * scale;
    angles[4] = 90.0f + PREPARE_FEMUR_RAISE * inv_t * scale +
                STRIKE_FEMUR_EXTEND * inv_t * scale;
    angles[5] = 90.0f - PREPARE_FEMUR_RAISE * inv_t * scale -
                STRIKE_TIBIA_EXTEND * inv_t * scale;

    // Middle legs return to neutral
    for (int i = 6; i < 12; i++) {
        angles[i] = 90.0f;
    }

    // Rear legs return to neutral from bent position
    angles[12] = 90.0f;
    angles[13] = 90.0f - PREPARE_REAR_BEND * inv_t * scale;
    angles[14] = 90.0f + PREPARE_REAR_BEND * inv_t * scale;

    angles[15] = 90.0f;
    angles[16] = 90.0f - PREPARE_REAR_BEND * inv_t * scale;
    angles[17] = 90.0f + PREPARE_REAR_BEND * inv_t * scale;
}

/**
 * Main entry: compute ALL servo angles for strike at given global progress.
 *
 * @param global_progress 0.0 to 1.0 (full animation)
 * @param intensity Attack intensity (0-100)
 * @param angles Output: all 18 servo angles
 *
 * ROUTES to correct phase function based on progress:
 *   0.0 - 0.5:   PreparePhase
 *   0.5 - 0.667: StrikePhase
 *   0.667 - 1.0: RecoveryPhase
 */
inline void ComputeStrikeFrame(float global_progress, float intensity,
                                float angles[18]) {
    if (global_progress < PREPARE_END) {
        float local = global_progress / PREPARE_END;
        ComputePrepareAngles(local, intensity, angles);

    } else if (global_progress < STRIKE_END) {
        float local = (global_progress - PREPARE_END) / (STRIKE_END - PREPARE_END);
        ComputeStrikeAngles(local, intensity, angles);

    } else {
        float local = (global_progress - STRIKE_END) / (1.0f - STRIKE_END);
        ComputeRecoveryAngles(local, intensity, angles);
    }
}

} // namespace StrikeAttack
