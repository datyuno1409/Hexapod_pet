#include "board.h"
#include "boards/common/wifi_board.h"
#include "config.h"
#include "display/lcd_display.h"
#include "hexapod_attack_patterns.h"
#include "hexapod_gait_generator.h"
#include "hexapod_servo_controller.h"
#include "led/single_led.h"
#include "system_reset.h"
#include "hexapod_uart_bridge.h"
#include "hexapod_motion.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "boards/common/esp32_camera.h"
#include "hexapod_constants.h"
#include <cJSON.h>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_video_init.h>
#include <esp_wifi.h>
#include <esp_heap_caps.h>

#define TAG "HexapodBotBoard"

// Display orientation for ST7735 0.96" 80x160
// Landscape mode: swap_xy=true, mirror_x=true, mirror_y=false
#define DISPLAY_SWAP_XY     true
#define DISPLAY_MIRROR_X    true
#define DISPLAY_MIRROR_Y    false
#define DISPLAY_INVERT_COLOR true
#define DISPLAY_OFFSET_X    1
#define DISPLAY_OFFSET_Y    26
#define DISPLAY_SPI_MODE    0

class HexapodBotBoard : public WifiBoard {
private:
  // I2C servo bus for PCA9685 controllers
  i2c_master_bus_handle_t servo_i2c_bus_ = nullptr;

  // Motion layer update task (20Hz = 50ms interval)
  TaskHandle_t motion_task_handle_ = nullptr;


  // Camera
  Camera *camera_ = nullptr;

  // TFT Display (ST7735 0.96" 80x160 via SPI, using SpiLcdDisplay)
  Display* display_ = nullptr;

