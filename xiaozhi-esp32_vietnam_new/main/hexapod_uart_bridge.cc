#include "hexapod_uart_bridge.h"

#include "hexapod_uart_link.h"
#include "sdkconfig.h"

// ─── Chọn đúng config header theo board đang build (dùng Kconfig) ───────────
// KHÔNG dùng __has_include vì cả 2 file đều tồn tại trong codebase,
// khiến compiler luôn chọn bot/config.h bất kể board nào đang được build.
#if defined(CONFIG_BOARD_TYPE_HEXAPOD_BOT)
    #include "boards/hexapod_bot/config.h"
    // Motion và emotion chỉ cần thiết trên Bot (Slave)
    #include "hexapod_motion.h"
    #include "hexapod_emotion_display.h"
    #define HEXAPOD_IS_BOT 1
#elif defined(CONFIG_BOARD_TYPE_HEXAPOD_VOICEBOT)
    #include "boards/hexapod_voicebot/config.h"
    #define HEXAPOD_IS_BOT 0
#elif defined(CONFIG_BOARD_TYPE_HEXAPOD_DUAL)
    #include "boards/hexapod-dual/config.h"
    #include "hexapod_motion.h"
    #include "hexapod_emotion_display.h"
    #define HEXAPOD_IS_BOT 1
#else
    // Board không phải hexapod — UART bridge sẽ không hoạt động (HEXAPOD_UART_PORT không defined)
    #define HEXAPOD_IS_BOT 0
#endif

#include <esp_log.h>
#include <cJSON.h>

#define TAG "HexapodUartBridge"

HexapodUartBridge& HexapodUartBridge::GetInstance() {
    static HexapodUartBridge instance;
    return instance;
}

HexapodUartBridge::Role HexapodUartBridge::GetRole() const {
    return role_;
}

bool HexapodUartBridge::Start(Role role) {
    if (started_) return true;

#ifndef HEXAPOD_UART_PORT
    ESP_LOGW(TAG, "HEXAPOD_UART_* is not configured for this board");
    return false;
#else
    role_ = role;
    auto& link = HexapodUartLink::GetInstance();
    link.SetMessageHandler([this](HexapodUartLink::MessageType type, uint8_t seq, const std::string& payload) {
        HandleMessage(static_cast<uint8_t>(type), seq, payload);
    });
    started_ = link.Start(HEXAPOD_UART_PORT, HEXAPOD_UART_BAUD_RATE, HEXAPOD_UART_TX_PIN, HEXAPOD_UART_RX_PIN,
                          HEXAPOD_UART_RX_BUF_SIZE, HEXAPOD_UART_TX_BUF_SIZE);
    if (started_) {
        const char* role_str = role_ == Role::kMaster ? "MASTER" :
                               role_ == Role::kSlave  ? "SLAVE" :
                               role_ == Role::kDual   ? "DUAL" : "UNKNOWN";
        ESP_LOGI(TAG, "UART bridge started as %s (GPIO TX=%d RX=%d baud=%d)",
                 role_str, HEXAPOD_UART_TX_PIN, HEXAPOD_UART_RX_PIN, HEXAPOD_UART_BAUD_RATE);
        // Announce ourselves to the other board
        std::string announce = std::string(R"({"role":")") + role_str + "\"}";
        link.SendJson(HexapodUartLink::MessageType::Ping, announce.c_str());
    }
    return started_;
#endif
}

bool HexapodUartBridge::IsStarted() const {
    return started_;
}

bool HexapodUartBridge::SendCommandJson(const std::string& payload) {
    cJSON* root = cJSON_Parse(payload.c_str());
    HexapodUartLink::MessageType type = HexapodUartLink::MessageType::Config;
    if (root) {
        const cJSON* cmd = cJSON_GetObjectItem(root, "cmd");
        const char* value = cJSON_IsString(cmd) ? cmd->valuestring : "";
        if (strcmp(value, "motion") == 0) type = HexapodUartLink::MessageType::Move;
        else if (strcmp(value, "gait") == 0) type = HexapodUartLink::MessageType::Gait;
        else if (strcmp(value, "pose") == 0) type = HexapodUartLink::MessageType::Pose;
        else if (strcmp(value, "pwm") == 0) type = HexapodUartLink::MessageType::Pwm;
        else if (strcmp(value, "emotion") == 0) type = HexapodUartLink::MessageType::Emotion;
        else if (strcmp(value, "camera") == 0) type = HexapodUartLink::MessageType::CameraSnapshot;
        else if (strcmp(value, "ping") == 0) type = HexapodUartLink::MessageType::Ping;
        cJSON_Delete(root);
    }
    return HexapodUartLink::GetInstance().Send(type, payload);
}

bool HexapodUartBridge::SendStatusRequest() {
    return HexapodUartLink::GetInstance().SendJson(HexapodUartLink::MessageType::StatusRequest, "{}");
}

std::string HexapodUartBridge::GetLastTelemetry() const {
    return last_telemetry_;
}

