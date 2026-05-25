#include "hexapod_motion.h"
#include "hexapod_servo_controller.h"
#include "hexapod_constants.h"
#include <esp_log.h>
#include <map>
#include <cmath>

#define TAG "HexapodMotion"

HexapodMotion& HexapodMotion::GetInstance() {
    static HexapodMotion instance;
    // Auto-initialize with default ServoController if not explicitly injected
    if (instance.servo_controller_ == nullptr) {
        instance.servo_controller_ = &ServoController::GetInstance();
    }
    return instance;
}

void HexapodMotion::Init(ServoController& servo_ctrl) {
    GetInstance().servo_controller_ = &servo_ctrl;
}

HexapodMotion::HexapodMotion() {}

void HexapodMotion::MoveForward(uint8_t speed, uint32_t /*duration_ms*/) {
    ESP_LOGI(TAG, "MoveForward speed=%d", speed);
    current_speed_ = speed;
    is_moving_ = true;
    GenerateWalkingGait(speed, true);
}

void HexapodMotion::MoveBackward(uint8_t speed, uint32_t /*duration_ms*/) {
    ESP_LOGI(TAG, "MoveBackward speed=%d", speed);
    current_speed_ = speed;
    is_moving_ = true;
    GenerateWalkingGait(speed, false);
}

void HexapodMotion::TurnLeft(uint8_t speed, uint32_t /*duration_ms*/) {
    ESP_LOGI(TAG, "TurnLeft speed=%d", speed);
    current_speed_ = speed;
    is_moving_ = true;
    GenerateTurningGait(speed, true);
}

void HexapodMotion::TurnRight(uint8_t speed, uint32_t /*duration_ms*/) {
    ESP_LOGI(TAG, "TurnRight speed=%d", speed);
    current_speed_ = speed;
    is_moving_ = true;
    GenerateTurningGait(speed, false);
}

void HexapodMotion::Jump(uint8_t intensity) {
    ESP_LOGI(TAG, "Jump intensity=%d", intensity);
    GenerateJumpMotion(intensity);
}

void HexapodMotion::Dance(uint8_t intensity, uint32_t /*duration_ms*/) {
    ESP_LOGI(TAG, "Dance intensity=%d", intensity);
    is_moving_ = true;
    GenerateDanceMotion(intensity);
}

void HexapodMotion::Stand() {
    ESP_LOGI(TAG, "Stand");
    is_moving_ = false;
    servo_controller_->SetNeutral();
}

void HexapodMotion::Sit() {
    ESP_LOGI(TAG, "Sit");
    is_moving_ = false;

    float sit_pose[HexapodConst::NUM_SERVOS];
    for (int leg = 0; leg < 6; leg++) {
        sit_pose[leg * 3 + 0] = HexapodConst::NEUTRAL_ANGLE_COXA;   // coxa neutral
        sit_pose[leg * 3 + 1] = 45.0f;   // femur forward-down
        sit_pose[leg * 3 + 2] = 135.0f;  // tibia bent down
    }
    servo_controller_->SetServoAngles(sit_pose);
}

void HexapodMotion::Stop() {
    ESP_LOGI(TAG, "Stop");
    is_moving_ = false;
    servo_controller_->StopAll();
}

void HexapodMotion::SetSpeed(uint8_t speed) {
    current_speed_ = speed;
}

void HexapodMotion::GenerateWalkingGait(uint8_t speed, bool forward) {
    ESP_LOGD(TAG, "WalkingGait forward=%d speed=%d", forward, speed);

    // Simple static pose: legs alternate between lifted and planted
    // Real dynamic gait is handled by GaitGenerator; this is a fallback pose.
    float step = forward ? HexapodConst::FEMUR_ANGLE_FORWARD
                         : HexapodConst::FEMUR_ANGLE_BACKWARD;  // femur angle: 110=forward, 70=backward

    float pose[HexapodConst::NUM_SERVOS];
    // Group A (FL, ML, BL — even indices): step forward, lifted
    for (int leg : {0, 2, 4}) {
        pose[leg * 3 + 0] = HexapodConst::NEUTRAL_ANGLE_COXA;   // coxa
        pose[leg * 3 + 1] = step;                                // femur
        pose[leg * 3 + 2] = HexapodConst::TIBIA_ANGLE_LIFTED;   // tibia lifted
    }
    // Group B (FR, MR, BR — odd indices): planted, pushing
    for (int leg : {1, 3, 5}) {
        pose[leg * 3 + 0] = HexapodConst::NEUTRAL_ANGLE_COXA;   // coxa
        pose[leg * 3 + 1] = HexapodConst::NEUTRAL_ANGLE_FEMUR;  // femur neutral
        pose[leg * 3 + 2] = HexapodConst::TIBIA_ANGLE_PLANTED;  // tibia planted
    }
    servo_controller_->SetServoAngles(pose);
}

