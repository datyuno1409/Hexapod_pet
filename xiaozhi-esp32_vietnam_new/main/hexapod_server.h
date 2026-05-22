#ifndef HEXAPOD_SERVER_H
#define HEXAPOD_SERVER_H

#include <string>
#include <functional>
#include <cJSON.h>
#include <memory>

/**
 * @brief Hexapod WebSocket Server
 *
 * Runs on Hexapod ESP32 to receive commands from VoiceBot.
 * Supports motion control, camera capture, and emotion display.
 */
class HexapodServer {
public:
    static HexapodServer& GetInstance();

    /**
     * @brief Start WebSocket server
     * @param port WebSocket port (default 8081)
     * @return true if successful
     */
    bool Start(int port = 8081);

    /**
     * @brief Stop WebSocket server
     */
    void Stop();

    /**
     * @brief Check if server is running
     */
    bool IsRunning() const;

    /**
     * @brief Send response back to VoiceBot
     * @param response JSON response
     */
    void SendResponse(const std::string& response);

    // Called by static HTTP handler — must be public
    void HandleMessage(const std::string& payload);

private:
    HexapodServer();
    ~HexapodServer();

    HexapodServer(const HexapodServer&) = delete;
    HexapodServer& operator=(const HexapodServer&) = delete;

    bool running_ = false;
    int port_ = 0;
    /**
     * @brief Handle motion command
     * @param motion_cmd JSON command with action, speed, duration
     */
    void HandleMotionCommand(const cJSON* motion_cmd);

    /**
     * @brief Handle camera command (capture or stream)
     * @param camera_cmd JSON command with action (capture/stream_on/stream_off)
     */
    void HandleCameraCommand(const cJSON* camera_cmd);

    /**
     * @brief Handle emotion display command
     * @param emotion_cmd JSON command with emotion and optional text
     */
    void HandleEmotionCommand(const cJSON* emotion_cmd);

    /**
     * @brief Parse and dispatch JSON command
     * @param root cJSON root object
     */
    void DispatchCommand(const cJSON* root);
};

#endif  // HEXAPOD_SERVER_H
