#pragma once

#include <cstdint>
#include <cmath>

/**
 * TRIPOD GAIT KINEMATICS
 *
 * WHAT IS TRIPOD GAIT?
 *   The tripod gait is the fastest and most stable hexapod walking pattern.
 *   At any moment, 3 legs are on the ground (forming a stable tripod)
 *   while the other 3 legs swing forward through the air.
 *
 * LEG GROUPING:
 *   Group A (swing together): Legs 0, 2, 4 (FL, ML, BL - left diagonal)
 *   Group B (swing together): Legs 1, 3, 5 (FR, MR, BR - right diagonal)
 *
 * PHASE DIAGRAM:
 *   Phase 0.0-0.5: Group A swings (in air), Group B pushes (on ground)
 *   Phase 0.5-1.0: Group B swings (in air), Group A pushes (on ground)
 *
 * CYCLE TIME: 600ms (0.6 seconds)
 *   - Fast enough for quick motion
 *   - Slow enough for servo response time
 *   - Stable tripod support at all times
 *
 * COORDINATE SYSTEM:
 *   X-axis: Forward/backward (+ = forward)
 *   Y-axis: Left/right (+ = right)
 *   Z-axis: Up/down (+ = up)
 *
 * LEG NUMBERING:
 *   0 = FL (Front-Left)
 *   1 = FR (Front-Right)
 *   2 = ML (Mid-Left)
 *   3 = MR (Mid-Right)
 *   4 = BL (Back-Left)
 *   5 = BR (Back-Right)
 */

