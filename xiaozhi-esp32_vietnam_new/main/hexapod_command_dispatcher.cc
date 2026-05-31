#include "hexapod_command_dispatcher.h"
#include <esp_log.h>

static const char* TAG = "CMD_DISPATCHER";

bool CommandDispatcher::Parse(const cJSON* root, ParsedCommand& out) {
    if (!root) return false;

    cJSON* cmd_item = cJSON_GetObjectItem(root, "cmd");
    if (!cmd_item || !cJSON_IsString(cmd_item)) {
        return false;
    }
    out.cmd = cmd_item->valuestring;

    if (out.cmd == "motion") {
        cJSON* action = cJSON_GetObjectItem(root, "action");
        cJSON* speed = cJSON_GetObjectItem(root, "speed");
        cJSON* duration = cJSON_GetObjectItem(root, "duration_ms");

        out.motion_action = cJSON_IsString(action) ? action->valuestring : "";
        out.speed = cJSON_IsNumber(speed) ? speed->valueint : HexapodConst::DEFAULT_MOTION_SPEED;
        out.duration_ms = cJSON_IsNumber(duration) ? duration->valueint : 0;
    } else if (out.cmd == "emotion") {
        cJSON* emotion = cJSON_GetObjectItem(root, "emotion");
        cJSON* text = cJSON_GetObjectItem(root, "text");

        out.emotion = cJSON_IsString(emotion) ? emotion->valuestring : "neutral";
        out.emotion_text = cJSON_IsString(text) ? text->valuestring : "";
    } else if (out.cmd == "camera") {
        cJSON* action = cJSON_GetObjectItem(root, "action");
        out.camera_action = cJSON_IsString(action) ? action->valuestring : "";
    }

    return true;
}

void CommandDispatcher::Dispatch(const cJSON* root,
                                 MotionHandler motion_handler,
                                 EmotionHandler emotion_handler,
                                 CameraHandler camera_handler,
                                 PingHandler ping_handler,
                                 UnknownHandler unknown_handler) {
    ParsedCommand parsed;
    if (!Parse(root, parsed)) {
        ESP_LOGE(TAG, "Failed to parse command from JSON");
        return;
    }

    if (parsed.cmd == "motion") {
        if (motion_handler) motion_handler(parsed);
    } else if (parsed.cmd == "emotion") {
        if (emotion_handler) emotion_handler(parsed);
    } else if (parsed.cmd == "camera") {
        if (camera_handler) camera_handler(parsed);
    } else if (parsed.cmd == "ping") {
        if (ping_handler) ping_handler();
    } else {
        if (unknown_handler) {
            unknown_handler(parsed.cmd);
        } else {
            ESP_LOGW(TAG, "Unknown command: %s", parsed.cmd.c_str());
        }
    }
}
