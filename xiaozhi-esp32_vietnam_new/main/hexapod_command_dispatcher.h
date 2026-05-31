#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include "cJSON.h"
#include "hexapod_constants.h"

/**
 * @brief Parsed command data structure to avoid repeated JSON lookups
 */
struct ParsedCommand {
    std::string cmd;

    // Motion parameters
    std::string motion_action;
    int speed = HexapodConst::DEFAULT_MOTION_SPEED;
    int duration_ms = 0;

    // Emotion parameters
    std::string emotion;
    std::string emotion_text;

    // Camera parameters
    std::string camera_action;
    int camera_value = 0;  // Generic value field for camera commands (e.g., quality level)
};

/**
 * @brief CommandDispatcher centralizes the parsing and dispatching of robot commands
 * from different sources (WebSocket, UART, etc.)
 */
class CommandDispatcher {
public:
    using MotionHandler = std::function<void(const ParsedCommand&)>;
    using EmotionHandler = std::function<void(const ParsedCommand&)>;
    using CameraHandler = std::function<void(const ParsedCommand&)>;
    using PingHandler = std::function<void()>;
    using UnknownHandler = std::function<void(const std::string&)>;

    /**
     * @brief Parse a cJSON object into a ParsedCommand struct
     * @return true if 'cmd' field is present and valid
     */
    static bool Parse(const cJSON* root, ParsedCommand& out);

    /**
     * @brief Unified entry point for dispatching commands
     */
    static void Dispatch(const cJSON* root, MotionHandler motion_handler, EmotionHandler emotion_handler, CameraHandler camera_handler, PingHandler ping_handler, UnknownHandler unknown_handler = nullptr);
};