  // ========================================================================
  // Servo I2C Bus Initialization (for PCA9685 @ 0x40, 0x41)
  // ========================================================================
  void InitializeServoI2c() {
    ESP_LOGI(TAG, "Initializing Servo I2C bus for PCA9685...");

    i2c_master_bus_config_t bus_config = {
        .i2c_port = (i2c_port_t)SERVO_I2C_PORT,
        .sda_io_num = SERVO_I2C_SDA,
        .scl_io_num = SERVO_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags =
            {
                .enable_internal_pullup = 1,
            },
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &servo_i2c_bus_));
    ESP_LOGI(TAG, "Servo I2C bus initialized (GPIO %d SDA, %d SCL)",
             SERVO_I2C_SDA, SERVO_I2C_SCL);
  }

  // ========================================================================
  // Motion Layer Initialization (ServoController -> GaitGenerator -> AttackPatterns)
  // ========================================================================
  void InitializeMotionLayer() {
    ESP_LOGI(TAG, "Initializing motion layer...");

    auto &servo = ServoController::GetInstance();
    if (!servo.Initialize(servo_i2c_bus_)) {
      ESP_LOGE(TAG, "ServoController initialization failed!");
      return;
    }

    auto &gait = GaitGenerator::GetInstance();
    if (!gait.Initialize(&servo)) {
      ESP_LOGE(TAG, "GaitGenerator initialization failed!");
      return;
    }

    auto &attack = AttackPatterns::GetInstance();
    if (!attack.Initialize(&gait, &servo)) {
      ESP_LOGE(TAG, "AttackPatterns initialization failed!");
      return;
    }

    // Create dedicated real-time task for motion updates (20Hz)
    // Priority 15 (high) on core 0 for deterministic timing
    BaseType_t ok = xTaskCreatePinnedToCore(
        [](void* arg) {
          HexapodBotBoard* self = static_cast<HexapodBotBoard*>(arg);
          TickType_t last_wake = xTaskGetTickCount();
          const TickType_t interval = pdMS_TO_TICKS(50); // 20Hz

          while (true) {
            // Update gait and attack patterns
            GaitGenerator::GetInstance().Update();
            AttackPatterns::GetInstance().UpdateFrame();

            // Sleep until next period (exactly 50ms)
            vTaskDelayUntil(&last_wake, interval);
          }
        },
        "motion_update", 4096, this, 15, &motion_task_handle_, 0);

    if (ok != pdPASS) {
      ESP_LOGE(TAG, "Failed to create motion update task!");
      return;
    }

    ESP_LOGI(TAG, "Motion layer ready: ServoController + GaitGenerator + "
                  "AttackPatterns @ 20Hz (task priority 15, core 0)");
  }

  // ========================================================================
  // TFT Display Initialization (ST7735 0.96" 80x160 via SPI -> SpiLcdDisplay)
  // ========================================================================
  void InitializeDisplay() {
    ESP_LOGI(TAG, "Initializing 0.96\" TFT display (ST7735 SPI)...");

    // STEP 1: Backlight ON early
    gpio_config_t bl_config = {
        .pin_bit_mask = (1ULL << TFT_SMALL_SPI_BLK),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&bl_config);
    gpio_set_level(TFT_SMALL_SPI_BLK, 1);
    ESP_LOGI(TAG, "Backlight GPIO %d set HIGH", TFT_SMALL_SPI_BLK);

    // STEP 2: Initialize SPI bus
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = TFT_SMALL_SPI_SDA;
    buscfg.miso_io_num = GPIO_NUM_NC;
    buscfg.sclk_io_num = TFT_SMALL_SPI_SCL;
    buscfg.quadwp_io_num = GPIO_NUM_NC;
    buscfg.quadhd_io_num = GPIO_NUM_NC;
    buscfg.max_transfer_sz = TFT_SMALL_WIDTH * TFT_SMALL_HEIGHT * sizeof(uint16_t);
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI bus initialized for TFT display");

    // STEP 3: Create panel IO (SPI)
    esp_lcd_panel_io_handle_t panel_io = nullptr;
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = TFT_SMALL_SPI_CS;
    io_config.dc_gpio_num = TFT_SMALL_SPI_DC;
    io_config.spi_mode = DISPLAY_SPI_MODE;
    io_config.pclk_hz = TFT_SMALL_SPI_FREQ_HZ;
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io));
    ESP_LOGI(TAG, "LCD panel IO created");

    // STEP 4: Create ST7789 panel handle (works for ST7735 too)
    esp_lcd_panel_handle_t panel = nullptr;
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = TFT_SMALL_SPI_RES;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;
    panel_config.bits_per_pixel = 16;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));
    ESP_LOGI(TAG, "ST7789/ST7735 panel handle created");

    // STEP 5: Reset and initialize panel
    esp_lcd_panel_reset(panel);

    // Send ST7735S manual initialization sequence before calling esp_lcd_panel_init
    ESP_LOGI(TAG, "Sending manual ST7735S initialization sequence...");
    
    // 1. Software Reset (SWRESET)
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(panel_io, 0x01, nullptr, 0));
    vTaskDelay(pdMS_TO_TICKS(150));

    // 2. Sleep Out (SLPOUT)
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(panel_io, 0x11, nullptr, 0));
    vTaskDelay(pdMS_TO_TICKS(500));

    // 3. Frame Rate Control (FRMCTR1)
    uint8_t frmctr1_data[] = {0x01, 0x2C, 0x2D};
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(panel_io, 0xB1, frmctr1_data, sizeof(frmctr1_data)));

    // 4. Color Mode / Interface Pixel Format (COLMOD)
    uint8_t colmod_data[] = {0x05}; // 16-bit color (RGB565)
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(panel_io, 0x3A, colmod_data, sizeof(colmod_data)));

    // 5. Display On (DISPON)
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(panel_io, 0x29, nullptr, 0));
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_lcd_panel_init(panel);
    esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR);
    esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
    esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
    esp_lcd_panel_set_gap(panel, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y);
    ESP_LOGI(TAG, "Panel initialized (swap_xy=%d, mirror_x=%d, mirror_y=%d, gap=%d,%d)",
             DISPLAY_SWAP_XY, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y,
             DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y);

    // STEP 6: Create SpiLcdDisplay (LVGL handles all rendering)
    // Landscape logical resolution is 160x80 after swap_xy.
    display_ = new SpiLcdDisplay(panel_io, panel,
                                  TFT_SMALL_HEIGHT, TFT_SMALL_WIDTH,
                                  0, 0,
                                  DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y,
                                  DISPLAY_SWAP_XY);
    ESP_LOGI(TAG, "SpiLcdDisplay created (landscape 160x80, panel gap applied)");
  }

  // ========================================================================
  // Camera Initialization (I2C for OV5640)
  // ========================================================================
  void InitializeCamera() {
    ESP_LOGI(TAG, "Initializing Hexapod Camera (OV5640)...");

    esp_cam_ctlr_dvp_pin_config_t dvp_pin_config = {
        .data_width = CAM_CTLR_DATA_WIDTH_8,
        .data_io =
            {
                [0] = CAMERA_D0_PIN,
                [1] = CAMERA_D1_PIN,
                [2] = CAMERA_D2_PIN,
                [3] = CAMERA_D3_PIN,
                [4] = CAMERA_D4_PIN,
                [5] = CAMERA_D5_PIN,
                [6] = CAMERA_D6_PIN,
                [7] = CAMERA_D7_PIN,
            },
        .vsync_io = CAMERA_VSYNC_PIN,
        .de_io = CAMERA_HREF_PIN,
        .pclk_io = CAMERA_PCLK_PIN,
        .xclk_io = CAMERA_XCLK_PIN,
    };

    esp_video_init_sccb_config_t sccb_config = {
        .init_sccb = true,
        .i2c_config =
            {
                .port = (i2c_port_t)CAMERA_I2C_PORT,
                .scl_pin = CAMERA_I2C_SCL,
                .sda_pin = CAMERA_I2C_SDA,
            },
        .freq = CAMERA_I2C_FREQ_HZ,
    };

    esp_video_init_dvp_config_t dvp_config = {
        .sccb_config = sccb_config,
        .reset_pin = CAMERA_RESET_PIN,
        .pwdn_pin = CAMERA_PWDN_PIN,
        .dvp_pin = dvp_pin_config,
        .xclk_freq = HexapodConst::CAMERA_XCLK_FREQ_HZ,  // Stable OV5640 XCLK for clean stream on COM10
    };

    esp_video_init_config_t video_config = {
        .dvp = &dvp_config,
    };

    camera_ = new Esp32Camera(video_config, HexapodConst::CAMERA_STREAM_DEFAULT_WIDTH, HexapodConst::CAMERA_STREAM_DEFAULT_HEIGHT);
    if (camera_) {
      ESP_LOGI(TAG, "Camera object created");
    } else {
      ESP_LOGE(TAG, "Failed to create camera object");
    }
  }

