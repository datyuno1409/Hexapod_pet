#include "hexapod_mcp_tools.h"
#include "hexapod_protocol.h"
#include "hexapod_constants.h"
#include <esp_log.h>
#include <cJSON.h>

#define TAG "HexapodMcpTools"

// Helper: read string from PropertyList, return default if not found
static std::string GetString(const PropertyList& args, const std::string& key, const std::string& def = "") {
    try {
        return args[key].value<std::string>();
    } catch (...) {
        return def;
    }
}

// Helper: read int from PropertyList, return default if not found
static int GetInt(const PropertyList& args, const std::string& key, int def = 0) {
    try {
        return args[key].value<int>();
    } catch (...) {
        return def;
    }
}

// Helper: read bool from PropertyList
static bool GetBool(const PropertyList& args, const std::string& key, bool def = false) {
    try {
        return args[key].value<bool>();
    } catch (...) {
        return def;
    }
}

// Helper: build JSON string and send to hexapod
static std::string SendJson(cJSON* root) {
    char* raw = cJSON_PrintUnformatted(root);
    std::string payload(raw);
    cJSON_free(raw);
    cJSON_Delete(root);
    HexapodProtocol::GetInstance().SendCommand(payload);
    return payload;
}

// ============================================================================

ReturnValue HexapodMcpTools::HandleMoveCommand(const PropertyList& args) {
    std::string action      = GetString(args, "action", "forward");
    int         speed       = GetInt(args, "speed", static_cast<int>(HexapodConst::DEFAULT_MOTION_SPEED));
    int         duration_ms = GetInt(args, "duration_ms", 1000);

    ESP_LOGI(TAG, "hexapod.move: action=%s speed=%d duration=%dms",
             action.c_str(), speed, duration_ms);

    cJSON* cmd = cJSON_CreateObject();
    cJSON_AddStringToObject(cmd, "cmd", "motion");
    cJSON_AddStringToObject(cmd, "action", action.c_str());
    cJSON_AddNumberToObject(cmd, "speed", speed);
    cJSON_AddNumberToObject(cmd, "duration_ms", duration_ms);
    SendJson(cmd);

    return std::string("{\"status\":\"ok\",\"action\":\"" + action + "\"}");
}

ReturnValue HexapodMcpTools::HandleCameraCapture(const PropertyList& args) {
    std::string resolution = GetString(args, "resolution", HexapodConst::CAMERA_DEFAULT_RESOLUTION);

    ESP_LOGI(TAG, "hexapod.camera.capture: resolution=%s", resolution.c_str());

    cJSON* cmd = cJSON_CreateObject();
    cJSON_AddStringToObject(cmd, "cmd", "camera");
    cJSON_AddStringToObject(cmd, "action", "capture");
    cJSON_AddStringToObject(cmd, "resolution", resolution.c_str());
    SendJson(cmd);

    return std::string("{\"status\":\"ok\",\"action\":\"capture\"}");
}

ReturnValue HexapodMcpTools::HandleCameraStream(const PropertyList& args) {
    bool enable = GetBool(args, "enable", true);

    ESP_LOGI(TAG, "hexapod.camera.stream: enable=%d", enable);

    cJSON* cmd = cJSON_CreateObject();
    cJSON_AddStringToObject(cmd, "cmd", "camera");
    cJSON_AddStringToObject(cmd, "action", enable ? "stream_on" : "stream_off");
    SendJson(cmd);

    return std::string(std::string("{\"status\":\"ok\",\"streaming\":") + (enable ? "true" : "false") + "}");
}

ReturnValue HexapodMcpTools::HandleEmotion(const PropertyList& args) {
    std::string emotion = GetString(args, "emotion", "neutral");
    std::string text    = GetString(args, "text", "");

    ESP_LOGI(TAG, "hexapod.emotion: emotion=%s text=%s", emotion.c_str(), text.c_str());

    cJSON* cmd = cJSON_CreateObject();
    cJSON_AddStringToObject(cmd, "cmd", "emotion");
    cJSON_AddStringToObject(cmd, "emotion", emotion.c_str());
    if (!text.empty()) {
        cJSON_AddStringToObject(cmd, "text", text.c_str());
    }
    SendJson(cmd);

    return std::string("{\"status\":\"ok\",\"emotion\":\"" + emotion + "\"}");
}

ReturnValue HexapodMcpTools::HandleStatus(const PropertyList& /*args*/) {
    ESP_LOGI(TAG, "hexapod.status called");

    cJSON* cmd = cJSON_CreateObject();
    cJSON_AddStringToObject(cmd, "cmd", "ping");
    SendJson(cmd);

    return std::string(R"({"status":"ok","motion_state":"idle","camera_ready":true})");
}

void HexapodMcpTools::RegisterTools(McpServer& mcp_server) {
    ESP_LOGI(TAG, "Registering hexapod MCP tools");

    PropertyList move_props;
    move_props.AddProperty(Property("action", kPropertyTypeString));
    move_props.AddProperty(Property("speed", kPropertyTypeInteger, HexapodConst::DEFAULT_MOTION_SPEED, 1, 100));
    move_props.AddProperty(Property("duration_ms", kPropertyTypeInteger, 1000, 0, 10000));
    mcp_server.AddTool("hexapod.move",
                       "Move the hexapod robot. action: forward/backward/left/right/jump/sit/dance/stand",
                       move_props, HandleMoveCommand);

    PropertyList cap_props;
    cap_props.AddProperty(Property("resolution", kPropertyTypeString, HexapodConst::CAMERA_DEFAULT_RESOLUTION));
    mcp_server.AddTool("hexapod.camera.capture", "Capture image from hexapod camera",
                       cap_props, HandleCameraCapture);

    PropertyList stream_props;
    stream_props.AddProperty(Property("enable", kPropertyTypeBoolean, true));
    mcp_server.AddTool("hexapod.camera.stream", "Enable or disable camera streaming",
                       stream_props, HandleCameraStream);

    PropertyList emotion_props;
    emotion_props.AddProperty(Property("emotion", kPropertyTypeString));
    emotion_props.AddProperty(Property("text", kPropertyTypeString, std::string("")));
    mcp_server.AddUserOnlyTool("hexapod.emotion",
                               "Set emotion display. emotion: happy/sad/curious/excited/neutral/angry",
                               emotion_props, HandleEmotion);

    PropertyList status_props;
    mcp_server.AddTool("hexapod.status", "Get hexapod robot status",
                       status_props, HandleStatus);

    ESP_LOGI(TAG, "Hexapod MCP tools registered (%d tools)", 5);
}
