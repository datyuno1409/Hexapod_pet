# Project Analysis Report
Generated: 2025-05-31

## System Architecture

```
                        ┌─────────────────────────────────────────────────┐
                        │           VOICEBOT BOARD (ESP32-S3)            │
                        │                                                 │
                        │  ┌──────────┐    ┌───────────────────────────┐  │
                        │  │   MQTT / │    │  HexapodProtocol (HTTP)     │  │
                        │  │ WebSocket│    │  SendCommandJson()          │  │
                        │  │ Protocol │    └────────────┬────────────────┘  │
                        │  └────┬─────┘                 │ UART TX          │
                        │       │  Voice/LLM           │ (GPIO)           │
                        │       ▼                      ▼                  │
                        │  ┌─────────────────────────────────────────┐    │
                        │  │  HexapodUartBridge (kMaster)             │    │
                        │  │  SendCommandJson() → HexapodUartLink     │    │
                        │  └─────────────────────────────────────────┘    │
                        └─────────────────────────────────────────────────┘
                                          │
                                    UART cable
                                ┌──────┴──────┐
                        ┌───────▼──────┐  ┌──────▼───────┐
                        │  BOT BOARD   │  │  DUAL BOARD   │
                        │  (ESP32-S3)  │  │  (ESP32-S3)   │
                        │              │  │               │
                        │  UART RX ──► │  │  UART+Dual    │
                        │  UartBridge  │  │  UartBridge   │
                        │  (kSlave)    │  │  (kDual)      │
                        │       │      │  │       │       │
                        │  ┌────┴────┐ │  │  ┌────┴────┐  │
                        │  │Dispatcher│ │  │  │Dispatcher│ │
                        │  └────┬─────┘ │  │  └────┬─────┘  │
                        │       │      │  │       │       │
                        │  ┌────┴────────▼──▼────────┴─────┐ │
                        │  │        HexapodMotion           │ │
                        │  │  MoveForward/Turn/Jump/etc.    │ │
                        │  └────────────┬───────────────────┘ │
                        │               │                      │
                        │  ┌────────────▼───────────────────┐  │
                        │  │       GaitGenerator             │  │
                        │  │  Walk(TRIPOD/RIPPLE/WAVE/JUMP) │  │
                        │  │  Update() → ComputeServoAngles │  │
                        │  └────────────┬───────────────────┘  │
                        │               │                      │
                        │  ┌────────────▼───────────────────┐  │
                        │  │      ServoController            │  │
                        │  │  2x PCA9685 @ I2C (0x40, 0x41) │  │
                        │  │  18 servos: SetServoAngles()   │  │
                        │  └────────────────────────────────┘  │
                        │                                        │
                        │  ┌─────────────────────────────────┐   │
                        │  │      HexapodServer (HTTP)        │   │
                        │  │  POST /cmd → HandleMessage()    │   │
                        │  │  GET  /    → INDEX_HTML joystick│   │
                        │  │  WS  /camera → camera_stream_task│   │
                        │  └─────────────────────────────────┘   │
                        └────────────────────────────────────────┘
```

### Module Data Flow

1. **VoiceBot** receives voice → LLM → MCP tools (`hexapod.move`, etc.)
2. MCP tools build JSON → `HexapodProtocol::SendCommand()`
3. Protocol routes to UART bridge → `HexapodUartLink::Send()` with binary framing (SOF + CRC16)
4. **Bot** receives frame → `ParseByte()` state machine → `HexapodUartBridge::HandleMessage()`
5. Bridge dispatches to `CommandDispatcher::Dispatch()` → `HexapodMotion` methods
6. Motion commands → `GaitGenerator::Walk()` → periodic `GaitGenerator::Update()`
7. `Update()` calls `ComputeServoAngles()` (IK + gait phase) → `ServoController::SetServoAnglesBatched()`
8. Batched I2C writes: 2 transactions (one per PCA9685 @ 0x40, 0x41) → 18 servos

---

## Issues Found

### 🔴 CRITICAL

