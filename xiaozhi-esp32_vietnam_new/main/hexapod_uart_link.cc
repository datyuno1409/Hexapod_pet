#include "hexapod_uart_link.h"

#include <cstring>
#include <vector>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "HexapodUart"

namespace {
constexpr uint8_t kSof1 = 0xAA;
constexpr uint8_t kSof2 = 0x55;
constexpr uint8_t kVersion = 0x01;
constexpr size_t kMaxPayload = 2048;

uint16_t Crc16Ccitt(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021) : static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}
}

HexapodUartLink& HexapodUartLink::GetInstance() {
    static HexapodUartLink instance;
    return instance;
}

bool HexapodUartLink::Start(uart_port_t port, int baud_rate, gpio_num_t tx_pin, gpio_num_t rx_pin,
                            int rx_buffer_size, int tx_buffer_size) {
    if (started_) {
        return true;
    }

    port_ = port;
    uart_config_t config = {};
    config.baud_rate = baud_rate;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_param_config(port_, &config));
    ESP_ERROR_CHECK(uart_set_pin(port_, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(port_, rx_buffer_size, tx_buffer_size, 0, nullptr, 0));

    started_ = true;
    xTaskCreate(RxTaskEntry, "hexapod_uart_rx", 4096, this, 8, nullptr);
    ESP_LOGI(TAG, "UART link started port=%d baud=%d tx=%d rx=%d", port_, baud_rate, tx_pin, rx_pin);
    return true;
}

bool HexapodUartLink::IsStarted() const {
    return started_;
}

bool HexapodUartLink::Send(MessageType type, const std::string& payload) {
    if (!started_) {
        ESP_LOGW(TAG, "UART link not started");
        return false;
    }
    if (payload.size() > kMaxPayload) {
        ESP_LOGE(TAG, "Payload too large: %u", static_cast<unsigned>(payload.size()));
        return false;
    }

    const uint16_t len = static_cast<uint16_t>(payload.size());
    std::vector<uint8_t> frame;
    frame.reserve(9 + len);
    frame.push_back(kSof1);
    frame.push_back(kSof2);
    frame.push_back(kVersion);
    frame.push_back(static_cast<uint8_t>(type));
    frame.push_back(++tx_seq_);
    frame.push_back(static_cast<uint8_t>(len & 0xFF));
    frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    frame.insert(frame.end(), payload.begin(), payload.end());

    const uint16_t crc = Crc16Ccitt(frame.data() + 2, frame.size() - 2);
    frame.push_back(static_cast<uint8_t>(crc & 0xFF));
    frame.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));

    const int written = uart_write_bytes(port_, frame.data(), frame.size());
    return written == static_cast<int>(frame.size());
}

bool HexapodUartLink::SendJson(MessageType type, const char* json) {
    return Send(type, json ? std::string(json) : std::string("{}"));
}

void HexapodUartLink::SetMessageHandler(MessageHandler handler) {
    handler_ = handler;
}

void HexapodUartLink::RxTaskEntry(void* arg) {
    static_cast<HexapodUartLink*>(arg)->RxTask();
}

void HexapodUartLink::RxTask() {
    uint8_t buffer[128];
    while (true) {
        int len = uart_read_bytes(port_, buffer, sizeof(buffer), pdMS_TO_TICKS(100));
        for (int i = 0; i < len; ++i) {
            ParseByte(buffer[i]);
        }
    }
}

void HexapodUartLink::ResetParser() {
    state_ = ParseState::Sof1;
    rx_type_ = 0;
    rx_seq_ = 0;
    rx_len_ = 0;
    rx_crc_ = 0;
    rx_payload_.clear();
}

void HexapodUartLink::ParseByte(uint8_t byte) {
    switch (state_) {
        case ParseState::Sof1:
            if (byte == kSof1) state_ = ParseState::Sof2;
            break;
        case ParseState::Sof2:
            state_ = (byte == kSof2) ? ParseState::Version : ParseState::Sof1;
            break;
        case ParseState::Version:
            if (byte != kVersion) { ResetParser(); break; }
            state_ = ParseState::Type;
            break;
        case ParseState::Type:
            rx_type_ = byte;
            state_ = ParseState::Seq;
            break;
        case ParseState::Seq:
            rx_seq_ = byte;
            state_ = ParseState::LenL;
            break;
        case ParseState::LenL:
            rx_len_ = byte;
            state_ = ParseState::LenH;
            break;
        case ParseState::LenH:
            rx_len_ |= static_cast<uint16_t>(byte) << 8;
            if (rx_len_ > kMaxPayload) { ResetParser(); break; }
            if (rx_len_ > 0) {
                rx_payload_.clear();
                rx_payload_.reserve(rx_len_);
            }
            state_ = rx_len_ ? ParseState::Payload : ParseState::CrcL;
            break;
        case ParseState::Payload:
            rx_payload_.push_back(static_cast<char>(byte));
            if (rx_payload_.size() >= rx_len_) state_ = ParseState::CrcL;
            break;
        case ParseState::CrcL:
            rx_crc_ = byte;
            state_ = ParseState::CrcH;
            break;
        case ParseState::CrcH: {
            rx_crc_ |= static_cast<uint16_t>(byte) << 8;
            std::vector<uint8_t> check;
            check.reserve(5 + rx_payload_.size());
            check.push_back(kVersion);
            check.push_back(rx_type_);
            check.push_back(rx_seq_);
            check.push_back(static_cast<uint8_t>(rx_len_ & 0xFF));
            check.push_back(static_cast<uint8_t>((rx_len_ >> 8) & 0xFF));
            check.insert(check.end(), rx_payload_.begin(), rx_payload_.end());
            const uint16_t expected = Crc16Ccitt(check.data(), check.size());
            if (expected == rx_crc_) {
                if (handler_) handler_(static_cast<MessageType>(rx_type_), rx_seq_, rx_payload_);
            } else {
                static uint32_t crc_error_count = 0;
                crc_error_count++;
                if (crc_error_count % 100 == 0) {
                    ESP_LOGW(TAG, "CRC errors: %u", crc_error_count);
                } else {
                    ESP_LOGW(TAG, "CRC mismatch seq=%u", rx_seq_);
                }
            }
            ResetParser();
            break;
        }
    }
}
