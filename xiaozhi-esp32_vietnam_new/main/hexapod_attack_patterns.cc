#include "hexapod_attack_patterns.h"
#include "attacks/strike_attack.h"
#include "attacks/lunge_attack.h"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "ATTACK_PATTERNS";

// ============ SINGLETON ============

AttackPatterns& AttackPatterns::GetInstance() {
    static AttackPatterns instance;
    return instance;
}

// ============ CONSTRUCTOR ============

AttackPatterns::AttackPatterns()
    : servo_controller_(nullptr),
      gait_generator_(nullptr),
      current_attack_(STRIKE),
      attack_start_time_ms_(0),
      attack_intensity_(100),
      attack_duration_ms_(0),
      is_attacking_(false),
      was_walking_before_(false) {
    ESP_LOGI(TAG, "AttackPatterns created (singleton)");
}

// ============ LIFECYCLE ============

bool AttackPatterns::Initialize(GaitGenerator* gait_gen, ServoController* servo_ctrl) {
    if (!gait_gen || !servo_ctrl) {
        ESP_LOGE(TAG, "Null pointer in Initialize (gait_gen=%p, servo_ctrl=%p)",
                 gait_gen, servo_ctrl);
        return false;
    }

    gait_generator_ = gait_gen;
    servo_controller_ = servo_ctrl;

    ESP_LOGI(TAG, "AttackPatterns initialized with GaitGenerator + ServoController");
    return true;
}

void AttackPatterns::Shutdown() {
    if (is_attacking_) {
        StopAttack();
    }
    gait_generator_ = nullptr;
    servo_controller_ = nullptr;
    ESP_LOGI(TAG, "AttackPatterns shutdown");
}

// ============ ATTACK COMMANDS ============

void AttackPatterns::ExecuteAttack(AttackType attack, uint8_t intensity) {
    // Clamp intensity to valid range
    attack_intensity_ = (intensity > 100) ? 100 : intensity;

    // If already attacking, cancel first then restart
    if (is_attacking_) {
        ESP_LOGW(TAG, "Cancelling previous attack to start new one");
        CancelAttack();
    }

    // Remember if gait was running so we can resume after attack
    was_walking_before_ = gait_generator_->IsWalking();

    // Pause walking during attack
    // WHY Pause instead of Stop?
    //   Pause preserves gait state (type, speed, direction)
    //   So we can Resume exactly where we left off
    if (was_walking_before_) {
        gait_generator_->Pause();
        ESP_LOGI(TAG, "GaitGenerator paused for attack");
    }

    // Set attack parameters
    current_attack_ = attack;
    attack_start_time_ms_ = esp_timer_get_time() / 1000;  // microseconds → milliseconds

    // Set attack duration based on type
    // WHY different durations?
    //   Strike: Quick jab needs only 600ms
    //   Lunge: Forward charge needs 1100ms for full effect
    switch (attack) {
        case STRIKE:
            attack_duration_ms_ = StrikeAttack::TOTAL_DURATION_MS;  // 600ms
            ESP_LOGI(TAG, "Strike attack started, intensity=%d, duration=%ldms",
                     attack_intensity_, attack_duration_ms_);
            break;
        case LUNGE:
            attack_duration_ms_ = LungeAttack::TOTAL_DURATION_MS;   // 1100ms
            ESP_LOGI(TAG, "Lunge attack started, intensity=%d, duration=%ldms",
                     attack_intensity_, attack_duration_ms_);
            break;
        default:
            attack_duration_ms_ = 600;
            break;
    }

    is_attacking_ = true;
}

void AttackPatterns::StopAttack() {
    if (!is_attacking_) {
        return;
    }

    ESP_LOGI(TAG, "Attack stopped manually");
    CompleteAttack();
}

// ============ STATUS ============

uint32_t AttackPatterns::GetAttackElapsedMs() const {
    if (!is_attacking_) {
        return 0;
    }
    uint32_t now_ms = esp_timer_get_time() / 1000;
    return now_ms - attack_start_time_ms_;
}

// ============ MAIN UPDATE ============

void AttackPatterns::UpdateFrame() {
    // Nothing to do if not attacking
    if (!is_attacking_) {
        return;
    }

    uint32_t elapsed_ms = GetAttackElapsedMs();

    // Check if attack has finished
    if (elapsed_ms >= attack_duration_ms_) {
        ESP_LOGI(TAG, "Attack completed (elapsed %ldms >= duration %ldms)",
                 elapsed_ms, attack_duration_ms_);
        CompleteAttack();
        return;
    }

    // Compute servo angles for current frame
    float angles[18];
    // Initialize all to neutral as safety default
    for (int i = 0; i < 18; i++) {
        angles[i] = 90.0f;
    }

    // Delegate to attack-specific function
    switch (current_attack_) {
        case STRIKE:
            ComputeStrikeFrame(elapsed_ms, angles);
            break;
        case LUNGE:
            ComputeLungeFrame(elapsed_ms, angles);
            break;
    }

    // Send computed angles to servo controller (direct array)
    servo_controller_->SetServoAngles(angles);
}

// ============ INTERNAL HELPERS ============

void AttackPatterns::CompleteAttack() {
    is_attacking_ = false;

    // Return all servos to neutral position
    servo_controller_->SetNeutral();

    // Resume walking if it was active before attack
    // WHY check was_walking_before_?
    //   If robot was walking before attack, resume walking after
    //   If robot was standing still, don't start walking
    if (was_walking_before_ && gait_generator_) {
        gait_generator_->Resume();
        ESP_LOGI(TAG, "GaitGenerator resumed after attack");
    }

    ESP_LOGI(TAG, "Attack completed, returned to neutral");
}

void AttackPatterns::ComputeStrikeFrame(uint32_t elapsed_ms, float angles[18]) {
    // Normalize elapsed time to 0.0-1.0
    float global_progress = (float)elapsed_ms / (float)StrikeAttack::TOTAL_DURATION_MS;
    if (global_progress > 1.0f) global_progress = 1.0f;

    // Delegate to StrikeAttack kinematics
    StrikeAttack::ComputeStrikeFrame(global_progress, attack_intensity_, angles);
}

void AttackPatterns::ComputeLungeFrame(uint32_t elapsed_ms, float angles[18]) {
    // LungeAttack takes elapsed_ms directly
    LungeAttack::ComputeLungeFrame(elapsed_ms, attack_intensity_, angles);
}
