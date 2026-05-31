#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "display/emote_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "led/single_led.h"
#include "assets/lang_config.h"
#include "backlight.h"
#include "hexapod_servo_controller.h"
#include "hexapod_gait_generator.h"
#include "hexapod_attack_patterns.h"
#include "boards/common/esp32_camera.h"

#include <cJSON.h>
#include <wifi_station.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_st7789.h>
#include <esp_timer.h>

#define TAG "HexapodDualBoard"

class HexapodDualBoard : public WifiBoard {
private:
    // ========================================================================
    // MAIN TFT Display (ST7789 240x240 - VoiceBot UI)
    // ========================================================================
    esp_lcd_panel_io_handle_t main_tft_panel_io_ = nullptr;
    esp_lcd_panel_handle_t main_tft_panel_ = nullptr;
    Display* main_display_ = nullptr;

    // ========================================================================
    // EMOTION Display (ST7735 80x160 - Bot emotion display)
    // ========================================================================
    esp_lcd_panel_io_handle_t emotion_panel_io_ = nullptr;
    esp_lcd_panel_handle_t emotion_panel_ = nullptr;
    Display* emotion_display_ = nullptr;

    // ========================================================================
    // I2C buses
    // ========================================================================
    i2c_master_bus_handle_t servo_i2c_bus_ = nullptr;
    i2c_master_bus_handle_t camera_i2c_bus_ = nullptr;

    // ========================================================================
    // Camera
    // ========================================================================
    Camera* camera_ = nullptr;

    // ========================================================================
    // Buttons
    // ========================================================================
    Button boot_button_;
    Button volume_up_button_;
    Button volume_down_button_;

    // ========================================================================
    // Motion update task (20Hz = 50ms interval)
    // ========================================================================
    TaskHandle_t motion_task_handle_ = nullptr;