public:
  HexapodBotBoard() {
    ESP_LOGI(TAG, "Initializing Hexapod Bot Board...");

    InitializeServoI2c();
    InitializeMotionLayer();
    // Release JTAG pin GPIO 40 for TFT RES use
    gpio_reset_pin((gpio_num_t)40);
    InitializeDisplay();
    // Camera init with safety check to prevent boot loop on failure
    try {
      InitializeCamera();
    } catch (...) {
      ESP_LOGE(TAG, "Camera initialization threw exception - skipping");
    }

    // UART bridge - Bot acts as SLAVE
#ifdef HEXAPOD_UART_PORT
    auto& uart_bridge = HexapodUartBridge::GetInstance();
    uart_bridge.Start(HexapodUartBridge::Role::kSlave);
    ESP_LOGI(TAG, "UART bridge initialized as SLAVE for VoiceBot communication");
#endif

    esp_err_t wifi_ps_ret = esp_wifi_set_ps(WIFI_PS_NONE);
    ESP_LOGI(TAG, "WiFi power save disabled for camera stream: %s", esp_err_to_name(wifi_ps_ret));
    esp_err_t wifi_bw_ret = esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
    ESP_LOGI(TAG, "WiFi bandwidth forced HT20 for stream stability: %s", esp_err_to_name(wifi_bw_ret));
    ESP_LOGI(TAG, "PSRAM free after camera init: %u bytes", (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    ESP_LOGI(TAG, "Hexapod Bot board initialized successfully");

    // Removed startup_anim task to avoid stack overflow during boot.
    // The robot will naturally move to neutral pose during ServoController initialization.
  }



  virtual std::string GetBoardType() override { return "hexapod_bot"; }

  virtual Led *GetLed() override {
    static SingleLed led(BUILTIN_LED_GPIO);
    return &led;
  }

  virtual AudioCodec *GetAudioCodec() override {
    return nullptr;
  }

  virtual Display *GetDisplay() override {
    return display_;
  }

  virtual Camera *GetCamera() override { return camera_; }

  virtual void SetPowerSaveMode(bool /*enabled*/) override {
    // TODO: implement power save mode
  }

  virtual std::string GetBoardJson() override {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "board", "hexapod_bot");
    cJSON_AddStringToObject(root, "description", "Hexapod Robot Controller");
    cJSON_AddNumberToObject(root, "servo_count", 18);
    cJSON_AddNumberToObject(root, "pca9685_count", 2);
    char *json_str = cJSON_PrintUnformatted(root);
    std::string result(json_str);
    cJSON_free(json_str);
    cJSON_Delete(root);
    return result;
  }

  virtual std::string GetDeviceStatusJson() override {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "board", "hexapod_bot");
    cJSON_AddBoolToObject(root, "servo_i2c_ready", servo_i2c_bus_ != nullptr);
    cJSON_AddBoolToObject(root, "motion_active",
                          GaitGenerator::GetInstance().IsWalking());
    char *json_str = cJSON_PrintUnformatted(root);
    std::string result(json_str);
    cJSON_free(json_str);
    cJSON_Delete(root);
    return result;
  }
};

DECLARE_BOARD(HexapodBotBoard);
