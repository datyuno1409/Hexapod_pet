#include "hexapod_protocol.h"
#include "hexapod_uart_bridge.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include <string>

// Uses plain HTTP POST to /cmd on the hexapod server.
// WHY not WebSocket: esp_websocket_client requires CONFIG_HTTPD_WS_SUPPORT=y
// and additional menuconfig setup. HTTP POST has enough throughput for
// hexapod command rate (~1 cmd/sec from voice) and needs no extra config.

#define TAG "HexapodProtocol"

HexapodProtocol& HexapodProtocol::GetInstance() {
    static HexapodProtocol instance;
    return instance;
}

HexapodProtocol::HexapodProtocol() {}
HexapodProtocol::~HexapodProtocol() {}

bool HexapodProtocol::Connect(const std::string& host, int port) {
    host_ = host;
    port_ = port;
    connected_ = true;
    ESP_LOGI(TAG, "Hexapod target set: %s:%d (HTTP POST)", host.c_str(), port);
    return true;
}

void HexapodProtocol::Disconnect() {
    connected_ = false;
    ESP_LOGI(TAG, "Hexapod disconnected");
}

bool HexapodProtocol::IsConnected() const { return connected_; }

bool HexapodProtocol::SendCommand(const std::string& command) {
    if (HexapodUartBridge::GetInstance().IsStarted()) {
        return HexapodUartBridge::GetInstance().SendCommandJson(command);
    }

    if (!connected_) {
        ESP_LOGE(TAG, "Not connected to hexapod");
        return false;
    }

    std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/cmd";

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 2000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, command.c_str(), (int)command.size());

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGD(TAG, "Command sent: %s", command.c_str());
    return true;
}

void HexapodProtocol::SetResponseCallback(std::function<void(const std::string&)> callback) {
    response_callback_ = callback;
}

void HexapodProtocol::HandleResponse(const std::string& message) {
    if (response_callback_) {
        response_callback_(message);
    }
}

std::string HexapodProtocol::GetLastTelemetry() const {
    return HexapodUartBridge::GetInstance().GetLastTelemetry();
}
