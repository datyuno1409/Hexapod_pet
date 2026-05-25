#include "hexapod_server.h"
#include "hexapod_motion.h"
#include "hexapod_emotion_display.h"
#include "hexapod_command_dispatcher.h"
#include "boards/common/board.h"
#include "esp32_camera.h"
#include "hexapod_constants.h"
#include <esp_log.h>
#include <esp_http_server.h>
#include <cJSON.h>
#include <string>
#include <vector>
#include <algorithm>
#include <mutex>

#define TAG "HexapodServer"

static std::vector<int> ws_clients;
static std::mutex ws_mutex;

static httpd_handle_t server_handle = nullptr;
static HexapodServer* g_server_instance = nullptr;
static const char* INDEX_HTML = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hexapod Controller</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #1a1a2e; color: #fff; display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; margin: 0; }
        h1 { color: #00d2ff; text-shadow: 0 0 10px rgba(0,210,255,0.5); margin-bottom: 30px; }
        .controller { display: grid; grid-template-columns: repeat(3, 80px); grid-gap: 15px; background: rgba(255,255,255,0.05); padding: 30px; border-radius: 20px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
        button { width: 80px; height: 80px; border: none; border-radius: 15px; background: #16213e; color: #fff; font-size: 24px; cursor: pointer; transition: all 0.2s; box-shadow: 0 4px 0 #0f3460; }
        button:active { transform: translateY(4px); box-shadow: none; background: #00d2ff; }
        .btn-action { background: #e94560; box-shadow: 0 4px 0 #950740; }
        .btn-action:active { background: #ff4d6d; }
        .status { margin-top: 20px; font-size: 14px; color: #888; }
        .special-btns { margin-top: 20px; display: flex; gap: 10px; }
        .special-btns button { width: 120px; height: 50px; font-size: 16px; }
        .camera-feed { width: 320px; height: 240px; background: #000; margin-bottom: 20px; border-radius: 10px; border: 2px solid #00d2ff; overflow: hidden; display: flex; align-items: center; justify-content: center; }
        .camera-feed img { width: 100%; height: 100%; object-fit: cover; }
    </style>
</head>
<body>
    <h1>HEXAPOD BOT</h1>
    <div class="camera-feed">
        <img id="camera" src="" alt="Camera Feed (Connecting...)">
    </div>
    <div class="controller">
        <div></div>
        <button onclick="sendCmd('forward')">⬆️</button>
        <div></div>
        <button onclick="sendCmd('left')">⬅️</button>
        <button class="btn-action" onclick="sendCmd('stand')">🏠</button>
        <button onclick="sendCmd('right')">➡️</button>
        <div></div>
        <button onclick="sendCmd('backward')">⬇️</button>
        <div></div>
    </div>
    <div class="special-btns">
        <button class="btn-action" onclick="sendCmd('dance')">💃 DANCE</button>
        <button class="btn-action" onclick="sendCmd('jump')">🚀 JUMP</button>
    </div>
    <div class="status" id="status">Ready</div>

    <script>
        // Camera WebSocket logic
        const camImg = document.getElementById('camera');
        const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = wsProtocol + '//' + window.location.host + '/camera';
        let camWs;

        function connectCamera() {
            camWs = new WebSocket(wsUrl);
            camWs.binaryType = 'blob';
            camWs.onmessage = (event) => {
                if (event.data instanceof Blob) {
                    const url = URL.createObjectURL(event.data);
                    const oldUrl = camImg.src;
                    camImg.src = url;
                    if (oldUrl.startsWith('blob:')) {
                        URL.revokeObjectURL(oldUrl);
                    }
                }
            };
            camWs.onclose = () => {
                setTimeout(connectCamera, 2000);
            };
        }
        connectCamera();

        function sendCmd(action) {
            const status = document.getElementById('status');
            status.innerText = 'Sending: ' + action + '...';
            const body = {
                cmd: "motion",
                action: action,
                speed: 60,
                duration_ms: 1000
            };
            fetch('/cmd', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(body)
            })
            .then(res => res.json())
            .then(data => {
                status.innerText = 'Success: ' + action;
                setTimeout(() => status.innerText = 'Ready', 2000);
            })
            .catch(err => {
                status.innerText = 'Error: ' + err;
                console.error(err);
            });
        }
    </script>
</body>
</html>
)html";

// HTTP POST handler: receives JSON command in request body
static esp_err_t command_post_handler(httpd_req_t* req) {
    char buf[512] = {0};
    int total_len = req->content_len;

    if (total_len <= 0 || total_len >= (int)sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content length");
        return ESP_FAIL;
    }

    int received = httpd_req_recv(req, buf, total_len);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive body");
        return ESP_FAIL;
    }
    buf[received] = '\0';

    ESP_LOGI(TAG, "Received command: %s", buf);

    if (g_server_instance) {
        g_server_instance->HandleMessage(std::string(buf));
    }

    // Send back simple ACK
    const char* resp = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

#ifdef CONFIG_HTTPD_WS_SUPPORT
// WebSocket handler for camera stream
static esp_err_t camera_ws_handler(httpd_req_t* req) {
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, "New WebSocket connection on fd %d", fd);
        std::lock_guard<std::mutex> lock(ws_mutex);
        ws_clients.push_back(fd);
        return ESP_OK;
    }

    // Handle closing or messages
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret == ESP_ERR_NOT_FOUND || ret == ESP_ERR_INVALID_STATE) {
        int fd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, "WebSocket closed on fd %d", fd);
        std::lock_guard<std::mutex> lock(ws_mutex);
        ws_clients.erase(std::remove(ws_clients.begin(), ws_clients.end(), fd), ws_clients.end());
    }
    return ret;
}
#endif

// Background task to push camera frames
static void camera_stream_task(void* arg) {
    ESP_LOGI(TAG, "Camera stream task started");
    while (true) {
#ifdef CONFIG_HTTPD_WS_SUPPORT
        {
            std::lock_guard<std::mutex> lock(ws_mutex);
            if (!ws_clients.empty()) {
                Camera* camera = Board::GetInstance().GetCamera();
                if (camera) {
                    camera->Capture();
                }
            }
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(1000 / HexapodConst::CAMERA_STREAM_FPS)); // Configurable FPS
    }
}

// HTTP GET / handler: serves simple web control page
static esp_err_t root_get_handler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req, INDEX_HTML);
    return ESP_OK;
}