    // ========================================================================
    // MAIN TFT Display Initialization (ST7789 240x240 via SPI)
    // ========================================================================
    void InitializeMainTftDisplay() {
        ESP_LOGI(TAG, "Initializing main TFT ST7789 display (240x240)...");

        // Initialize SPI bus for main display
        spi_bus_config_t bus_config = {
            .mosi_io_num = TFT_SPI_MOSI,
            .miso_io_num = TFT_SPI_MISO,
            .sclk_io_num = TFT_SPI_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = TFT_WIDTH * TFT_HEIGHT * 2,
        };
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO));
        ESP_LOGI(TAG, "SPI bus initialized for main display");

        // Initialize LCD I/O
        esp_lcd_panel_io_spi_config_t io_config = {
            .cs_gpio_num = TFT_SPI_CS,
            .dc_gpio_num = TFT_DC_PIN,
            .spi_mode = 0,
            .pclk_hz = TFT_SPI_FREQ_HZ,
            .trans_queue_depth = 10,
            .on_color_trans_done = nullptr,
            .user_ctx = nullptr,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_config, &main_tft_panel_io_));

        // Initialize ST7789 panel
        esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = TFT_RST_PIN,
            .rgb_endian = LCD_RGB_ENDIAN_RGB,
            .bits_per_pixel = 16,
            .flags = {
                .reset_active_high = 0,
            },
            .vendor_config = nullptr,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(main_tft_panel_io_, &panel_config, &main_tft_panel_));

        // Reset and initialize display
        ESP_ERROR_CHECK(esp_lcd_panel_reset(main_tft_panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_init(main_tft_panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(main_tft_panel_, true));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(main_tft_panel_, true));

        // Create SPI LCD display abstraction
        main_display_ = new SpiLcdDisplay(main_tft_panel_io_, main_tft_panel_,
                                          TFT_WIDTH, TFT_HEIGHT,
                                          0, 0, false, false, false);
        ESP_LOGI(TAG, "Main TFT display initialized (240x240)");
    }

    // ========================================================================
    // EMOTION Display Initialization (ST7735 80x160 via SPI)
    // ========================================================================
    void InitializeEmotionDisplay() {
        ESP_LOGI(TAG, "Initializing emotion display (ST7735 80x160)...");

        // Note: Uses same SPI bus as main display (SPI3_HOST)
        // LCD I/O config
        esp_lcd_panel_io_spi_config_t io_config = {
            .cs_gpio_num = TFT_SMALL_SPI_CS,
            .dc_gpio_num = TFT_SMALL_SPI_DC,
            .spi_mode = 3,
            .pclk_hz = TFT_SMALL_SPI_FREQ_HZ,
            .trans_queue_depth = 10,
            .on_color_trans_done = nullptr,
            .user_ctx = nullptr,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_config, &emotion_panel_io_));
        ESP_LOGI(TAG, "Emotion display LCD I/O configured");

        // ST7735-compatible panel init
        esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = TFT_SMALL_SPI_RES,
            .rgb_endian = LCD_RGB_ENDIAN_BGR,
            .bits_per_pixel = 16,
            .flags = {
                .reset_active_high = 0,
            },
            .vendor_config = nullptr,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(emotion_panel_io_, &panel_config, &emotion_panel_));

        // Reset and initialize
        ESP_ERROR_CHECK(esp_lcd_panel_reset(emotion_panel_));
        vTaskDelay(pdMS_TO_TICKS(120));
        ESP_ERROR_CHECK(esp_lcd_panel_init(emotion_panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(emotion_panel_, true));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(emotion_panel_, true));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(emotion_panel_, true, false));
        ESP_ERROR_CHECK(esp_lcd_panel_set_gap(emotion_panel_, 1, 26));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(emotion_panel_, true));

        // Initialize backlight
        gpio_config_t bl_config = {
            .pin_bit_mask = (1ULL << TFT_SMALL_SPI_BLK),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&bl_config);
        gpio_set_level(TFT_SMALL_SPI_BLK, 1);

        // Create emotion display
        emotion_display_ = new emote::EmoteDisplay(emotion_panel_, emotion_panel_io_,
                                                    TFT_SMALL_HEIGHT, TFT_SMALL_WIDTH);

        // Setup emotion display layout (160x80 landscape)
        auto* emote_disp = static_cast<emote::EmoteDisplay*>(emotion_display_);
        emote_disp->AddLayoutData("status_icon",  "GFX_ALIGN_TOP_LEFT",    2,  0);
        emote_disp->AddLayoutData("toast_label",  "GFX_ALIGN_TOP_LEFT",   20,  1, 138, 14);
        emote_disp->AddLayoutData("clock_label",  "GFX_ALIGN_TOP_LEFT",   20,  0, 138, 16);
        emote_disp->AddLayoutData("eye_anim",     "GFX_ALIGN_LEFT_MID",    4, 10);
        emote_disp->AddLayoutData("listen_anim",  "GFX_ALIGN_RIGHT_MID", -10,  8);

        ESP_LOGI(TAG, "Emotion display initialized (80x160 landscape)");
    }

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
            .flags = {
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

        // 1. ServoController: low-level PCA9685 driver
        auto& servo = ServoController::GetInstance();
        if (!servo.Initialize(servo_i2c_bus_)) {
            ESP_LOGE(TAG, "ServoController initialization failed!");
            return;
        }

        // 2. GaitGenerator: biological gait engine (tripod, ripple, wave)
        auto& gait = GaitGenerator::GetInstance();
        if (!gait.Initialize(&servo)) {
            ESP_LOGE(TAG, "GaitGenerator initialization failed!");
            return;
        }

        // 3. AttackPatterns: strike and lunge animations
        auto& attack = AttackPatterns::GetInstance();
        if (!attack.Initialize(&gait, &servo)) {
            ESP_LOGE(TAG, "AttackPatterns initialization failed!");
            return;
        }

        // Start 20Hz task to drive gait and attack animation updates
        BaseType_t ok = xTaskCreatePinnedToCore(
            [](void* arg) {
                TickType_t last_wake = xTaskGetTickCount();
                const TickType_t interval = pdMS_TO_TICKS(50); // 20Hz

                while (true) {
                    GaitGenerator::GetInstance().Update();
                    AttackPatterns::GetInstance().UpdateFrame();
                    vTaskDelayUntil(&last_wake, interval);
                }
            },
            "motion_update", 4096, this, 15, &motion_task_handle_, 0);

        if (ok != pdPASS) {
            ESP_LOGE(TAG, "Failed to create motion update task!");
            return;
        }

        ESP_LOGI(TAG, "Motion layer ready: ServoController + GaitGenerator + AttackPatterns @ 20Hz");
    }

    // ========================================================================
    // Camera Initialization (OV5640 via DVP)
    // ========================================================================
    void InitializeCamera() {
        ESP_LOGI(TAG, "Initializing Camera (OV5640)...");

        esp_cam_ctlr_dvp_pin_config_t dvp_pin_config = {
            .data_width = CAMERA_D0_PIN,
            .data_io = {
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
            .xclk_io = GPIO_NUM_NC,
        };

        esp_video_init_sccb_config_t sccb_config = {
            .init_sccb = true,
            .i2c_config = {
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
            .xclk_freq = 24000000, // 24MHz for OV5640
        };

        esp_video_init_config_t video_config = {
            .dvp = &dvp_config,
        };

        camera_ = new Esp32Camera(video_config);
        if (camera_) {
            ESP_LOGI(TAG, "Camera object created successfully");
        } else {
            ESP_LOGE(TAG, "Failed to create camera object");
        }
    }

    // ========================================================================
    // Button Initialization
    // ========================================================================
    void InitializeButtons() {
        // Boot button: toggle chat/command mode
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });

        // Volume up
        volume_up_button_.OnClick([this]() {
            auto codec = GetAudioCodec();
            auto volume = codec->output_volume() + 10;
            if (volume > 100) {
                volume = 100;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(std::string("Volume: ") + std::to_string(volume));
        });

        volume_up_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(100);
            GetDisplay()->ShowNotification("Max volume");
        });

        // Volume down
        volume_down_button_.OnClick([this]() {
            auto codec = GetAudioCodec();
            auto volume = codec->output_volume() - 10;
            if (volume < 0) {
                volume = 0;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(std::string("Volume: ") + std::to_string(volume));
        });

        volume_down_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(0);
            GetDisplay()->ShowNotification("Muted");
        });

        ESP_LOGI(TAG, "Buttons initialized");
    }

