#pragma once

#include <cstdint>
#include <cmath>

/**
 * RIPPLE GAIT KINEMATICS
 *
 * WHAT IS RIPPLE GAIT?
 *   In ripple gait, only ONE leg swings at a time while 5 remain on ground.
 *   Legs lift sequentially, creating a "rippling" wave effect around the body.
 *   More stable than tripod but slower.
 *
 * LEG LIFT ORDER:
 *   0 → 1 → 2 → 3 → 4 → 5 → 0 → ...
 *   (Front-Left, Front-Right, Mid-Left, Mid-Right, Back-Left, Back-Right)
 *
 * PHASE TIMELINE (1000ms cycle):
 *   0ms-167ms:   Leg 0 swings (0.0-0.167)
 *   167ms-333ms: Leg 1 swings (0.167-0.333)
 *   333ms-500ms: Leg 2 swings (0.333-0.500)
 *   500ms-667ms: Leg 3 swings (0.500-0.667)
 *   667ms-833ms: Leg 4 swings (0.667-0.833)
 *   833ms-1000ms: Leg 5 swings (0.833-1.000)
 *
 * WHY RIPPLE?
 *   - Max stability: 5/6 legs on ground always
 *   - Smooth appearance: wave effect looks natural
 *   - Good for rough terrain, slow precise motion
 *
 * CYCLE TIME: 1000ms (1 second)
 */

namespace RippleGait {

constexpr uint32_t CYCLE_TIME_MS = 1000;

// Each leg occupies 1/6 of cycle
constexpr float LEG_SWING_FRACTION = 1.0f / 6.0f;  // ~0.167

// Leg geometry (same as tripod)
constexpr float COXA_LENGTH_MM = 30.0f;
constexpr float FEMUR_LENGTH_MM = 50.0f;
constexpr float TIBIA_LENGTH_MM = 70.0f;

// Stride slightly smaller than tripod for stability
constexpr float STRIDE_LENGTH_MM = 60.0f;
constexpr float LIFT_HEIGHT_MM = 30.0f;
constexpr float NEUTRAL_Z_OFFSET_MM = -50.0f;

/**
 * Determine if a specific leg is swinging at given phase.
 *
 * @param leg_id Leg number (0-5)
 * @param phase Global phase (0.0 to 1.0)
 * @return true if leg is currently in swing phase (in the air)
 *
 * HOW IT WORKS:
 *   Each leg has a dedicated "window" in the cycle.
 *   Window start = leg_id / 6.0
 *   Window end = (leg_id + 1) / 6.0
 *   Leg swings when phase is inside its window.
 */
inline bool IsLegSwinging(uint8_t leg_id, float phase) {
    float swing_start = leg_id * LEG_SWING_FRACTION;
    float swing_end = swing_start + LEG_SWING_FRACTION;
    return (phase >= swing_start) && (phase < swing_end);
}

/**
 * Get local phase for a specific leg (0.0-1.0 within its swing window).
 *
 * @param leg_id Leg number (0-5)
 * @param phase Global phase (0.0 to 1.0)
 * @return Local phase (0.0-1.0) within this leg's swing or stance window
 *
 * WHY local phase?
 *   Global phase tells us where we are in the full cycle.
 *   Local phase tells us where this specific leg is in ITS portion.
 *   Makes foot position computation independent per leg.
 */
inline float GetLegLocalPhase(uint8_t leg_id, float phase) {
    if (IsLegSwinging(leg_id, phase)) {
        // Within swing window, compute progress 0-1
        float swing_start = leg_id * LEG_SWING_FRACTION;
        return (phase - swing_start) / LEG_SWING_FRACTION;
    } else {
        // During stance, compute progress in pushback phase
        // WHY this calculation?
        //   Stance covers all cycle time except the swing window
        //   Need to map remaining time (5/6 of cycle) to 0-1

        float swing_start = leg_id * LEG_SWING_FRACTION;
        float stance_phase;

        if (phase < swing_start) {
            // Before swing window
            stance_phase = phase / swing_start;
        } else {
            // After swing window
            float swing_end = swing_start + LEG_SWING_FRACTION;
            stance_phase = (phase - swing_end) / (1.0f - swing_end);
        }

        return stance_phase;
    }
}

/**
 * Compute foot X position during ripple gait.
 *
 * @param leg_id Leg number (0-5)
 * @param phase Global phase (0.0 to 1.0)
 * @param direction 1.0=forward, -1.0=backward
 * @return X offset from neutral position (mm)
 *
 * ALGORITHM:
 *   During swing (1/6 cycle): move from back to front
 *   During stance (5/6 cycle): push body forward (leg moves backward)
 */
inline float ComputeFootX(uint8_t leg_id, float phase, float direction) {
    bool is_swinging = IsLegSwinging(leg_id, phase);
    float local_phase = GetLegLocalPhase(leg_id, phase);

    float x_offset;
    if (is_swinging) {
        // Swing: foot moves forward
        x_offset = -STRIDE_LENGTH_MM / 2.0f + local_phase * STRIDE_LENGTH_MM;
    } else {
        // Stance: foot pushes body (moves backward relative to body)
        x_offset = STRIDE_LENGTH_MM / 2.0f - local_phase * STRIDE_LENGTH_MM;
    }

    return x_offset * direction;
}

/**
 * Compute foot Z position (height) during ripple gait.
 *
 * @param leg_id Leg number (0-5)
 * @param phase Global phase (0.0 to 1.0)
 * @return Z position (mm, negative = below body)
 *
 * ALGORITHM:
 *   Swing: lift foot in sine arc (smooth up and down)
 *   Stance: keep foot on ground (constant Z)
 */
inline float ComputeFootZ(uint8_t leg_id, float phase) {
    if (IsLegSwinging(leg_id, phase)) {
        float local_phase = GetLegLocalPhase(leg_id, phase);
        // Sine arc: 0 → LIFT_HEIGHT → 0
        float lift = std::sin(local_phase * M_PI) * LIFT_HEIGHT_MM;
        return NEUTRAL_Z_OFFSET_MM + lift;
    } else {
        return NEUTRAL_Z_OFFSET_MM;  // Stance: on ground
    }
}

/**
 * Compute left/right offset for leg.
 * Same as tripod - Y position is fixed based on leg mounting.
 */
inline float ComputeFootY(uint8_t leg_id) {
    constexpr float Y_OFFSET = 80.0f;  // mm from center
    return (leg_id % 2 == 0) ? Y_OFFSET : -Y_OFFSET;
}

/**
 * Full foot position computation for ripple gait.
 */
inline void ComputeFootPosition(uint8_t leg_id, float phase, float direction,
                                 float& x_out, float& y_out, float& z_out) {
    constexpr float X_NEUTRAL = 0.0f;  // Neutral X from body center

    x_out = X_NEUTRAL + ComputeFootX(leg_id, phase, direction);
    y_out = ComputeFootY(leg_id);
    z_out = ComputeFootZ(leg_id, phase);
}

} // namespace RippleGait