// HTTP GET /ping handler for health check
static esp_err_t ping_handler(httpd_req_t* req) {
    httpd_resp_sendstr(req, "{\"status\":\"alive\"}");
    return ESP_OK;
}

// ============================================================================

HexapodServer& HexapodServer::GetInstance() {
    static HexapodServer instance;
    return instance;
}

HexapodServer::HexapodServer() {
    g_server_instance = this;
}

HexapodServer::~HexapodServer() {
    Stop();
}

bool HexapodServer::Start(int port) {
    if (running_) {
        ESP_LOGW(TAG, "Server already running");
        return true;
    }

    port_ = port;
    ESP_LOGI(TAG, "Starting HTTP command server on port %d", port);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.stack_size = 8192;
    config.max_open_sockets = 4;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server_handle, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return false;
    }

    httpd_uri_t cmd_uri = {
        .uri      = "/cmd",
        .method   = HTTP_POST,
        .handler  = command_post_handler,
        .user_ctx = nullptr,
    };

    httpd_uri_t ping_uri = {
        .uri      = "/ping",
        .method   = HTTP_GET,
        .handler  = ping_handler,
        .user_ctx = nullptr,
    };

    httpd_uri_t root_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = root_get_handler,
        .user_ctx = nullptr,
    };


    httpd_register_uri_handler(server_handle, &cmd_uri);
    httpd_register_uri_handler(server_handle, &ping_uri);
    httpd_register_uri_handler(server_handle, &root_uri);
