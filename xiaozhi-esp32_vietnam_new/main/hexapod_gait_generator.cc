#include "hexapod_gait_generator.h"
#include "gaits/tripod_gait.h"
#include "gaits/ripple_gait.h"
#include "gaits/wave_gait.h"
#include "hexapod_constants.h"
#include <cmath>
#include <esp_log.h>

static const char* TAG = "GAIT_GENERATOR";

// ============ SINGLETON IMPLEMENTATION ============

GaitGenerator& GaitGenerator::GetInstance() {
    static GaitGenerator instance;
    return instance;
}

// ============ CONSTRUCTOR ============

GaitGenerator::GaitGenerator()
    : servo_controller_(nullptr),
      current_gait_(TRIPOD),
      current_dir_(FORWARD),
      speed_percent_(50),
      motion_start_time_(0),
      motion_duration_ms_(0),
      gait_cycle_time_ms_(600),  // Default: tripod
      is_walking_(false),
      is_paused_(false) {
    ESP_LOGI(TAG, "GaitGenerator created (singleton)");
}

// ============ LIFECYCLE ============

bool GaitGenerator::Initialize(ServoController* servo_ctrl) {
    if (!servo_ctrl) {
        ESP_LOGE(TAG, "ServoController pointer is null");
        return false;
    }

    servo_controller_ = servo_ctrl;
    ESP_LOGI(TAG, "GaitGenerator initialized with ServoController");

    // Move all legs to neutral (standing) position
    servo_controller_->SetNeutral();

    return true;
}

void GaitGenerator::Shutdown() {
    if (is_walking_) {
        Stop();
    }
    servo_controller_ = nullptr;
    ESP_LOGI(TAG, "GaitGenerator shutdown");
}

// ============ HIGH-LEVEL MOTION COMMANDS ============

void GaitGenerator::Walk(GaitType gait, uint8_t speed, Direction dir, uint32_t duration_ms) {
    // Clamp speed to 0-100
    speed_percent_ = (speed > HexapodConst::MAX_SPEED) ? HexapodConst::MAX_SPEED : speed;

    // Set gait parameters
    current_gait_ = gait;
    current_dir_ = dir;

    // Set cycle time based on gait type
    // WHY different cycle times?
    //   TRIPOD: 600ms (fastest, stable)
    //   RIPPLE: 1000ms (balanced)
    //   WAVE: 1500ms (slowest, most stable)
    switch (gait) {
        case TRIPOD:
            gait_cycle_time_ms_ = HexapodConst::TRIPOD_CYCLE_TIME_MS;
            break;
        case RIPPLE:
            gait_cycle_time_ms_ = HexapodConst::RIPPLE_CYCLE_TIME_MS;
            break;
        case WAVE:
            gait_cycle_time_ms_ = HexapodConst::WAVE_CYCLE_TIME_MS;
            break;
        default:
            gait_cycle_time_ms_ = HexapodConst::TRIPOD_CYCLE_TIME_MS;
    }

    // Start motion
    motion_start_time_ = esp_timer_get_time() / 1000;  // Convert microseconds to milliseconds
    motion_duration_ms_ = duration_ms;
    is_walking_ = true;
    is_paused_ = false;

    ESP_LOGI(TAG, "Walk started: gait=%d, speed=%d%%, dir=%d, duration=%ldms",
             gait, speed_percent_, dir, duration_ms);
}

void GaitGenerator::Stop() {
    if (!is_walking_) {
        return;
    }

    is_walking_ = false;
    is_paused_ = false;

    // Return to neutral position
    servo_controller_->SetNeutral();

    ESP_LOGI(TAG, "Motion stopped, returned to neutral");
}

void GaitGenerator::Pause() {
    if (is_walking_ && !is_paused_) {
        is_paused_ = true;
        ESP_LOGI(TAG, "Motion paused");
    }
}

void GaitGenerator::Resume() {
    if (is_walking_ && is_paused_) {
        is_paused_ = false;
        // Adjust motion_start_time to account for pause duration
        // WHY: We don't want the pause duration to shift the gait phase
        motion_start_time_ = esp_timer_get_time() / 1000 - GetElapsedTime();
        ESP_LOGI(TAG, "Motion resumed");
    }
}