void HexapodUartBridge::HandleMessage(uint8_t raw_type, uint8_t seq, const std::string& payload) {
    auto type = static_cast<HexapodUartLink::MessageType>(raw_type);
    ESP_LOGI(TAG, "RX type=0x%02x seq=%u payload=%s (role=%s)",
             raw_type, seq, payload.c_str(),
             role_ == Role::kMaster ? "MASTER" :
             role_ == Role::kSlave  ? "SLAVE" :
             role_ == Role::kDual   ? "DUAL" : "UNKNOWN");

    switch (type) {
        case HexapodUartLink::MessageType::Ping:
            // Ping: always respond with Pong
            HexapodUartLink::GetInstance().SendJson(HexapodUartLink::MessageType::Pong, R"({"status":"ok"})");
            break;

        case HexapodUartLink::MessageType::Pong:
            // Pong received - link is alive
            ESP_LOGI(TAG, "Link alive (Pong received)");
            break;

        case HexapodUartLink::MessageType::Ack:
            // Ack received - command was processed
            ESP_LOGD(TAG, "Command acknowledged");
            break;

        case HexapodUartLink::MessageType::Telemetry:
        case HexapodUartLink::MessageType::CameraInfo:
            // Telemetry/CameraInfo: Master stores it, Slave doesn't need it
            last_telemetry_ = payload;
            break;

        case HexapodUartLink::MessageType::StatusRequest:
            // StatusRequest: Slave/Dual responds with telemetry
            if (role_ == Role::kSlave || role_ == Role::kDual) {
                SendTelemetry();
            }
            break;

        case HexapodUartLink::MessageType::Move:
        case HexapodUartLink::MessageType::Gait:
        case HexapodUartLink::MessageType::Pose:
        case HexapodUartLink::MessageType::Pwm:
        case HexapodUartLink::MessageType::Stop:
        case HexapodUartLink::MessageType::Emotion:
        case HexapodUartLink::MessageType::CameraSnapshot:
        case HexapodUartLink::MessageType::Config:
            // Command messages: Slave/Dual execute locally, Master just forwards (already sent)
            if (role_ == Role::kSlave || role_ == Role::kDual) {
                ExecuteRobotCommand(payload);
                HexapodUartLink::GetInstance().SendJson(HexapodUartLink::MessageType::Ack, R"({"status":"ok"})");
                SendTelemetry();
            } else {
                // Master received a command back? Shouldn't happen, just ACK
                ESP_LOGW(TAG, "Master received command type 0x%02x - unexpected", raw_type);
            }
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type 0x%02x", raw_type);
            HexapodUartLink::GetInstance().SendJson(HexapodUartLink::MessageType::Error, R"({"error":"unsupported_type"})");
            break;
    }
}

#if HEXAPOD_IS_BOT
void HexapodUartBridge::ExecuteRobotCommand(const std::string& payload) {
    cJSON* root = cJSON_Parse(payload.c_str());
    if (!root) return;

    const cJSON* cmd = cJSON_GetObjectItem(root, "cmd");
    const cJSON* action = cJSON_GetObjectItem(root, "action");
    const cJSON* speed = cJSON_GetObjectItem(root, "speed");
    const cJSON* duration = cJSON_GetObjectItem(root, "duration_ms");
    const char* cmd_s = cJSON_IsString(cmd) ? cmd->valuestring : "";
    const char* action_s = cJSON_IsString(action) ? action->valuestring : "";
    uint8_t speed_v = cJSON_IsNumber(speed) ? static_cast<uint8_t>(speed->valueint) : 50;
    uint32_t duration_v = cJSON_IsNumber(duration) ? static_cast<uint32_t>(duration->valueint) : 0;

    if (strcmp(cmd_s, "motion") == 0) {
        if (strcmp(action_s, "forward") == 0) HexapodMotion::GetInstance().MoveForward(speed_v, duration_v);
        else if (strcmp(action_s, "backward") == 0) HexapodMotion::GetInstance().MoveBackward(speed_v, duration_v);
        else if (strcmp(action_s, "left") == 0) HexapodMotion::GetInstance().TurnLeft(speed_v, duration_v);
        else if (strcmp(action_s, "right") == 0) HexapodMotion::GetInstance().TurnRight(speed_v, duration_v);
        else if (strcmp(action_s, "jump") == 0) HexapodMotion::GetInstance().Jump(speed_v);
        else if (strcmp(action_s, "dance") == 0) HexapodMotion::GetInstance().Dance(speed_v, duration_v);
        else if (strcmp(action_s, "sit") == 0) HexapodMotion::GetInstance().Sit();
        else if (strcmp(action_s, "stand") == 0) HexapodMotion::GetInstance().Stand();
        else if (strcmp(action_s, "stop") == 0) HexapodMotion::GetInstance().Stop();
    } else if (strcmp(cmd_s, "emotion") == 0) {
        const cJSON* emotion = cJSON_GetObjectItem(root, "emotion");
        const cJSON* text = cJSON_GetObjectItem(root, "text");
        HexapodEmotionDisplay::GetInstance().ShowEmotion(cJSON_IsString(emotion) ? emotion->valuestring : "neutral",
                                                          cJSON_IsString(text) ? text->valuestring : "");
    } else if (strcmp(cmd_s, "ping") == 0) {
        SendTelemetry();
    }
    cJSON_Delete(root);
}
#else
// VoiceBot (Master) không thực thi lệnh robot cục bộ — chỉ gửi qua UART
void HexapodUartBridge::ExecuteRobotCommand(const std::string& /*payload*/) {
    ESP_LOGW(TAG, "ExecuteRobotCommand called on non-Bot board — ignored");
}
#endif

void HexapodUartBridge::SendTelemetry() {
#if HEXAPOD_IS_BOT
    HexapodUartLink::GetInstance().SendJson(
        HexapodUartLink::MessageType::Telemetry,
        R"({"link":"uart","motion":"ready","battery":0,"gait":"default","camera":"available"})");
#else
    // VoiceBot không gửi telemetry (đó là nhiệm vụ của Bot/Slave)
    ESP_LOGD(TAG, "SendTelemetry: skipped on VoiceBot (Master)");
#endif
}
