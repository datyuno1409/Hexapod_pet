#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "led/single_led.h"
#include "assets/lang_config.h"
#include "backlight.h"
#include "hexapod_uart_bridge.h"
#include "hexapod_servo_controller.h"
#include "hexapod_gait_generator.h"
#include "hexapod_attack_patterns.h"
#include "hexapod_protocol.h"
#include <wifi_station.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_st7789.h>

#define TAG "HexapodVoicebotBoard"

class HexapodVoicebotBoard : public WifiBoard {
private:
    // TFT Display
    esp_lcd_panel_io_handle_t tft_panel_io_ = nullptr;
    esp_lcd_panel_handle_t tft_panel_ = nullptr;
    Display* display_ = nullptr;

    // Buttons
    Button boot_button_;
    Button volume_up_button_;
    Button volume_down_button_;

    // Servo I2C bus and Motion Task
    i2c_master_bus_handle_t servo_i2c_bus_ = nullptr;
    TaskHandle_t motion_task_handle_ = nullptr;

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
            "motion_update",
            4096,
            nullptr,
            15,
            &motion_task_handle_,
            0
        );

        if (ok != pdPASS) {
            ESP_LOGE(TAG, "Failed to create motion update task!");
            return;
        }

        ESP_LOGI(TAG, "Motion layer ready: ServoController + GaitGenerator + AttackPatterns @ 20Hz (task priority 15, core 0)");
    }

    // ========================================================================
    // TFT Display Initialization (ST7789 via SPI)
    // ========================================================================
    void InitializeTftDisplay() {
        ESP_LOGI(TAG, "Initializing TFT ST7789 display via SPI...");

        // Initialize SPI bus
        spi_bus_config_t bus_config = {
            .mosi_io_num = TFT_SPI_MOSI,
            .miso_io_num = TFT_SPI_MISO,
            .sclk_io_num = TFT_SPI_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = TFT_WIDTH * TFT_HEIGHT * 2,  // 240*240*2 bytes
        };
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO));
        ESP_LOGI(TAG, "SPI bus initialized");

        // Initialize LCD I/O directly on SPI bus (no separate spi_device needed)
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
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_config, &tft_panel_io_));
        ESP_LOGI(TAG, "LCD I/O configured");

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
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(tft_panel_io_, &panel_config, &tft_panel_));
        ESP_LOGI(TAG, "ST7789 panel initialized");

        // Reset and initialize display
        ESP_ERROR_CHECK(esp_lcd_panel_reset(tft_panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_init(tft_panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(tft_panel_, true));  // ST7789 needs color inversion
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(tft_panel_, true));

        // Create SPI LCD display abstraction (ST7789 240x240)
        display_ = new SpiLcdDisplay(tft_panel_io_, tft_panel_,
                                     TFT_WIDTH, TFT_HEIGHT,
                                     0, 0, false, false, false);
        ESP_LOGI(TAG, "TFT display initialized successfully");
    }

    void InitializeCamera() {
        // VoiceBot doesn't have a camera by default
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

        boot_button_.OnLongPress([this]() {
            ESP_LOGI(TAG, "BOOT button long press: Spawning diagnostic motion task...");
            xTaskCreate([](void* arg) {
                ESP_LOGI(TAG, "Diagnostic Motion Task started");
                HexapodProtocol::GetInstance().SendCommand("{\"cmd\":\"motion\",\"action\":\"stand\"}");
                vTaskDelay(pdMS_TO_TICKS(2000));
                HexapodProtocol::GetInstance().SendCommand("{\"cmd\":\"motion\",\"action\":\"dance\",\"speed\":60,\"duration_ms\":3000}");
                vTaskDelay(pdMS_TO_TICKS(4000));
                HexapodProtocol::GetInstance().SendCommand("{\"cmd\":\"motion\",\"action\":\"sit\"}");
                ESP_LOGI(TAG, "Diagnostic Motion Task finished");
                vTaskDelete(nullptr);
            }, "diag_motion", 4096, nullptr, 5, nullptr);
        });

        // Volume up
        volume_up_button_.OnClick([this]() {
            auto codec = GetAudioCodec();
            auto volume = codec->output_volume() + 10;
            if (volume > 100) {
                volume = 100;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });

        volume_up_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(100);
            GetDisplay()->ShowNotification(Lang::Strings::MAX_VOLUME);
        });

        // Volume down
        volume_down_button_.OnClick([this]() {
            auto codec = GetAudioCodec();
            auto volume = codec->output_volume() - 10;
            if (volume < 0) {
                volume = 0;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });

        volume_down_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(0);
            GetDisplay()->ShowNotification(Lang::Strings::MUTED);
        });

        ESP_LOGI(TAG, "Buttons initialized");
    }

public:
    HexapodVoicebotBoard() :
        boot_button_(BOOT_BUTTON_GPIO, false, 1000),
        volume_up_button_(VOLUME_UP_BUTTON_GPIO, false, 1000),
        volume_down_button_(VOLUME_DOWN_BUTTON_GPIO, false, 1000) {
        InitializeTftDisplay();
        InitializeButtons();
        GetBacklight()->RestoreBrightness();
        InitializeServoI2c();
        InitializeMotionLayer();

        // Initialize UART bridge for communication with Hexapod Bot board
        // VoiceBot acts as MASTER, sending commands to Bot (SLAVE)
#ifdef HEXAPOD_UART_PORT
        if (!ServoController::GetInstance().IsInitialized()) {
            auto& uart_bridge = HexapodUartBridge::GetInstance();
            uart_bridge.Start(HexapodUartBridge::Role::kMaster);
            ESP_LOGI(TAG, "UART bridge initialized as MASTER for Hexapod Bot communication");
        } else {
            ESP_LOGI(TAG, "Local ServoController is initialized (diagnostic mode). Skipping UART bridge initialization to preserve Console UART (GPIO 43/44).");
        }
#endif

        // Set default safe volume to prevent MAX98357A distortion
        GetAudioCodec()->SetOutputVolume(60);

        ESP_LOGI(TAG, "Hexapod VoiceBot board initialized");
    }

    virtual std::string GetBoardType() override { return "hexapod_voicebot"; }

    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(TFT_BL_PIN, false);
        return &backlight;
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        // Simplex I2S: separate TX (speaker) and RX (microphone)
        // Using 10-parameter constructor for INMP441 + MAX98357A
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK,
            AUDIO_I2S_SPK_GPIO_LRCK,
            AUDIO_I2S_SPK_GPIO_DOUT,
            I2S_STD_SLOT_BOTH,
            AUDIO_I2S_MIC_GPIO_SCK,
            AUDIO_I2S_MIC_GPIO_WS,
            AUDIO_I2S_MIC_GPIO_DIN,
            I2S_STD_SLOT_LEFT
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
        return display_;
    }

    virtual Camera* GetCamera() override {
        return nullptr;
    }
};

DECLARE_BOARD(HexapodVoicebotBoard);