void HexapodMotion::GenerateTurningGait(uint8_t /*speed*/, bool left) {
    ESP_LOGD(TAG, "TurningGait left=%d", left);

    float pose[HexapodConst::NUM_SERVOS];
    // Left legs swing more when turning left, right legs when turning right
    for (int leg = 0; leg < 6; leg++) {
        bool is_left = (leg % 2 == 0);
        float coxa_angle = HexapodConst::NEUTRAL_ANGLE_COXA + (is_left == left ? HexapodConst::COXA_TURN_OFFSET_DEG
                                                                             : -HexapodConst::COXA_TURN_OFFSET_DEG);
        pose[leg * 3 + 0] = coxa_angle;
        pose[leg * 3 + 1] = HexapodConst::FEMUR_ANGLE_TURN;
        pose[leg * 3 + 2] = HexapodConst::TIBIA_ANGLE_TURN;
    }
    servo_controller_->SetServoAngles(pose);
}

void HexapodMotion::GenerateJumpMotion(uint8_t intensity) {
    ESP_LOGD(TAG, "JumpMotion intensity=%d", intensity);

    // Crouch: compress legs downward
    // Range: TIBIA_ANGLE_CROUCH_MIN (60°) to TIBIA_ANGLE_CROUCH_MAX (90°)
    float crouch = HexapodConst::TIBIA_ANGLE_CROUCH_MIN +
                   (intensity / 100.0f) * (HexapodConst::TIBIA_ANGLE_CROUCH_MAX - HexapodConst::TIBIA_ANGLE_CROUCH_MIN);
    float crouch_pose[HexapodConst::NUM_SERVOS];
    for (int leg = 0; leg < 6; leg++) {
        crouch_pose[leg * 3 + 0] = HexapodConst::NEUTRAL_ANGLE_COXA;
        crouch_pose[leg * 3 + 1] = HexapodConst::FEMUR_ANGLE_CROUCH;
        crouch_pose[leg * 3 + 2] = crouch;
    }
    servo_controller_->SetServoAngles(crouch_pose);

    // Extend: push legs up — GaitGenerator timer will drive animation
    // No blocking delay; let the 20Hz timer produce the motion smoothly
    float extend_pose[HexapodConst::NUM_SERVOS];
    for (int leg = 0; leg < 6; leg++) {
        extend_pose[leg * 3 + 0] = HexapodConst::NEUTRAL_ANGLE_COXA;
        extend_pose[leg * 3 + 1] = HexapodConst::FEMUR_ANGLE_UP;   // femur up
        extend_pose[leg * 3 + 2] = HexapodConst::TIBIA_ANGLE_EXTENDED;   // tibia extended
    }
    servo_controller_->SetServoAngles(extend_pose);
}

void HexapodMotion::GenerateDanceMotion(uint8_t intensity) {
    ESP_LOGD(TAG, "DanceMotion intensity=%d", intensity);

    float sway = (intensity / 100.0f) * HexapodConst::DANCE_COXA_SWAY_MAX_DEG;

    float pose[HexapodConst::NUM_SERVOS];
    for (int leg = 0; leg < 6; leg++) {
        bool is_left = (leg % 2 == 0);
        pose[leg * 3 + 0] = HexapodConst::NEUTRAL_ANGLE_COXA + (is_left ? sway : -sway);  // coxa sways
        pose[leg * 3 + 1] = HexapodConst::NEUTRAL_ANGLE_FEMUR;
        pose[leg * 3 + 2] = HexapodConst::TIBIA_ANGLE_TURN;
    }
    servo_controller_->SetServoAngles(pose);
}
