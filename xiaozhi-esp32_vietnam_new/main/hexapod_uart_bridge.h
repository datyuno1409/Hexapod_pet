#ifndef HEXAPOD_UART_BRIDGE_H
#define HEXAPOD_UART_BRIDGE_H

#include <string>

class HexapodUartBridge {
public:
    enum class Role {
        kUnknown,    // Not started yet
        kMaster,     // VoiceBot: sends commands to Bot
        kSlave,      // Bot: receives commands, executes motion/emotion
        kDual,       // Single board: both send and receive
    };

    static HexapodUartBridge& GetInstance();

    // Start UART bridge with a specific role
    // - kMaster (VoiceBot): sends commands, receives telemetry
    // - kSlave (Bot): receives commands, executes them, sends telemetry
    // - kDual: both send and receive
    bool Start(Role role = Role::kUnknown);
    bool IsStarted() const;
    Role GetRole() const;

    // Send a command (JSON payload) to the other board via UART
    // On Master: sends to Bot for execution
    // On Slave: sends telemetry/status back to VoiceBot
    bool SendCommandJson(const std::string& payload);
    bool SendStatusRequest();
    std::string GetLastTelemetry() const;

private:
    HexapodUartBridge() = default;
    HexapodUartBridge(const HexapodUartBridge&) = delete;
    HexapodUartBridge& operator=(const HexapodUartBridge&) = delete;

    void HandleMessage(uint8_t type, uint8_t seq, const std::string& payload);

    // Only Slave/Dual execute robot commands locally
    void ExecuteRobotCommand(const std::string& payload);

    // Only Slave/Dual send telemetry back
    void SendTelemetry();

    bool started_ = false;
    Role role_ = Role::kUnknown;
    std::string last_telemetry_ = R"({"link":"unknown"})";
};

#endif  // HEXAPOD_UART_BRIDGE_H