#### CRIT-1: JPEG JPEG buffer leak in camera stream task
**File:** [hexapod_server.cc:320-356](xiaozhi-esp32_vietnam_new/main/hexapod_server.cc#L320-L356)  
**Impact:** Memory leak causes eventual heap exhaustion and crash on long streaming sessions.

The `image_to_jpeg()` call allocates `out` via `heap_caps_malloc()`, and the free path is:
```cpp
if (needs_jpeg && out) {
    heap_caps_free(out);  // only inside payload && payload_len > 0 block
    out = nullptr;
    out_len = 0;
}
```
But `out` is only freed inside the `if (payload && payload_len > 0)` block. If `payload` is null (e.g., capture failed), the previously-allocated `out` from the *prior iteration* is never freed. Also, `out` is a function-static-like variable persisting across loop iterations — its content is only freed conditionally.

**Fix:**
```cpp
// Always free previous buffer at top of loop before new allocation
if (out) {
    heap_caps_free(out);
    out = nullptr;
    out_len = 0;
}
// ... rest of capture logic, only allocate when needed
```

---

#### CRIT-2: Null pointer dereference in `HexapodMotion::Lunge()`
**File:** [hexapod_motion.cc:137-145](xiaozhi-esp32_vietnam_new/main/hexapod_motion.cc#L137-L145)  
**Impact:** Crash when calling `Lunge()` if `EnsureServo()` fails — `ctrl` dereferenced without null check.

```cpp
void HexapodMotion::Lunge(uint8_t intensity, uint32_t duration_ms) {
    if (!EnsureServo()) return;       // returns without error log if null
    // ...
    ServoController* ctrl = GetInstance().servo_controller_;  // raw pointer
    float lunge_pose[HexapodConst::NUM_SERVOS];
    // ...
    ctrl->SetServoAngles(lunge_pose);  // CRASH if ctrl is null
}
```

`EnsureServo()` logs an error but the `Lunge()` implementation re-fetches the raw pointer instead of using the guard. Compare with `Sit()` and `Stand()` which call `GetInstance().servo_controller_->SetNeutral()` with the same pattern — those work because `EnsureServo()` returns early on failure, but `Lunge()` re-fetches *after* `EnsureServo()` succeeded, so the pointer could be stale or the guard is unreliable.

**Fix:**
```cpp
void HexapodMotion::Lunge(uint8_t intensity, uint32_t duration_ms) {
    ServoController* ctrl = GetInstance().servo_controller_;
    if (!ctrl) {
        ESP_LOGE(TAG, "Lunge: ServoController not initialized");
        return;
    }
    // use ctrl directly, no redundant EnsureServo() call
    float lunge_pose[HexapodConst::NUM_SERVOS];
    // ...
}
```

---

#### CRIT-3: UART RX parser `reserve()` on untrusted length field
**File:** [hexapod_uart_link.cc:150-153](xiaozhi-esp32_vietnam_new/main/hexapod_uart_link.cc#L150-L153)  
**Impact:** Malformed/corrupted UART frame with large `len` field causes `std::string::reserve()` to attempt allocating `kMaxPayload` (2048 bytes) on every frame byte — normal frames have small payloads. More critically, if the RX buffer is corrupted (noise on UART line, baud mismatch), the parser could accumulate garbage bytes before a SOF is detected, and `reserve()` is called before bounds-check against `kMaxPayload`. The bounds check `if (rx_len_ > kMaxPayload)` exists at line 151, but the `clear()` + `reserve()` happens unconditionally at lines 152-153 *after* that check — which is correct, but `reserve(0)` triggers a heap allocation on every zero-length frame.

**Fix:**
```cpp
case ParseState::LenH: {
    rx_len_ |= static_cast<uint16_t>(byte) << 8;
    if (rx_len_ > kMaxPayload) { ResetParser(); break; }
    if (rx_len_ > 0) {
        rx_payload_.clear();
        rx_payload_.reserve(rx_len_);
    }
    state_ = rx_len_ ? ParseState::Payload : ParseState::CrcL;
    break;
}
```

---

### 🟠 HIGH

#### HIGH-1: All gait phase functions are no-ops — all gaits behave identically
**File:** [hexapod_gait_generator.cc:305-323](xiaozhi-esp32_vietnam_new/main/hexapod_gait_generator.cc#L305-L323)  
**Impact:** TRIPOD, RIPPLE, WAVE, BI_GAIT, and JUMP all use identical phase calculation (`return norm_time`). The intended behavior where legs lift in different patterns per gait type is completely broken — every gait looks the same (or walks with all legs in sync).

```cpp
float GaitGenerator::GetTripodPhase(float norm_time) { return norm_time; }   // OK
float GaitGenerator::GetRipplePhase(float norm_time) { return norm_time; }    // WRONG
float GaitGenerator::GetWavePhase(float norm_time)   { return norm_time; }    // WRONG
float GaitGenerator::GetBiGaitPhase(float norm_time) { return norm_time; }    // WRONG
float GaitGenerator::GetJumpPhase(float norm_time)  { return norm_time; }     // OK-ish
```

**Fix (Ripple — sequential leg lift):**
```cpp
float GaitGenerator::GetRipplePhase(float norm_time) {
    // Each leg lifts in sequence: leg 0 at 0.0, leg 1 at 0.167, ...
    // We need to know which leg we're computing for — add leg_id parameter
    // or compute per-leg offset in ComputeServoAngles()
    return norm_time; // placeholder — needs leg_id parameter
}
```

**Fix (Wave — full sequential):**
```cpp
float GaitGenerator::GetWavePhase(float norm_time, int leg_id) {
    float leg_offset = leg_id / 6.0f;
    return fmodf(norm_time + leg_offset, 1.0f);
}
```

---

#### HIGH-2: Null pointer access in `application.cc` JSON parsing
**File:** [application.cc:562](xiaozhi-esp32_vietnam_new/main/application.cc#L562)  
**Impact:** Crash on malformed incoming JSON messages where `"type"` field is missing or null.

```cpp
auto type = cJSON_GetObjectItem(root, "type");
if (strcmp(type->valuestring, "tts") == 0) {  // CRASH if type is null
```

No null check on `type` before dereferencing `type->valuestring`. The `cJSON_IsString()` check is missing.

**Fix:**
```cpp
auto type = cJSON_GetObjectItem(root, "type");
if (!cJSON_IsString(type)) {
    ESP_LOGW(TAG, "Invalid message: missing or non-string 'type'");
    return;
}
if (strcmp(type->valuestring, "tts") == 0) {
```

---

#### HIGH-3: Lateral motion (Left/Right) ignores coxa angle — robot slides instead of turning
**File:** [hexapod_gait_generator.cc:186-188](xiaozhi-esp32_vietnam_new/main/hexapod_gait_generator.cc#L186-L188)  
**Impact:** `TurnLeft()` and `TurnRight()` produce no meaningful turning because `ComputeLegIK()` always sets coxa to `NEUTRAL_ANGLE_COXA`. Direction is only used to flip `dir` sign (forward vs backward), but lateral turning requires coxa offset per leg.

```cpp
pose.coxa = HexapodConst::NEUTRAL_ANGLE_COXA;  // always neutral!
```

**Fix:** In `ComputeLegIK()`, add per-leg coxa offset based on direction:
```cpp
if (current_dir_ == LEFT || current_dir_ == RIGHT) {
    float turn_scale = (current_dir_ == LEFT ? 1.0f : -1.0f);
    float leg_offset = (leg_id % 2 == 0) ? turn_scale : -turn_scale;
    pose.coxa = HexapodConst::NEUTRAL_ANGLE_COXA
              + leg_offset * HexapodConst::COXA_TURN_OFFSET_DEG;
} else {
    pose.coxa = HexapodConst::NEUTRAL_ANGLE_COXA;
}
```

---

#### HIGH-4: `MoveServo()` callback executes synchronously — reentrancy risk
**File:** [hexapod_servo_controller.cc:307-327](xiaozhi-esp32_vietnam_new/main/hexapod_servo_controller.cc#L307-L327)  
**Impact:** The callback fires synchronously inside the ISR/thread context that called `MoveServo()`. If the callback triggers another servo move or accesses shared state, it creates reentrancy bugs. Currently `callback` is always `nullptr` in all callers, but this is a latent defect.

**Fix:**
```cpp
void ServoController::MoveServo(uint8_t servo_id, float target_angle, uint8_t speed,
                                  std::function<void()> callback) {
    // ... setup movement_ state ...
    if (callback) {
        // Defer callback to avoid reentrancy in callers
        // (Use FreeRTOS deferred call or post to a task queue)
    }
}
```

---

#### HIGH-5: IK swing height = swing forward — unnatural motion
**File:** [hexapod_gait_generator.cc:281-285](xiaozhi-esp32_vietnam_new/main/hexapod_gait_generator.cc#L281-L285)  
**Impact:** The Z-axis (height) swing uses the same `SWING_AMPLITUDE_DEG` as the X-axis (forward), meaning the foot lifts as far forward as it swings forward. This creates flat, robotic-looking trajectories instead of a natural arc.

```cpp
float swing_height = HexapodConst::SWING_AMPLITUDE_DEG *  // same as forward!
    std::sin(swing_local_phase * M_PI);
```

**Fix:**
```cpp
float swing_height = HexapodConst::SWING_HEIGHT_AMPLITUDE_DEG *
    std::sin(swing_local_phase * M_PI);  // dedicated height constant
```

---

#### HIGH-6: HTTP handler accepts any method on `/` and `/ping` URIs
**File:** [hexapod_server.cc:419-429](xiaozhi-esp32_vietnam_new/main/hexapod_server.cc#L419-L429)  
**Impact:** The root and ping URIs only register `HTTP_GET`, but `httpd_uri_match_wildcard` matching could allow POST to reach `root_get_handler` if wildcard matching is active. The `command_post_handler` buffer is 512 bytes — no enforcement of `Content-Length` header checking beyond the size guard.

**Fix:** Add explicit content-type and method validation:
```cpp
if (req->method != HTTP_POST) {
    httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "POST only");
    return ESP_FAIL;
}
```

---

### 🟡 MEDIUM

#### MED-1: `motion_mutex_` never deleted (minor leak on shutdown)
**File:** [hexapod_motion.h:43](xiaozhi-esp32_vietnam_new/main/hexapod_motion.h#L43), [hexapod_motion.cc:20](xiaozhi-esp32_vietnam_new/main/hexapod_motion.cc#L20)  
**Impact:** The `SemaphoreHandle_t` created with `xSemaphoreCreateMutex()` is never freed. On systems with dynamic teardown (OTA, deep sleep wake), this accumulates.

**Fix:**
```cpp
HexapodMotion::~HexapodMotion() {
    if (motion_mutex_) {
        vSemaphoreDelete(motion_mutex_);
    }
}
```

---

#### MED-2: `Resume()` timestamp math is fragile
**File:** [hexapod_gait_generator.cc:126-131](xiaozhi-esp32_vietnam_new/main/hexapod_gait_generator.cc#L126-L131)  
**Impact:** `motion_start_time_` is adjusted with integer subtraction, but `esp_timer_get_time()` returns `uint64_t`. If elapsed time exceeds `motion_start_time_` (timer wraps), this produces wrong timing.

```cpp
motion_start_time_ = esp_timer_get_time() / 1000 - GetElapsedTime();
```

**Fix:**
```cpp
uint32_t now = esp_timer_get_time() / 1000;
motion_start_time_ = now - GetElapsedTime();
```

---

#### MED-3: No CRC mismatch counter or error rate tracking
**File:** [hexapod_uart_link.cc:174-179](xiaozhi-esp32_vietnam_new/main/hexapod_uart_link.cc#L174-L179)  
**Impact:** CRC errors are logged but never counted. In noisy environments (real robot with motor EMI), silent frame corruption will go unnoticed — no alert, no error reporting back to VoiceBot.

**Fix:**
```cpp
static uint32_t crc_error_count = 0;
// in ParseByte CrcH case:
if (expected != rx_crc_) {
    crc_error_count++;
    if (crc_error_count % 100 == 0) {
        ESP_LOGW(TAG, "CRC errors: %u (rate: %.1f%%)",
            crc_error_count, (float)crc_error_count / (crc_error_count + valid_count) * 100);
    }
}
```

---

#### MED-4: `SweepTest()` blocks FreeRTOS for 1.8+ seconds
**File:** [hexapod_motion.cc:285-306](xiaozhi-esp32_vietnam_new/main/hexapod_motion.cc#L285-L306)  
**Impact:** The sweep test runs 91 iterations × 10ms delay = 910ms per sweep, 2 sweeps = 1.82s of pure blocking. During this time, Wi-Fi timers, audio, and UART RX are starved. This is a debug function but could accidentally be called in production.

**Fix:** Guard with compile-time flag or non-blocking state machine:
```cpp
#ifdef SERVO_SWEEP_TEST_ENABLED
// Non-blocking: track phase in member state, advance one step per Update() call
#endif
```

---

#### MED-5: Task monitor is commented out
**File:** [main.cc:73](xiaozhi-esp32_vietnam_new/main/main.cc#L73)  
**Impact:** Stack overflow detection is disabled — in production with many FreeRTOS tasks (audio, camera, UART, HTTP), stack exhaustion is a real risk.

**Fix:**
```cpp
xTaskCreatePinnedToCore(task_monitor, "TaskMonitor", 3 * 1024, NULL, 1, NULL, tskNO_AFFINITY);
```

---

### 🟢 LOW

#### LOW-1: `UpdateGaitPhase()` is defined but never does anything
**File:** [hexapod_gait_generator.cc:179](xiaozhi-esp32_vietnam_new/main/hexapod_gait_generator.cc#L179)  
Empty virtual-looking method that could mislead future developers.

**Fix:** Either implement it or remove the call from `Update()` and the declaration from the header.

---

#### LOW-2: Inconsistent string concatenation in MCP tools
**File:** [hexapod_mcp_tools.cc:101](xiaozhi-esp32_vietnam_new/main/hexapod_mcp_tools.cc#L101)  
Double `std::string()` wrapping — `std::string(std::string(...))` — harmless but ugly.

**Fix:**
```cpp
return std::string("{\"status\":\"ok\",\"streaming\":") + (enable ? "true" : "false") + "}";
```

---

#### LOW-3: `application.cc` hardcodes board type string comparison
**File:** [application.cc:392,416,503-517](xiaozhi-esp32_vietnam_new/main/application.cc#L392)  
Uses `board_type == "hexapod_bot"` string comparison instead of an enum or Kconfig define. Renaming a board type in Kconfig won't catch these.

**Fix:** Use `#ifdef CONFIG_BOARD_TYPE_HEXAPOD_BOT` guards consistently, matching the pattern already used in `hexapod_uart_bridge.cc`.

---

#### LOW-4: `HexapodServer::SendResponse()` only logs — never sends
**File:** [hexapod_server.cc:473-475](xiaozhi-esp32_vietnam_new/main/hexapod_server.cc#L473-L475)  
The WebSocket response path is unimplemented — all client communication is one-way (HTTP POST in, no response body beyond status). The `command_post_handler` always returns `{"status":"ok"}` regardless of success/failure.

**Fix:**
```cpp
void HexapodServer::SendResponse(const std::string& response) {
    std::lock_guard<std::mutex> lock(ws_mutex);
    for (int fd : ws_clients) {
        httpd_ws_frame_t frame = {
            .final = true, .fragmented = false,
            .type = HTTPD_WS_TYPE_TEXT,
            .payload = (uint8_t*)response.c_str(),
            .len = response.size()
        };
        httpd_ws_send_frame_async(server_handle, fd, &frame);
    }
}
```

---

#### LOW-5: Gait cycle time is hardcoded as `600` for BI_GAIT (alias of TRIPOD)
**File:** [hexapod_gait_generator.cc:86-87](xiaozhi-esp32_vietnam_new/main/hexapod_gait_generator.cc#L86-L87)  
**Impact:** BI_GAIT is supposed to be "bi-pedal like" (different timing), but it reuses TRIPOD's 600ms cycle. The name is misleading since it's functionally identical to TRIPOD in current implementation.

---

## Recommended Fixes

### Priority 1 — CRITICAL (fix immediately)

```cpp
// CRIT-1: Always free JPEG buffer at top of loop
static void camera_stream_task(void* arg) {
    // ...
    uint8_t* out = nullptr;
    size_t out_len = 0;
    while (true) {
        // FREE PREVIOUS BUFFER FIRST
        if (out) {
            heap_caps_free(out);
            out = nullptr;
            out_len = 0;
        }
        cleanup_dead_clients(server_handle);
        // ... capture logic ...
        if (needs_jpeg && data && len > 0) {
            if (image_to_jpeg(data, len, w, h, (v4l2_pix_fmt_t)fmt, 60, &out, &out_len)) {
                // send frame...
            }
        }
        // remove the conditional free at the bottom — now handled at top
        vTaskDelay(pdMS_TO_TICKS(1000 / HexapodConst::CAMERA_STREAM_FPS));
    }
}
```

### Priority 2 — HIGH (fix in next sprint)

```cpp
// HIGH-1: Wave gait phase with per-leg offset
float GaitGenerator::GetWavePhase(float norm_time, int leg_id) {
    float leg_offset = static_cast<float>(leg_id) / 6.0f;
    return fmodf(norm_time + leg_offset, 1.0f);
}
// Call from ComputeServoAngles():
// leg_phase = GetWavePhase(phase_0_to_1, leg);
```

```cpp
// HIGH-2: Null-safe JSON type check in application.cc
auto type = cJSON_GetObjectItem(root, "type");
if (!cJSON_IsString(type) || !type->valuestring) {
    ESP_LOGW(TAG, "Invalid message: missing 'type'");
    return;
}
```

```cpp
// HIGH-3: Lateral turning with coxa offset
void GaitGenerator::ComputeServoAngles() {
    // ... inside per-leg loop:
    if (current_gait_ != JUMP) {
        float coxa, femur, tibia;
        bool reachable = TripodGait::InverseKinematics(x, y, z, coxa, femur, tibia);
        if (reachable && (current_dir_ == LEFT || current_dir_ == RIGHT)) {
            float turn = (current_dir_ == LEFT ? 1.0f : -1.0f);
            float side = (leg % 2 == 0) ? turn : -turn;
            coxa += side * HexapodConst::COXA_TURN_OFFSET_DEG;
        }
        // ... use coxa/femur/tibia
    }
}
```

### Priority 3 — MEDIUM (address when convenient)

```cpp
// MED-1: Destructor for mutex cleanup
HexapodMotion::~HexapodMotion() {
    if (motion_mutex_) {
        vSemaphoreDelete(motion_mutex_);
    }
}
```

```cpp
// MED-3: CRC error counter
// Add to hexapod_uart_link.cc:
static uint32_t g_crc_errors = 0;
// In CrcH case:
if (expected != rx_crc_) {
    g_crc_errors++;
    if (g_crc_errors % 256 == 0) {
        ESP_LOGW(TAG, "CRC error count: %u", g_crc_errors);
    }
}
```

### Priority 4 — LOW (cosmetic / defensive)

```cpp
// LOW-3: Consistent board type macros
// In application.cc, replace:
//   if (board_type == "hexapod_bot")
// with:
#ifdef CONFIG_BOARD_TYPE_HEXAPOD_BOT
// matching hexapod_uart_bridge.cc pattern
#endif
```

---

## Optimization Roadmap

| Priority | Item | Effort | Impact |
|----------|------|--------|--------|
| P0 | CRIT-1: JPEG buffer leak fix | 5 min | Prevents crash after 10-30 min streaming |
| P0 | CRIT-2: Lunge null deref guard | 2 min | Prevents crash on early servo failure |
| P0 | CRIT-3: UART RX reserve guard | 5 min | Prevents heap exhaustion on noisy UART |
| P1 | HIGH-1: Wave/Ripple gait phases | 2 hours | Enables all gait types to actually work |
| P1 | HIGH-2: JSON null checks | 10 min | Prevents crash on malformed server messages |
| P1 | HIGH-3: Turning coxa offset | 30 min | Enables actual robot turning |
| P1 | HIGH-5: Separate swing height constant | 5 min | More natural foot trajectories |
| P2 | MED-1: Mutex destructor | 5 min | Clean shutdown / OTA safety |
| P2 | MED-3: CRC error counter | 15 min | Debugability for UART noise issues |
| P2 | MED-4: Non-blocking sweep test | 1 hour | Prevents RTOS starvation |
| P3 | LOW-3: Board type macro consistency | 20 min | Compile-time safety |
| P3 | LOW-4: WS response implementation | 30 min | Enables server-to-client status |

### Estimated Total: ~5 hours for all P0-P2 fixes

---

## Architecture Strengths

- **Singleton pattern** consistent across all major components — easy lifecycle management
- **Batched I2C writes** in `SetServoAnglesBatched()` — 2 transactions vs 72 individual writes, well-optimized
- **Binary UART framing** with CRC16-CCITT — robust against noise
- **Triple-board architecture** (VoiceBot / Bot / Dual) — flexible deployment
- **MCP tool integration** — enables LLM-driven robot control
- **HTTP + WebSocket dual transport** — graceful degradation

## Architecture Weaknesses

- **No state machine for gait transitions** — calling `Walk()` while already walking just refreshes parameters, can cause servo jumps
- **No watchdog timer** — if UART RX task hangs, no recovery mechanism
- **Hard-coded magic numbers** throughout (512-byte HTTP buffer, 4096 stack for camera task, 100ms I2C timeout)
- **`application.cc` is 1800+ lines** — should be split into subsystems
- **No connection state management** for HTTP client — `esp_http_client_init/perform/cleanup` per request (no connection reuse)