#ifdef CONFIG_HTTPD_WS_SUPPORT
    httpd_uri_t camera_ws = {
        .uri        = "/camera",
        .method     = HTTP_GET,
        .handler    = camera_ws_handler,
        .user_ctx   = nullptr,
        .is_websocket = true
    };
    httpd_register_uri_handler(server_handle, &camera_ws);

    xTaskCreate(camera_stream_task, "camera_stream", 4096, nullptr, 5, nullptr);
#endif

    running_ = true;
    ESP_LOGI(TAG, "HTTP command server started on port %d", port);
    return true;
}

void HexapodServer::Stop() {
    if (!running_) return;
    if (server_handle) {
        httpd_stop(server_handle);
        server_handle = nullptr;
    }
    running_ = false;
    ESP_LOGI(TAG, "HTTP command server stopped");
}

bool HexapodServer::IsRunning() const {
    return running_;
}

void HexapodServer::SendResponse(const std::string& response) {
    ESP_LOGI(TAG, "Response: %s", response.c_str());
}

void HexapodServer::HandleMessage(const std::string& payload) {
    cJSON* root = cJSON_Parse(payload.c_str());
    if (!root) {
        ESP_LOGE(TAG, "JSON parse failed: %s", payload.c_str());
        return;
    }
    DispatchCommand(root);
    cJSON_Delete(root);
}

void HexapodServer::DispatchCommand(const cJSON* root) {
    CommandDispatcher::Dispatch(root,
        [this](const ParsedCommand& p) { HandleMotionCommand(p); },
        [this](const ParsedCommand& p) { HandleEmotionCommand(p); },
        [this](const ParsedCommand& p) { HandleCameraCommand(p); },
        [this]() { ESP_LOGI(TAG, "Ping received"); },
        [this](const std::string& unknown) { ESP_LOGW(TAG, "Unknown command: %s", unknown.c_str()); }
    );
}

void HexapodServer::HandleMotionCommand(const ParsedCommand& cmd) {
    std::string action = cmd.motion_action;
    int speed       = cmd.speed;
    int duration_ms = cmd.duration_ms;

    if (action.empty()) {
        ESP_LOGE(TAG, "Motion: missing 'action'");
        return;
    }

    ESP_LOGI(TAG, "Motion: action=%s speed=%d duration=%dms", action.c_str(), speed, duration_ms);

    HexapodMotion& motion = HexapodMotion::GetInstance();
    if      (action == "forward")  motion.MoveForward(speed, duration_ms);
    else if (action == "backward") motion.MoveBackward(speed, duration_ms);
    else if (action == "left")     motion.TurnLeft(speed, duration_ms);
    else if (action == "right")    motion.TurnRight(speed, duration_ms);
    else if (action == "jump")     motion.Jump(speed);
    else if (action == "sit")      motion.Sit();
    else if (action == "dance")    motion.Dance(speed, duration_ms);
    else if (action == "stand")    motion.Stand();
    else ESP_LOGW(TAG, "Unknown action: %s", action.c_str());
}

void HexapodServer::HandleCameraCommand(const ParsedCommand& cmd) {
    std::string action = cmd.camera_action;
    if (action.empty()) {
        ESP_LOGE(TAG, "Camera: missing 'action'");
        return;
    }
    ESP_LOGI(TAG, "Camera: action=%s", action.c_str());
    // TODO: implement camera capture/stream
}

void HexapodServer::HandleEmotionCommand(const ParsedCommand& cmd) {
    std::string emotion = cmd.emotion;
    std::string text = cmd.emotion_text;

    ESP_LOGI(TAG, "Emotion: %s text=%s", emotion.c_str(), text.c_str());
    HexapodEmotionDisplay::GetInstance().ShowEmotion(emotion, text);
}