public:
    HexapodDualBoard() :
        boot_button_(BOOT_BUTTON_GPIO),
        volume_up_button_(VOLUME_UP_BUTTON_GPIO),
        volume_down_button_(VOLUME_DOWN_BUTTON_GPIO) {

        ESP_LOGI(TAG, "Initializing Hexapod Dual Board...");

        // Initialize all subsystems
        InitializeMainTftDisplay();
        InitializeEmotionDisplay();
        InitializeServoI2c();
        InitializeMotionLayer();
        InitializeCamera();
        InitializeButtons();

        // Restore backlight brightness
        GetBacklight()->RestoreBrightness();

        ESP_LOGI(TAG, "Hexapod Dual Board initialized successfully!");
        ESP_LOGI(TAG, "  - Main display: 240x240 TFT ST7789");
        ESP_LOGI(TAG, "  - Emotion display: 80x160 ST7735");
        ESP_LOGI(TAG, "  - Servos: 18 (2x PCA9685)");
        ESP_LOGI(TAG, "  - Camera: OV5640");
        ESP_LOGI(TAG, "  - Audio: I2S Simplex (INMP441 + MAX98357A)");
    }

    virtual std::string GetBoardType() override {
        return "hexapod_dual";
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK,
            AUDIO_I2S_SPK_GPIO_LRCK,
            AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK,
            AUDIO_I2S_MIC_GPIO_WS,
            AUDIO_I2S_MIC_GPIO_DIN
        );
#else
        static NoAudioCodecDuplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK,
            AUDIO_I2S_SPK_GPIO_LRCK,
            AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_DIN
        );
#endif
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return main_display_;
    }

    virtual Display* GetEmotionDisplay() override {
        return emotion_display_;
    }

    virtual Camera* GetCamera() override {
        return camera_;
    }

    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(TFT_BL_PIN, false);
        return &backlight;
    }

    virtual void SetPowerSaveMode(bool enabled) override {
        // TODO: implement power save mode
        ESP_LOGI(TAG, "Power save mode %s", enabled ? "enabled" : "disabled");
    }

    virtual std::string GetBoardJson() override {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "board", "hexapod_dual");
        cJSON_AddStringToObject(root, "description", "Combined Hexapod VoiceBot + Bot");
        cJSON_AddNumberToObject(root, "servo_count", HexapodConst::NUM_SERVOS);
        cJSON_AddNumberToObject(root, "pca9685_count", HexapodConst::PCA9685_COUNT);
        cJSON_AddStringToObject(root, "main_display", "ST7789 240x240");
        cJSON_AddStringToObject(root, "emotion_display", "ST7735 80x160");
        cJSON_AddStringToObject(root, "camera", "OV5640 5MP");

        char* json_str = cJSON_PrintUnformatted(root);
        std::string result(json_str);
        cJSON_free(json_str);
        cJSON_Delete(root);
        return result;
    }

    virtual std::string GetDeviceStatusJson() override {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "board", "hexapod_dual");
        cJSON_AddBoolToObject(root, "servo_i2c_ready", servo_i2c_bus_ != nullptr);
        cJSON_AddBoolToObject(root, "motion_active", GaitGenerator::GetInstance().IsWalking());
        cJSON_AddBoolToObject(root, "camera_ready", camera_ != nullptr);
        cJSON_AddBoolToObject(root, "main_display_ready", main_display_ != nullptr);
        cJSON_AddBoolToObject(root, "emotion_display_ready", emotion_display_ != nullptr);

        char* json_str = cJSON_PrintUnformatted(root);
        std::string result(json_str);
        cJSON_free(json_str);
        cJSON_Delete(root);
        return result;
    }
};

DECLARE_BOARD(HexapodDualBoard);