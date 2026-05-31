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
      is_paused_(false),
	      pause_start_time_(0) {
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

    if (duration_ms == 0 && is_walking_ && !is_paused_ && current_gait_ == gait && current_dir_ == dir) {
        motion_duration_ms_ = 0;
        ESP_LOGD(TAG, "Continuous walk refresh: gait=%d speed=%d%% dir=%d", gait, speed_percent_, dir);
        return;
    }

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
        case BI_GAIT:
            gait_cycle_time_ms_ = HexapodConst::TRIPOD_CYCLE_TIME_MS;
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

    ESP_LOGI(TAG, "==== GAIT GENERATOR: Stop ====");
    ESP_LOGI(TAG, "-> Elapsed time: %lu ms, Target duration: %lu ms", GetElapsedTime(), motion_duration_ms_);

    is_walking_ = false;
    is_paused_ = false;

    // Return to neutral position
    servo_controller_->SetNeutral();

    ESP_LOGI(TAG, "Motion stopped, returned to neutral");
}

void GaitGenerator::Pause() {
    if (is_walking_ && !is_paused_) {
        is_paused_ = true;
	        pause_start_time_ = esp_timer_get_time() / 1000;
        ESP_LOGI(TAG, "Motion paused");
    }
}

void GaitGenerator::Resume() {
    if (is_walking_ && is_paused_) {
        is_paused_ = false;
        // Adjust motion_start_time to account for pause duration
        uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        uint32_t pause_duration = now - pause_start_time_;
        motion_start_time_ += pause_duration;
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
    if (motion_duration_ms_ > 0 && elapsed >= motion_duration_ms_) {
        Stop();
        return;
    }

    // Update gait phase and compute servo angles
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
        // Special case for JUMP: skip IK and use direct angle computation
        if (current_gait_ == JUMP) {
            LegPose pose = ComputeLegIK(leg, phase_0_to_1);
            int servo_base = leg * 3;
            target_angles_[servo_base + 0] = pose.coxa;
            target_angles_[servo_base + 1] = pose.femur;
            target_angles_[servo_base + 2] = pose.tibia;
            continue;
        }

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
            case BI_GAIT:
                TripodGait::ComputeFootPosition(leg, phase_0_to_1, dir, x, y, z);
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
            leg_phase = GetTripodPhase(phase_0_to_1, leg_id);
            break;
        case RIPPLE:
            leg_phase = GetRipplePhase(phase_0_to_1, leg_id);
            break;
        case WAVE:
            leg_phase = GetWavePhase(phase_0_to_1, leg_id);
            break;
        case BI_GAIT:
            leg_phase = GetBiGaitPhase(phase_0_to_1, leg_id);
            break;
        case JUMP:
            leg_phase = GetJumpPhase(phase_0_to_1, leg_id);
            break;
    }

    // Apply IK based on gait
    if (current_gait_ == JUMP) {
        // Jump: crouch (0-0.5), explode upward (0.5-0.7), land absorb (0.7-1.0)
        if (leg_phase < 0.5f) {
            // Crouch: compress legs
            pose.coxa = HexapodConst::NEUTRAL_ANGLE_COXA;
            pose.femur = HexapodConst::FEMUR_ANGLE_CROUCH;     // ~45°
            pose.tibia = HexapodConst::TIBIA_ANGLE_CROUCH_MIN; // ~60° (bent)
        } else if (leg_phase < 0.7f) {
            // Explode upward: extend legs rapidly
            pose.coxa = HexapodConst::NEUTRAL_ANGLE_COXA;
            pose.femur = HexapodConst::NEUTRAL_ANGLE_FEMUR;
            pose.tibia = HexapodConst::TIBIA_ANGLE_EXTENDED;   // ~120° (straight)
        } else {
            // Land absorb: bend knees again to cushion landing
            pose.coxa = HexapodConst::NEUTRAL_ANGLE_COXA;
            pose.femur = HexapodConst::FEMUR_ANGLE_CROUCH;
            pose.tibia = HexapodConst::TIBIA_ANGLE_CROUCH_MIN;
        }
    } else {
        // Existing tripod-based IK for walking/turning
        bool is_swinging = (leg_phase > 0.5f);

        // Compute coxa offset for lateral turning
        float coxa_offset = 0.0f;
        if (current_dir_ == LEFT || current_dir_ == RIGHT) {
            float turn_scale = (current_dir_ == LEFT) ? 1.0f : -1.0f;
            float side = (leg_id % 2 == 0) ? turn_scale : -turn_scale;
            coxa_offset = side * HexapodConst::COXA_TURN_OFFSET_DEG;
        }

        if (is_swinging) {
            float swing_local_phase = (leg_phase - 0.5f) * 2.0f;
            float swing_forward = HexapodConst::SWING_AMPLITUDE_DEG * std::sin(swing_local_phase * M_PI);
            float swing_height = HexapodConst::SWING_HEIGHT_AMPLITUDE_DEG * std::sin(swing_local_phase * M_PI);

            pose.coxa = HexapodConst::NEUTRAL_ANGLE_COXA + coxa_offset;
            pose.femur = HexapodConst::NEUTRAL_ANGLE_FEMUR + swing_forward;
            pose.tibia = HexapodConst::NEUTRAL_ANGLE_TIBIA - swing_height;
        } else {
            float stance_local_phase = leg_phase * 2.0f;
            float push_back = -HexapodConst::STANCE_AMPLITUDE_DEG * std::sin(stance_local_phase * M_PI);

            pose.coxa = HexapodConst::NEUTRAL_ANGLE_COXA + coxa_offset;
            pose.femur = HexapodConst::NEUTRAL_ANGLE_FEMUR + push_back;
            pose.tibia = HexapodConst::NEUTRAL_ANGLE_TIBIA + HexapodConst::STANCE_AMPLITUDE_DEG;
        }
    }

    return pose;
}

