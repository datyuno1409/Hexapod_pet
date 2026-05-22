#ifndef HEXAPOD_PROTOCOL_H
#define HEXAPOD_PROTOCOL_H

#include <string>
#include <functional>

/**
 * HexapodProtocol - HTTP client on VoiceBot side.
 *
 * Sends JSON commands to HexapodBot via HTTP POST /cmd.
 * WHY HTTP not WebSocket: no extra Kconfig needed, works out-of-the-box
 * with esp_http_client which is always available in ESP-IDF.
 */
class HexapodProtocol {
public:
    static HexapodProtocol& GetInstance();

    // Set target host/port (call once after Wi-Fi connects)
    bool Connect(const std::string& host, int port = 8081);
    void Disconnect();
    bool IsConnected() const;

    // Send JSON command string to hexapod; returns false on network error
    bool SendCommand(const std::string& command);
    std::string GetLastTelemetry() const;

    // Optional callback for async responses (future use)
    void SetResponseCallback(std::function<void(const std::string&)> callback);

private:
    HexapodProtocol();
    ~HexapodProtocol();
    HexapodProtocol(const HexapodProtocol&) = delete;
    HexapodProtocol& operator=(const HexapodProtocol&) = delete;

    bool connected_ = false;
    std::string host_;
    int port_ = 0;
    std::function<void(const std::string&)> response_callback_ = nullptr;

    void HandleResponse(const std::string& message);
};

#endif  // HEXAPOD_PROTOCOL_H