// ============ STATUS QUERIES ============

uint32_t GaitGenerator::GetElapsedTime() const {
    if (!is_walking_) {
        return 0;
    }
    uint32_t now_ms = esp_timer_get_time() / 1000;
    return now_ms - motion_start_time_;
}

// ============ MAIN UPDATE FUNCTION ============

void GaitGenerator::Update() {
    // If not walking or paused, nothing to do
    if (!is_walking_ || is_paused_) {
        return;
    }

    uint64_t start_us = esp_timer_get_time();

    uint32_t elapsed = GetElapsedTime();

    // Check if motion duration has expired
    if (elapsed >= motion_duration_ms_) {
        Stop();
        return;
    }

    // Update gait phase and compute servo angles
    UpdateGaitPhase();
    ComputeServoAngles();

    // Send commands to servo controller
    servo_controller_->SetServoAngles(target_angles_.data());

    // Performance tracking
    uint64_t end_us = esp_timer_get_time();
    last_compute_us_ = end_us - start_us;
    if (last_compute_us_ > max_compute_us_) {
        max_compute_us_ = last_compute_us_;
    }
    compute_count_++;

    // Log periodic stats (every 100 updates)
    if (compute_count_ % 100 == 0) {
        ESP_LOGI(TAG, "Perf: gait_update=%.1fms (max=%.1fms) count=%u",
                 last_compute_us_ / 1000.0, max_compute_us_ / 1000.0, compute_count_);
    }
}

// ============ INTERNAL COMPUTATION ============

void GaitGenerator::UpdateGaitPhase() {
    // Phase calculation is done directly in ComputeServoAngles() via modulo.
    // This function is a hook for future per-gait phase pre-processing.
}

void GaitGenerator::ComputeServoAngles() {
    uint32_t elapsed = GetElapsedTime();
    uint32_t phase_time_ms = elapsed % gait_cycle_time_ms_;
    float phase_0_to_1 = (float)phase_time_ms / (float)gait_cycle_time_ms_;

    // Direction multiplier: forward=+1, backward=-1
    float dir = (current_dir_ == FORWARD) ? 1.0f : -1.0f;

    float speed_scale = speed_percent_ / 100.0f;

    for (int leg = 0; leg < 6; leg++) {
        float x, y, z;

        // Compute foot position using gait-specific kinematics
        switch (current_gait_) {
            case TRIPOD:
                TripodGait::ComputeFootPosition(leg, phase_0_to_1, dir, x, y, z);
                break;
            case RIPPLE:
                RippleGait::ComputeFootPosition(leg, phase_0_to_1, dir, x, y, z);
                break;
            case WAVE:
                WaveGait::ComputeFootPosition(leg, phase_0_to_1, dir, x, y, z);
                break;
            default:
                TripodGait::ComputeFootPosition(leg, phase_0_to_1, dir, x, y, z);
                break;
        }

        // Scale foot displacement from neutral by speed
        // WHY scale displacement not position?
        //   We want reduced MOVEMENT range, not reduced absolute position
        //   At 50% speed: half stride length, same neutral position
        float neutral_x = TripodGait::NEUTRAL_X_OFFSET_MM;
        float neutral_z = TripodGait::NEUTRAL_Z_OFFSET_MM;
        x = neutral_x + (x - neutral_x) * speed_scale;
        z = neutral_z + (z - neutral_z) * speed_scale;

        // Compute IK: (x, y, z) → servo angles
        float coxa, femur, tibia;
        bool reachable = TripodGait::InverseKinematics(x, y, z, coxa, femur, tibia);

        if (!reachable) {
            // If target unreachable, use neutral angles
            coxa = HexapodConst::NEUTRAL_ANGLE_COXA;
            femur = HexapodConst::NEUTRAL_ANGLE_FEMUR;
            tibia = HexapodConst::NEUTRAL_ANGLE_TIBIA;
        }

        int servo_base = leg * 3;
        target_angles_[servo_base + 0] = coxa;
        target_angles_[servo_base + 1] = femur;
        target_angles_[servo_base + 2] = tibia;
    }
}