float GaitGenerator::GetTripodPhase(float norm_time, int leg_id) {
    // Tripod: Group A (even legs 0,2,4) swing in first half (0-0.5)
    //         Group B (odd legs 1,3,5) swing in second half (0.5-1.0)
    bool is_group_a = (leg_id % 2 == 0);
    if (is_group_a) {
        // Group A swings during 0.0-0.5
        return norm_time < 0.5f ? norm_time * 2.0f : (norm_time - 0.5f) * 2.0f;
    } else {
        // Group B swings during 0.5-1.0
        return norm_time >= 0.5f ? (norm_time - 0.5f) * 2.0f : norm_time * 2.0f;
    }
}

float GaitGenerator::GetRipplePhase(float norm_time, int leg_id) {
    // Ripple: each leg lifts sequentially for 1/6 of cycle
    // Leg i swings during [i/6, (i+1)/6]
    float leg_offset = static_cast<float>(leg_id) / 6.0f;
    float swing_frac = 1.0f / 6.0f;
    float shifted = norm_time - leg_offset;
    if (shifted < 0.0f) shifted += 1.0f;
    // Return phase within this leg's swing window (0-1 during swing)
    return shifted / swing_frac;
}

float GaitGenerator::GetWavePhase(float norm_time, int leg_id) {
    // Wave: sequential from back to front (leg 4,5,2,3,0,1)
    static constexpr int wave_order[6] = {4, 5, 2, 3, 0, 1};
    int wave_index = 0;
    for (int i = 0; i < 6; i++) {
        if (wave_order[i] == leg_id) { wave_index = i; break; }
    }
    float leg_offset = static_cast<float>(wave_index) / 6.0f;
    float swing_frac = 1.0f / 6.0f;
    float shifted = norm_time - leg_offset;
    if (shifted < 0.0f) shifted += 1.0f;
    return shifted / swing_frac;
}

float GaitGenerator::GetBiGaitPhase(float norm_time, int leg_id) {
    return GetTripodPhase(norm_time, leg_id);
}

float GaitGenerator::GetJumpPhase(float norm_time, int leg_id) {
    // Jump is a simple linear phase from 0 to 1
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
