#include "hexapod_server.h"
#include "hexapod_motion.h"
#include "hexapod_emotion_display.h"
#include "hexapod_command_dispatcher.h"
#include "boards/common/board.h"
#include "boards/common/esp32_camera.h"
#include "display/lvgl_display/jpg/image_to_jpeg.h"
#include "hexapod_constants.h"
#include <esp_log.h>
#include <esp_http_server.h>
#include <cJSON.h>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>

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
<meta name="viewport" content="width=device-width,initial-scale=1.0,user-scalable=no">
<title>Hexapod</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#000;color:#fff;display:flex;flex-direction:column;align-items:center;min-height:100vh;overflow-x:hidden}
.header{width:100%;display:flex;justify-content:space-between;align-items:center;padding:8px 12px}
h1{color:#00d2ff;font-size:20px;text-shadow:0 0 10px rgba(0,210,255,.5)}
.stats{font-size:12px;color:#aaa;display:flex;gap:12px}
.camera-feed{width:100%;max-width:640px;aspect-ratio:4/3;background:#111;position:relative;overflow:hidden;display:flex;align-items:center;justify-content:center}
.camera-feed img{width:100%;height:100%;object-fit:cover;display:block}
.camera-overlay{position:absolute;top:6px;left:6px;color:#fff;font-size:12px;background:rgba(0,0,0,.6);padding:2px 8px;border-radius:4px;pointer-events:none}
.live-indicator{position:absolute;top:6px;right:6px;color:#0f0;font-size:12px;font-weight:700;opacity:.8}
.controls-container{display:flex;width:100%;max-width:640px;justify-content:center;align-items:center;padding:12px;gap:16px;flex-wrap:wrap}
.joystick-area{width:160px;height:160px;min-width:160px;min-height:160px;aspect-ratio:1;background:rgba(255,255,255,.08);border-radius:50%;position:relative;border:2px solid #333;flex-shrink:0;touch-action:none}
.joystick-knob{width:50px;height:50px;background:radial-gradient(circle,#00d2ff,#0078ff);border-radius:50%;position:absolute;top:50%;left:50%;margin:-25px 0 0 -25px;cursor:pointer;box-shadow:0 0 15px rgba(0,210,255,.4);transition:none}
.button-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;min-width:180px;flex:1;max-width:260px}
button{height:48px;border:none;border-radius:10px;color:#fff;font-weight:700;font-size:13px;cursor:pointer;transition:transform .1s;text-transform:uppercase;-webkit-tap-highlight-color:transparent;touch-action:manipulation}
button:active{transform:scale(.92)}
.btn-green{background:#2ecc71}
.btn-purple{background:#9b59b6}
.btn-orange{background:#e67e22}
.btn-blue{background:#3498db}
.status-bar{margin:4px 0 8px;font-size:13px;color:#888;text-align:center;min-height:20px}
.ip-display{margin:4px 0 12px;background:#222;padding:6px 16px;border-radius:16px;font-family:monospace;font-size:13px;color:#00d2ff;border:1px solid #333}
@media(max-width:480px){
h1{font-size:16px}
.joystick-area{width:130px;height:130px;min-width:130px;min-height:130px}
.joystick-knob{width:44px;height:44px;margin:-22px 0 0 -22px}
button{height:42px;font-size:11px}
.button-grid{gap:6px}
}
</style>
</head>
<body>
<div class=header>
<h1>HEXAPOD</h1>
<div class=stats><span>FPS: <span id=fps>0</span></span><span id=lat>--</span></div>
</div>
<div class=camera-feed>
<div class=camera-overlay id=overlay>Connecting...</div>
<div class=live-indicator id=live style=display:none>Live</div>
<img id=camera src="" alt="">
</div>
<div class=controls-container>
<div class=joystick-area id=joystickArea><div class=joystick-knob id=knob></div></div>
<div class=button-grid>
<button class=btn-green data-a=stand>Stand</button>
<button class=btn-purple data-a=dance>Dance</button>
<button class=btn-orange data-a=jump>Jump</button>
<button class=btn-blue data-a=sit_down>Sit</button>
<button class=btn-orange data-a=strike>Strike</button>
<button class=btn-purple data-a=lunge>Lunge</button>
</div>
</div>
<div class=status-bar id=status>Ready</div>
<div class=ip-display id=ip>192.168.1.149</div>
<script>
var cam=document.getElementById('camera'),st=document.getElementById('status'),ov=document.getElementById('overlay'),lv=document.getElementById('live'),fp=document.getElementById('fps'),kn=document.getElementById('knob'),ar=document.getElementById('joystickArea'),h=window.location.host;
var cws,bws,ft=0,fc=0;
function cc(){cws=new WebSocket('ws://'+h+'/camera');cws.binaryType='blob';cws.onopen=function(){ov.textContent='Camera OK';lv.style.display=''};cws.onmessage=function(e){if(!(e.data instanceof Blob))return;var n=Date.now();fc++;if(n-ft>=1e3){fp.textContent=fc;fc=0;ft=n}var u=URL.createObjectURL(e.data),o=cam.src;cam.src=u;if(o&&o.startsWith('blob:'))URL.revokeObjectURL(o)};cws.onclose=function(){ov.textContent='Connecting...';lv.style.display='none';setTimeout(cc,2e3)};cws.onerror=function(){ov.textContent='WS Error'}}
function bc(){bws=new WebSocket('ws://'+h+'/control');bws.onopen=function(){st.textContent='Ready'};bws.onclose=function(){setTimeout(bc,2e3)}}
cc();bc();
function sc(a,s){var b=JSON.stringify({cmd:'motion',action:a,speed:s||60,duration_ms:0});if(bws&&bws.readyState===WebSocket.OPEN)bws.send(b);else fetch('/cmd',{method:'POST',headers:{'Content-Type':'application/json'},body:b}).catch(function(){})}
document.querySelectorAll('button[data-a]').forEach(function(b){b.addEventListener('click',function(){var a=this.getAttribute('data-a');st.textContent=a;sc(a)})});
var id=!1,ld='',ls=0,lt=0;
function gp(x,y){var r=ar.getBoundingClientRect(),cx=r.left+r.width/2,cy=r.top+r.height/2,dx=x-cx,dy=y-cy,d=Math.sqrt(dx*dx+dy*dy),c=Math.min(d,55);return{nx:d>0?(dx/d)*c:0,ny:d>0?(dy/d)*c:0,cl:c}}
function ms(x,y){if(!id)return;var p=gp(x,y);kn.style.transform='translate('+p.nx+'px,'+p.ny+'px)';var dir='',spd=0;if(p.cl>12){if(Math.abs(p.ny)>Math.abs(p.nx))dir=p.ny<0?'forward':'backward';else dir=p.nx<0?'left':'right';spd=Math.round(((p.cl-12)/43)*100)}if(dir){var n=Date.now();if(dir!==ld||Math.abs(spd-ls)>5||n-lt>80){ld=dir;ls=spd;lt=n;sc(dir,spd)}}else if(ld){sc('stop');ld=''}}
function me(){if(!id)return;id=!1;kn.style.transform='';if(ld){sc('stop');ld=''}}
kn.addEventListener('mousedown',function(e){e.preventDefault();id=!0;ld='';var p=gp(e.clientX,e.clientY);kn.style.transform='translate('+p.nx+'px,'+p.ny+'px)'});
window.addEventListener('mousemove',function(e){if(id)ms(e.clientX,e.clientY)});
window.addEventListener('mouseup',me);
kn.addEventListener('touchstart',function(e){e.preventDefault();var t=e.touches[0];id=!0;ld='';var p=gp(t.clientX,t.clientY);kn.style.transform='translate('+p.nx+'px,'+p.ny+'px)'});
window.addEventListener('touchmove',function(e){if(id){e.preventDefault();var t=e.touches[0];ms(t.clientX,t.clientY)}},{passive:false});
window.addEventListener('touchend',me);window.addEventListener('touchcancel',me);
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
static esp_err_t camera_ws_handler(httpd_req_t* req) {
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, "Camera WS connected fd=%d", fd);
        {
            std::lock_guard<std::mutex> lock(ws_mutex);
            ws_clients.push_back(fd);
            ESP_LOGI(TAG, "Camera WS clients count: %zu", ws_clients.size());
        }
        return ESP_OK;
    }
    ESP_LOGW(TAG, "Camera WS unexpected method %d", req->method);
    return ESP_FAIL;
}

static esp_err_t control_ws_handler(httpd_req_t* req) {
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, "Control WS connected fd=%d", fd);
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret == ESP_ERR_NOT_FOUND || ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "Control WS disconnected");
        return ret;
    }

    if (ret == ESP_OK && ws_pkt.len > 0) {
        uint8_t* buf = (uint8_t*)calloc(1, ws_pkt.len + 1);
        if (buf) {
            ws_pkt.payload = buf;
            ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
            if (ret == ESP_OK && g_server_instance) {
                ESP_LOGD(TAG, "Control WS cmd: %s", (char*)buf);
                g_server_instance->HandleMessage(std::string((char*)buf, ws_pkt.len));
            }
            free(buf);
        }
    }
    return ret;
}
#endif

// Background task to push camera frames
static void camera_stream_task(void* arg) {
    ESP_LOGI(TAG, "Camera stream task started on core %d", xPortGetCoreID());

    Camera* camera = Board::GetInstance().GetCamera();
    if (!camera) {
        ESP_LOGE(TAG, "Camera null, stopping");
        vTaskDelete(NULL);
        return;
    }

    Esp32Camera* esp_cam = dynamic_cast<Esp32Camera*>(camera);
    if (!esp_cam) {
        ESP_LOGE(TAG, "Camera not Esp32Camera, stopping");
        vTaskDelete(NULL);
        return;
    }
    esp_cam->SetStreaming(true);
    ESP_LOGI(TAG, "Camera streaming enabled");

    uint8_t* jpeg_buf = nullptr;
    size_t jpeg_len = 0;
    uint32_t frame_count = 0;
    uint32_t fail_count = 0;
    TickType_t last_log = xTaskGetTickCount();
    uint32_t idle_loops = 0;

    while (true) {
#ifdef CONFIG_HTTPD_WS_SUPPORT
        bool has_clients = false;
        {
            std::lock_guard<std::mutex> lock(ws_mutex);
            has_clients = !ws_clients.empty();
        }

        if (!has_clients) {
            idle_loops++;
            if (idle_loops % 120 == 0) {  // ~10s at 12fps delay per iteration
                ESP_LOGI(TAG, "Waiting for WS clients (%u iterations idle)", idle_loops);
            }
            vTaskDelay(pdMS_TO_TICKS(1000 / HexapodConst::CAMERA_STREAM_FPS));
            continue;
        }
        idle_loops = 0;

        TickType_t cap_start = xTaskGetTickCount();
        bool cap_ok = esp_cam->CaptureAsync();
        TickType_t cap_end = xTaskGetTickCount();
        if (!cap_ok) {
            fail_count++;
            if (fail_count % 10 == 1) {
                ESP_LOGW(TAG, "CaptureAsync failed (%lu times)", fail_count);
            }
            vTaskDelay(pdMS_TO_TICKS(1000 / HexapodConst::CAMERA_STREAM_FPS));
            continue;
        }
        fail_count = 0;

        uint8_t* data = nullptr;
        size_t len = 0;
        uint16_t w, h;
        uint32_t fmt;

        if (!esp_cam->GetReadyFrame(&data, &len, &w, &h, &fmt)) {
            ESP_LOGD(TAG, "GetReadyFrame returned false");
            vTaskDelay(pdMS_TO_TICKS(1000 / HexapodConst::CAMERA_STREAM_FPS));
            continue;
        }

        ESP_LOGD(TAG, "Frame: %dx%d fmt=0x%08x len=%zu cap_time=%dms", w, h, fmt, len, (cap_end - cap_start) * portTICK_PERIOD_MS);

        uint8_t* payload = data;
        size_t payload_len = len;

        bool is_jpeg_native = (fmt == V4L2_PIX_FMT_JPEG || fmt == V4L2_PIX_FMT_MJPEG);
        bool needs_jpeg = !is_jpeg_native;

        // Validate native JPEG data: must start with 0xFF 0xD8
        if (is_jpeg_native && data && len >= 2) {
            if (data[0] != 0xFF || data[1] != 0xD8) {
                ESP_LOGW(TAG, "JPEG fmt but no JPEG header (0x%02x%02x), force encode", data[0], data[1]);
                needs_jpeg = true;
            } else {
                ESP_LOGD(TAG, "Native JPEG confirmed: len=%zu magic=FFD8", len);
            }
        }

        if (needs_jpeg && data && len > 0) {
            if (image_to_jpeg(data, len, w, h, (v4l2_pix_fmt_t)fmt, 60, &jpeg_buf, &jpeg_len)) {
                payload = jpeg_buf;
                payload_len = jpeg_len;
                ESP_LOGD(TAG, "JPEG encoded: raw=%zu -> jpeg=%zu", len, jpeg_len);
            } else {
                ESP_LOGW(TAG, "JPEG encode failed (%dx%d fmt=0x%08x len=%zu), skipping", w, h, fmt, len);
                payload = nullptr;
                payload_len = 0;
            }
        } else if (is_jpeg_native) {
            ESP_LOGD(TAG, "Native JPEG frame: len=%zu", len);
        }

        if (payload && payload_len > 0) {
            std::vector<int> clients_copy;
            {
                std::lock_guard<std::mutex> lock(ws_mutex);
                clients_copy = ws_clients;
            }

            int sent_ok = 0;
            for (int fd : clients_copy) {
                httpd_ws_frame_t ws_pkt;
                memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
                ws_pkt.payload = payload;
                ws_pkt.len = payload_len;
                ws_pkt.type = HTTPD_WS_TYPE_BINARY;

                esp_err_t ret = httpd_ws_send_frame_async(server_handle, fd, &ws_pkt);
                if (ret == ESP_OK) {
                    sent_ok++;
                } else {
                    ESP_LOGW(TAG, "WS send fail fd=%d err=%s", fd, esp_err_to_name(ret));
                    std::lock_guard<std::mutex> lock(ws_mutex);
                    ws_clients.erase(std::remove(ws_clients.begin(), ws_clients.end(), fd), ws_clients.end());
                }
            }

            frame_count++;
            TickType_t now = xTaskGetTickCount();
            if (frame_count % 60 == 0 || (now - last_log) > pdMS_TO_TICKS(5000)) {
                uint32_t ms = (now - last_log) * portTICK_PERIOD_MS;
                bool jpeg_valid = (payload_len >= 2 && payload[0] == 0xFF && payload[1] == 0xD8);
                ESP_LOGI(TAG, "Camera: %u frames sent, %d/%zu clients ok, jpeg=%zuB, valid=%s, %ums since last log",
                         frame_count, sent_ok, clients_copy.size(), payload_len,
                         jpeg_valid ? "YES" : "NO", ms);
                last_log = now;
            }
        }

        if (needs_jpeg && jpeg_buf) {
            heap_caps_free(jpeg_buf);
            jpeg_buf = nullptr;
            jpeg_len = 0;
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(1000 / HexapodConst::CAMERA_STREAM_FPS));
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

    if (server_handle != nullptr) {
        httpd_stop(server_handle);
        server_handle = nullptr;
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    port_ = port;
    ESP_LOGI(TAG, "Starting HTTP command server on port %d", port);

    // Give LWIP some time to stabilize after getting IP
    vTaskDelay(pdMS_TO_TICKS(100));

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.ctrl_port = 32769;      // use different ctrl_port to avoid conflict with OTA server
    config.stack_size = 8192;
    config.max_open_sockets = 4;
    config.lru_purge_enable = true; // Auto-purge old connections
    config.uri_match_fn = httpd_uri_match_wildcard;

    esp_err_t ret = httpd_start(&server_handle, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(1000));
        ret = httpd_start(&server_handle, &config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start HTTP server again: %s", esp_err_to_name(ret));
            return false;
        }
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

    httpd_uri_t control_ws = {
        .uri        = "/control",
        .method     = HTTP_GET,
        .handler    = control_ws_handler,
        .user_ctx   = nullptr,
        .is_websocket = true
    };
    httpd_register_uri_handler(server_handle, &control_ws);

    xTaskCreatePinnedToCore(camera_stream_task, "camera_stream", HexapodConst::CAMERA_STREAM_TASK_STACK_SIZE, nullptr, 5, nullptr, HexapodConst::CAMERA_STREAM_TASK_CORE);
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
    else if (action == "sit_down") motion.Sit();
    else if (action == "dance")    motion.Dance(speed, duration_ms);
    else if (action == "stand")    motion.Stand();
    else if (action == "strike")   motion.Strike(speed, duration_ms);
    else if (action == "lunge")    motion.Lunge(speed, duration_ms);
    else if (action == "stop")     motion.Stop();
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
