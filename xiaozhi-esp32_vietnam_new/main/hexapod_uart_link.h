#ifndef HEXAPOD_UART_LINK_H
#define HEXAPOD_UART_LINK_H

#include <cstdint>
#include <functional>
#include <string>

#include <driver/gpio.h>
#include <driver/uart.h>

class HexapodUartLink {
public:
    enum class MessageType : uint8_t {
        Ping = 0x01,
        Pong = 0x02,
        Ack = 0x03,
        Error = 0x04,
        Move = 0x10,
        Gait = 0x11,
        Pose = 0x12,
        Pwm = 0x13,
        Stop = 0x14,
        Emotion = 0x20,
        Telemetry = 0x30,
        StatusRequest = 0x31,
        CameraInfo = 0x40,
        CameraSnapshot = 0x41,
        CameraChunk = 0x42,
        CameraEnd = 0x43,
        Config = 0x50,
    };

    using MessageHandler = std::function<void(MessageType type, uint8_t seq, const std::string& payload)>;

    static HexapodUartLink& GetInstance();

    bool Start(uart_port_t port, int baud_rate, gpio_num_t tx_pin, gpio_num_t rx_pin,
               int rx_buffer_size = 4096, int tx_buffer_size = 4096);
    bool IsStarted() const;
    bool Send(MessageType type, const std::string& payload);
    bool SendJson(MessageType type, const char* json);
    void SetMessageHandler(MessageHandler handler);

private:
    HexapodUartLink() = default;
    HexapodUartLink(const HexapodUartLink&) = delete;
    HexapodUartLink& operator=(const HexapodUartLink&) = delete;

    static void RxTaskEntry(void* arg);
    void RxTask();
    void ParseByte(uint8_t byte);
    void ResetParser();

    uart_port_t port_ = UART_NUM_1;
    bool started_ = false;
    uint8_t tx_seq_ = 0;
    MessageHandler handler_ = nullptr;

    enum class ParseState : uint8_t { Sof1, Sof2, Version, Type, Seq, LenL, LenH, Payload, CrcL, CrcH };
    ParseState state_ = ParseState::Sof1;
    uint8_t rx_type_ = 0;
    uint8_t rx_seq_ = 0;
    uint16_t rx_len_ = 0;
    uint16_t rx_crc_ = 0;
    std::string rx_payload_;
};

#endif  // HEXAPOD_UART_LINK_H
