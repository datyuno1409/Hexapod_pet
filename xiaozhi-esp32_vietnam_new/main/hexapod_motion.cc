#include "hexapod_motion.h"
#include "hexapod_servo_controller.h"
#include <esp_log.h>
#include <map>
#include <cmath>

#define TAG "HexapodMotion"

HexapodMotion& HexapodMotion::GetInstance() {
    static HexapodMotion instance;
    return instance;
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
    ServoController::GetInstance().SetNeutral();
}

void HexapodMotion::Sit() {
    ESP_LOGI(TAG, "Sit");
    is_moving_ = false;

    std::map<uint8_t, float> sit_pose;
    for (int leg = 0; leg < 6; leg++) {
        sit_pose[leg * 3 + 0] = 90.0f;   // coxa neutral
        sit_pose[leg * 3 + 1] = 45.0f;   // femur forward-down
        sit_pose[leg * 3 + 2] = 135.0f;  // tibia bent down
    }
    ServoController::GetInstance().SetServoAngles(sit_pose);
}

void HexapodMotion::Stop() {
    ESP_LOGI(TAG, "Stop");
    is_moving_ = false;
    ServoController::GetInstance().StopAll();
}

void HexapodMotion::SetSpeed(uint8_t speed) {
    current_speed_ = speed;
}

void HexapodMotion::GenerateWalkingGait(uint8_t speed, bool forward) {
    ESP_LOGI(TAG, "WalkingGait forward=%d speed=%d", forward, speed);

    // Simple static pose: legs alternate between lifted and planted
    // Real dynamic gait is handled by GaitGenerator; this is a fallback pose.
    float step = forward ? 110.0f : 70.0f;  // femur angle: 110=forward, 70=backward

    std::map<uint8_t, float> pose;
    // Group A (FL, ML, BL — even indices): step forward, lifted
    for (int leg : {0, 2, 4}) {
        pose[leg * 3 + 0] = 90.0f;   // coxa
        pose[leg * 3 + 1] = step;    // femur
        pose[leg * 3 + 2] = 60.0f;   // tibia lifted
    }
    // Group B (FR, MR, BR — odd indices): planted, pushing
    for (int leg : {1, 3, 5}) {
        pose[leg * 3 + 0] = 90.0f;   // coxa
        pose[leg * 3 + 1] = 90.0f;   // femur neutral
        pose[leg * 3 + 2] = 120.0f;  // tibia planted
    }
    ServoController::GetInstance().SetServoAngles(pose);
}

void HexapodMotion::GenerateTurningGait(uint8_t /*speed*/, bool left) {
    ESP_LOGI(TAG, "TurningGait left=%d", left);

    std::map<uint8_t, float> pose;
    // Left legs swing more when turning left, right legs when turning right
    for (int leg = 0; leg < 6; leg++) {
        bool is_left = (leg % 2 == 0);
        float coxa_angle = 90.0f + (is_left == left ? 20.0f : -20.0f);
        pose[leg * 3 + 0] = coxa_angle;
        pose[leg * 3 + 1] = 90.0f;
        pose[leg * 3 + 2] = 110.0f;
    }
    ServoController::GetInstance().SetServoAngles(pose);
}

void HexapodMotion::GenerateJumpMotion(uint8_t intensity) {
    ESP_LOGI(TAG, "JumpMotion intensity=%d", intensity);

    // Crouch: compress legs downward
    float crouch = 60.0f + (intensity / 100.0f) * 30.0f;  // 60-90 tibia angle
    std::map<uint8_t, float> crouch_pose;
    for (int leg = 0; leg < 6; leg++) {
        crouch_pose[leg * 3 + 0] = 90.0f;
        crouch_pose[leg * 3 + 1] = 60.0f;
        crouch_pose[leg * 3 + 2] = crouch;
    }
    ServoController::GetInstance().SetServoAngles(crouch_pose);

    // Extend: push legs up — GaitGenerator timer will drive animation
    // No blocking delay; let the 20Hz timer produce the motion smoothly
    std::map<uint8_t, float> extend_pose;
    for (int leg = 0; leg < 6; leg++) {
        extend_pose[leg * 3 + 0] = 90.0f;
        extend_pose[leg * 3 + 1] = 120.0f;  // femur up
        extend_pose[leg * 3 + 2] = 30.0f;   // tibia extended
    }
    ServoController::GetInstance().SetServoAngles(extend_pose);
}

void HexapodMotion::GenerateDanceMotion(uint8_t intensity) {
    ESP_LOGI(TAG, "DanceMotion intensity=%d", intensity);

    float sway = (intensity / 100.0f) * 30.0f;

    std::map<uint8_t, float> pose;
    for (int leg = 0; leg < 6; leg++) {
        bool is_left = (leg % 2 == 0);
        pose[leg * 3 + 0] = 90.0f + (is_left ? sway : -sway);  // coxa sways
        pose[leg * 3 + 1] = 90.0f;
        pose[leg * 3 + 2] = 110.0f;
    }
    ServoController::GetInstance().SetServoAngles(pose);
}
