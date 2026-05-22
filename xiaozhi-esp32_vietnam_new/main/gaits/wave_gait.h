#pragma once

#include <cstdint>
#include <cmath>

/**
 * WAVE GAIT KINEMATICS
 *
 * WHAT IS WAVE GAIT?
 *   The slowest and most stable hexapod gait.
 *   Legs lift in a wave pattern from BACK to FRONT.
 *   Always 5 legs on ground (same as ripple).
 *
 * WAVE ORDER:
 *   Back-Left (4) → Back-Right (5) → Mid-Left (2) → Mid-Right (3) → Front-Left (0) → Front-Right (1)
 *
 * WHY BACK TO FRONT?
 *   - Weight transfers smoothly from rear to front
 *   - Mimics how insects and crustaceans walk
 *   - More stable because rear legs plant before front lifts
 *
 * PHASE TIMELINE (1500ms cycle):
 *   0ms-250ms:   Leg 4 (BL) swings (0.0-0.167)
 *   250ms-500ms: Leg 5 (BR) swings (0.167-0.333)
 *   500ms-750ms: Leg 2 (ML) swings (0.333-0.500)
 *   750ms-1000ms: Leg 3 (MR) swings (0.500-0.667)
 *   1000ms-1250ms: Leg 0 (FL) swings (0.667-0.833)
 *   1250ms-1500ms: Leg 1 (FR) swings (0.833-1.000)
 *
 * USE CASES:
 *   - Rough terrain navigation
 *   - Climbing / unstable surfaces
 *   - Slow, deliberate movement
 *
 * CYCLE TIME: 1500ms (1.5 seconds)
 */

namespace WaveGait {

constexpr uint32_t CYCLE_TIME_MS = 1500;

// Each leg occupies 1/6 of cycle
constexpr float LEG_SWING_FRACTION = 1.0f / 6.0f;

// Leg geometry (same as other gaits)
constexpr float COXA_LENGTH_MM = 30.0f;
constexpr float FEMUR_LENGTH_MM = 50.0f;
constexpr float TIBIA_LENGTH_MM = 70.0f;

// Wave gait uses smaller strides for maximum stability
constexpr float STRIDE_LENGTH_MM = 25.0f;
constexpr float LIFT_HEIGHT_MM = 25.0f;  // Lifts slightly higher for clearance
constexpr float NEUTRAL_Z_OFFSET_MM = -80.0f;

// Wave order: back to front
// WHY this order?
//   Index: wave_order[i] = which leg lifts at i-th turn
//   Rear legs (4,5) lift first → weight already on front legs
//   Front legs (0,1) lift last → most stable configuration
constexpr uint8_t WAVE_ORDER[6] = {4, 5, 2, 3, 0, 1};

/**
 * Get the position in wave order for a given leg.
 *
 * @param leg_id Leg number (0-5)
 * @return Index in wave order (0-5), determines when it lifts
 *
 * EXAMPLE:
 *   Leg 4 (BL) → first to lift → wave_index = 0
 *   Leg 0 (FL) → fifth to lift → wave_index = 4
 */
inline uint8_t GetWaveIndex(uint8_t leg_id) {
    for (uint8_t i = 0; i < 6; i++) {
        if (WAVE_ORDER[i] == leg_id) {
            return i;
        }
    }
    return 0;  // Fallback
}

/**
 * Determine if a leg is swinging at given phase.
 * Based on its position in wave order, not leg ID directly.
 *
 * @param leg_id Leg number (0-5)
 * @param phase Global phase (0.0 to 1.0)
 * @return true if leg is currently airborne
 */
inline bool IsLegSwinging(uint8_t leg_id, float phase) {
    uint8_t wave_index = GetWaveIndex(leg_id);
    float swing_start = wave_index * LEG_SWING_FRACTION;
    float swing_end = swing_start + LEG_SWING_FRACTION;
    return (phase >= swing_start) && (phase < swing_end);
}

/**
 * Get local phase (0-1) within this leg's swing window.
 *
 * @param leg_id Leg number (0-5)
 * @param phase Global phase (0.0 to 1.0)
 * @return Local phase 0-1 (for swing), or stance progress 0-1
 */
inline float GetLegLocalPhase(uint8_t leg_id, float phase) {
    uint8_t wave_index = GetWaveIndex(leg_id);
    float swing_start = wave_index * LEG_SWING_FRACTION;

    if (IsLegSwinging(leg_id, phase)) {
        return (phase - swing_start) / LEG_SWING_FRACTION;
    } else {
        float swing_end = swing_start + LEG_SWING_FRACTION;
        float stance_phase;

        if (phase < swing_start) {
            stance_phase = phase / swing_start;
        } else {
            stance_phase = (phase - swing_end) / (1.0f - swing_end);
        }

        return stance_phase;
    }
}

/**
 * Compute X foot position during wave gait.
 * Same algorithm as ripple, but using wave_index for timing.
 */
inline float ComputeFootX(uint8_t leg_id, float phase, float direction) {
    bool is_swinging = IsLegSwinging(leg_id, phase);
    float local_phase = GetLegLocalPhase(leg_id, phase);

    float x_offset;
    if (is_swinging) {
        x_offset = -STRIDE_LENGTH_MM / 2.0f + local_phase * STRIDE_LENGTH_MM;
    } else {
        x_offset = STRIDE_LENGTH_MM / 2.0f - local_phase * STRIDE_LENGTH_MM;
    }

    return x_offset * direction;
}

/**
 * Compute Z foot position during wave gait.
 * Higher lift height for terrain clearance.
 */
inline float ComputeFootZ(uint8_t leg_id, float phase) {
    if (IsLegSwinging(leg_id, phase)) {
        float local_phase = GetLegLocalPhase(leg_id, phase);
        float lift = std::sin(local_phase * M_PI) * LIFT_HEIGHT_MM;
        return NEUTRAL_Z_OFFSET_MM + lift;
    } else {
        return NEUTRAL_Z_OFFSET_MM;
    }
}

/**
 * Compute Y foot position (same as other gaits).
 */
inline float ComputeFootY(uint8_t leg_id) {
    constexpr float Y_OFFSET = 60.0f;
    return (leg_id % 2 == 0) ? Y_OFFSET : -Y_OFFSET;
}

/**
 * Full foot position for wave gait.
 */
inline void ComputeFootPosition(uint8_t leg_id, float phase, float direction,
                                 float& x_out, float& y_out, float& z_out) {
    constexpr float X_NEUTRAL = 80.0f;

    x_out = X_NEUTRAL + ComputeFootX(leg_id, phase, direction);
    y_out = ComputeFootY(leg_id);
    z_out = ComputeFootZ(leg_id, phase);
}

} // namespace WaveGait