LegPose GaitGenerator::ComputeLegIK(int leg_id, float phase_0_to_1) {
    LegPose pose;

    // Get gait-specific phase
    float leg_phase = phase_0_to_1;

    switch (current_gait_) {
        case TRIPOD:
            leg_phase = GetTripodPhase(phase_0_to_1);
            break;
        case RIPPLE:
            leg_phase = GetRipplePhase(phase_0_to_1);
            break;
        case WAVE:
            leg_phase = GetWavePhase(phase_0_to_1);
            break;
    }

    // Determine if leg is swinging or pushing
    // For now, simplified tripod logic
    // (Full gait kinematics in dedicated gait files)

    bool is_swinging = (leg_phase > 0.5f);  // Simplified: first half push, second half swing

    if (is_swinging) {
        // Swing phase: leg moves through air
        // WHY sine wave?
        //   Smooth acceleration (starts slow, peaks, ends slow)
        //   Natural-looking motion like a pendulum
        //   Math: sin(0)=0, sin(π/2)=1, sin(π)=0

        float swing_local_phase = (leg_phase - 0.5f) * 2.0f;  // Normalize 0.5-1.0 to 0-1

        // Forward/backward swing
        float swing_forward = HexapodConst::SWING_AMPLITUDE_DEG * std::sin(swing_local_phase * M_PI);
        // Height swing
        float swing_height = HexapodConst::SWING_AMPLITUDE_DEG * std::sin(swing_local_phase * M_PI);

        pose.coxa = HexapodConst::NEUTRAL_ANGLE_COXA;                      // No hip rotation in swing
        pose.femur = HexapodConst::NEUTRAL_ANGLE_FEMUR + swing_forward;    // Forward swing
        pose.tibia = HexapodConst::NEUTRAL_ANGLE_TIBIA - swing_height;     // Knee lifts during swing
    } else {
        // Stance phase: leg on ground, pushes body
        float stance_local_phase = leg_phase * 2.0f;  // Normalize 0-0.5 to 0-1

        // Push backward
        float push_back = -HexapodConst::STANCE_AMPLITUDE_DEG * std::sin(stance_local_phase * M_PI);

        pose.coxa = HexapodConst::NEUTRAL_ANGLE_COXA;                      // No hip rotation in stance
        pose.femur = HexapodConst::NEUTRAL_ANGLE_FEMUR + push_back;        // Push backward slightly
        pose.tibia = HexapodConst::NEUTRAL_ANGLE_TIBIA + HexapodConst::STANCE_AMPLITUDE_DEG;   // Slightly bent for stability
    }

    return pose;
}

float GaitGenerator::GetTripodPhase(float norm_time) {
    // WHY tripod phase?
    //   Tripod: 3 legs swing, 3 push, alternating
    //   Phase 0.0-0.5: Legs 0,2,4 swing
    //   Phase 0.5-1.0: Legs 1,3,5 swing
    //
    // For leg 0 (even): swings in second half (0.5-1.0)
    // For leg 1 (odd): swings in first half (0.0-0.5)

    // Return the normalized time as-is for now
    // (Leg-specific phase adjustment happens in ComputeLegIK)
    return norm_time;
}

float GaitGenerator::GetRipplePhase(float norm_time) {
    // WHY ripple phase?
    //   Ripple: Each leg lifts sequentially
    //   Leg 0: 0.0-0.167
    //   Leg 1: 0.167-0.333
    //   ... etc
    //
    // For now, return as-is. Full implementation in ripple_gait.h

    return norm_time;
}

float GaitGenerator::GetWavePhase(float norm_time) {
    // WHY wave phase?
    //   Wave: Sequential wave from rear to front
    //   Rear leg (leg 4/5) lifts first
    //
    // For now, return as-is. Full implementation in wave_gait.h

    return norm_time;
}

float GaitGenerator::EaseInOutQuad(float t) {
    // WHY easing function?
    //   Raw linear motion (0→1) looks jerky
    //   Easing curves make motion smooth
    //
    // Quadratic ease-in-out:
    //   Accelerates at start, decelerates at end
    //   Smooth acceleration throughout

    if (t < 0.5f) {
        // First half: ease in (accelerate)
        return 2.0f * t * t;
    } else {
        // Second half: ease out (decelerate)
        t = t - 1.0f;
        return -1.0f + (2.0f - 2.0f * t * t);
    }
}