namespace TripodGait {

// ============ CONSTANTS ============

// Cycle time for tripod gait (milliseconds)
constexpr uint32_t CYCLE_TIME_MS = 600;

// Leg geometry (millimeters)
// WHY these values?
//   Based on MG90S servo horn length + typical hexapod leg design
constexpr float COXA_LENGTH_MM = 30.0f;   // Hip segment
constexpr float FEMUR_LENGTH_MM = 50.0f;  // Upper leg
constexpr float TIBIA_LENGTH_MM = 70.0f;  // Lower leg

// Stride parameters (millimeters)
// WHY 40mm stride?
//   - Large enough for visible motion
//   - Small enough to avoid servo limits
//   - Tested value for MG90S servos
constexpr float STRIDE_LENGTH_MM = 40.0f;  // How far foot moves forward/back
constexpr float LIFT_HEIGHT_MM = 20.0f;    // How high foot lifts during swing

// Neutral foot position (relative to body center, millimeters)
// WHY these positions?
//   - Forms stable hexagon around body
//   - Equal weight distribution
//   - Avoids leg collisions
constexpr float NEUTRAL_X_OFFSET_MM = 80.0f;  // Forward/back from center
constexpr float NEUTRAL_Y_OFFSET_MM = 60.0f;  // Left/right from center
constexpr float NEUTRAL_Z_OFFSET_MM = -80.0f; // Down from body (standing height)

// ============ HELPER FUNCTIONS ============

/**
 * Determine if a leg is in Group A (legs 0,2,4) or Group B (legs 1,3,5).
 *
 * @param leg_id Leg number (0-5)
 * @return true if leg is in Group A (even numbered legs)
 *
 * WHY this grouping?
 *   Even legs (0,2,4) form left diagonal
 *   Odd legs (1,3,5) form right diagonal
 *   This creates alternating tripod support
 */
inline bool IsGroupA(uint8_t leg_id) {
    return (leg_id % 2) == 0;
}

/**
 * Compute foot X position during gait cycle.
 *
 * @param phase Normalized gait phase (0.0 to 1.0)
 * @param is_group_a Is this leg in Group A?
 * @param direction 1.0=forward, -1.0=backward
 * @return X position in millimeters (relative to neutral)
 *
 * ALGORITHM:
 *   Swing phase (leg in air):
 *     - Foot moves from back to front quickly
 *     - X goes from -STRIDE/2 to +STRIDE/2
 *
 *   Stance phase (leg on ground):
 *     - Foot pushes body forward (foot moves backward relative to body)
 *     - X goes from +STRIDE/2 to -STRIDE/2
 *
 * WHY this pattern?
 *   During stance, foot is stationary on ground, body moves forward
 *   During swing, foot moves forward through air to next position
 */
inline float ComputeFootX(float phase, bool is_group_a, float direction) {
    // Determine if this leg is currently swinging or in stance
    bool is_swinging;
    float leg_phase;  // Phase within this leg's cycle (0-1)

    if (is_group_a) {
        // Group A swings during phase 0.0-0.5
        is_swinging = (phase < 0.5f);
        leg_phase = is_swinging ? (phase * 2.0f) : ((phase - 0.5f) * 2.0f);
    } else {
        // Group B swings during phase 0.5-1.0
        is_swinging = (phase >= 0.5f);
        leg_phase = is_swinging ? ((phase - 0.5f) * 2.0f) : (phase * 2.0f);
    }

    float x_offset;
    if (is_swinging) {
        // Swing: move from back (-STRIDE/2) to front (+STRIDE/2)
        x_offset = -STRIDE_LENGTH_MM / 2.0f + leg_phase * STRIDE_LENGTH_MM;
    } else {
        // Stance: move from front (+STRIDE/2) to back (-STRIDE/2)
        x_offset = STRIDE_LENGTH_MM / 2.0f - leg_phase * STRIDE_LENGTH_MM;
    }

    // Apply direction (forward = +1, backward = -1)
    return x_offset * direction;
}

/**
 * Compute foot Y position (left/right offset).
 *
 * @param leg_id Leg number (0-5)
 * @return Y position in millimeters
 *
 * WHY constant Y?
 *   For forward/backward walking, legs don't move left/right
 *   Y position is determined by leg mounting position on body
 *
 * LEG POSITIONS:
 *   Left legs (0,2,4): Y = +NEUTRAL_Y_OFFSET_MM
 *   Right legs (1,3,5): Y = -NEUTRAL_Y_OFFSET_MM
 */
inline float ComputeFootY(uint8_t leg_id) {
    // Left legs: positive Y
    // Right legs: negative Y
    return (leg_id % 2 == 0) ? NEUTRAL_Y_OFFSET_MM : -NEUTRAL_Y_OFFSET_MM;
}

/**
 * Compute foot Z position (height) during gait cycle.
 *
 * @param phase Normalized gait phase (0.0 to 1.0)
 * @param is_group_a Is this leg in Group A?
 * @return Z position in millimeters (negative = below body)
 *
 * ALGORITHM:
 *   Swing phase: Foot lifts up in smooth arc
 *     - Starts at ground level (NEUTRAL_Z)
 *     - Rises to NEUTRAL_Z + LIFT_HEIGHT
 *     - Returns to ground level
 *     - Uses sine curve for smooth motion
 *
 *   Stance phase: Foot stays on ground
 *     - Z = NEUTRAL_Z (constant)
 *
 * WHY sine curve for lift?
 *   - Smooth acceleration/deceleration
 *   - Natural-looking motion
 *   - Avoids jerky servo movements
 */
inline float ComputeFootZ(float phase, bool is_group_a) {
    bool is_swinging;
    float leg_phase;

    if (is_group_a) {
        is_swinging = (phase < 0.5f);
        leg_phase = is_swinging ? (phase * 2.0f) : ((phase - 0.5f) * 2.0f);
    } else {
        is_swinging = (phase >= 0.5f);
        leg_phase = is_swinging ? ((phase - 0.5f) * 2.0f) : (phase * 2.0f);
    }

    if (is_swinging) {
        // Lift foot in smooth arc using sine
        // sin(0) = 0, sin(π/2) = 1, sin(π) = 0
        // Maps leg_phase 0→1 to height 0→LIFT_HEIGHT→0
        float lift = std::sin(leg_phase * M_PI) * LIFT_HEIGHT_MM;
        return NEUTRAL_Z_OFFSET_MM + lift;
    } else {
        // Stance: foot on ground
        return NEUTRAL_Z_OFFSET_MM;
    }
}

/**
 * Compute all 3 foot coordinates for a leg at given phase.
 *
 * @param leg_id Leg number (0-5)
 * @param phase Normalized gait phase (0.0 to 1.0)
 * @param direction 1.0=forward, -1.0=backward, 0.0=left/right
 * @param x_out Output: X coordinate (mm)
 * @param y_out Output: Y coordinate (mm)
 * @param z_out Output: Z coordinate (mm)
 *
 * WHY separate function?
 *   Combines X, Y, Z calculations into one call
 *   Easier to use from main gait generator
 */
inline void ComputeFootPosition(uint8_t leg_id, float phase, float direction,
                                 float& x_out, float& y_out, float& z_out) {
    bool is_group_a = IsGroupA(leg_id);

    x_out = NEUTRAL_X_OFFSET_MM + ComputeFootX(phase, is_group_a, direction);
    y_out = ComputeFootY(leg_id);
    z_out = ComputeFootZ(phase, is_group_a);
}

/**
 * Inverse Kinematics: Convert foot position (X,Y,Z) to servo angles.
 *
 * @param x Foot X position (mm)
 * @param y Foot Y position (mm)
 * @param z Foot Z position (mm)
 * @param coxa_angle Output: Hip rotation angle (degrees)
 * @param femur_angle Output: Shoulder angle (degrees)
 * @param tibia_angle Output: Knee angle (degrees)
 * @return true if IK solution found, false if position unreachable
 *
 * ALGORITHM (3-DOF leg IK):
 *   1. Coxa angle: atan2(y, x) - rotation to point toward target
 *   2. Project to 2D plane (distance from hip, height)
 *   3. Use law of cosines to solve femur/tibia angles
 *
 * WHY this approach?
 *   - Standard robotics IK for 3-link arm
 *   - Closed-form solution (fast, no iteration)
 *   - Works for all reachable positions
 *
 * SERVO MAPPING:
 *   Coxa: 0-180° (90° = straight ahead)
 *   Femur: 0-180° (90° = horizontal)
 *   Tibia: 0-180° (90° = straight)
 */
inline bool InverseKinematics(float x, float y, float z,
                               float& coxa_angle, float& femur_angle, float& tibia_angle) {
    // Step 1: Coxa angle (hip rotation in XY plane)
    // WHY atan2?
    //   Handles all quadrants correctly
    //   Returns angle in radians, convert to degrees
    coxa_angle = std::atan2(y, x) * 180.0f / M_PI;

    // Clamp coxa to servo limits (45° to 135°, centered at 90°)
    if (coxa_angle < 45.0f) coxa_angle = 45.0f;
    if (coxa_angle > 135.0f) coxa_angle = 135.0f;

    // Step 2: Project to 2D (distance from coxa joint, height)
    float horizontal_dist = std::sqrt(x * x + y * y) - COXA_LENGTH_MM;
    float vertical_dist = -z;  // Negative because Z is down

    // Distance from femur joint to foot
    float target_dist = std::sqrt(horizontal_dist * horizontal_dist + vertical_dist * vertical_dist);

    // Check if target is reachable
    // WHY this check?
    //   If target is too far, no IK solution exists
    //   Leg can't stretch beyond femur + tibia length
    float max_reach = FEMUR_LENGTH_MM + TIBIA_LENGTH_MM;
    if (target_dist > max_reach * 0.95f) {  // 95% to avoid singularity
        // Target unreachable, return neutral angles
        coxa_angle = 90.0f;
        femur_angle = 90.0f;
        tibia_angle = 90.0f;
        return false;
    }

    // Step 3: Law of cosines for femur angle
    // WHY law of cosines?
    //   Given 3 sides of triangle, can find angles
    //   Triangle: femur, tibia, target_dist
    float cos_femur = (FEMUR_LENGTH_MM * FEMUR_LENGTH_MM + target_dist * target_dist - TIBIA_LENGTH_MM * TIBIA_LENGTH_MM)
                      / (2.0f * FEMUR_LENGTH_MM * target_dist);

    // Clamp to valid range [-1, 1] to avoid acos domain error
    if (cos_femur < -1.0f) cos_femur = -1.0f;
    if (cos_femur > 1.0f) cos_femur = 1.0f;

    float femur_angle_rad = std::acos(cos_femur);

    // Add angle to horizontal
    float angle_to_target = std::atan2(vertical_dist, horizontal_dist);
    femur_angle = (femur_angle_rad + angle_to_target) * 180.0f / M_PI;

    // Step 4: Law of cosines for tibia angle
    float cos_tibia = (FEMUR_LENGTH_MM * FEMUR_LENGTH_MM + TIBIA_LENGTH_MM * TIBIA_LENGTH_MM - target_dist * target_dist)
                      / (2.0f * FEMUR_LENGTH_MM * TIBIA_LENGTH_MM);

    if (cos_tibia < -1.0f) cos_tibia = -1.0f;
    if (cos_tibia > 1.0f) cos_tibia = 1.0f;

    float tibia_angle_rad = std::acos(cos_tibia);
    tibia_angle = 180.0f - (tibia_angle_rad * 180.0f / M_PI);

    // Clamp to servo limits
    if (femur_angle < 30.0f) femur_angle = 30.0f;
    if (femur_angle > 150.0f) femur_angle = 150.0f;
    if (tibia_angle < 30.0f) tibia_angle = 30.0f;
    if (tibia_angle > 150.0f) tibia_angle = 150.0f;

    return true;
}

} // namespace TripodGait
