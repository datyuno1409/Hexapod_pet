#ifndef _HEXAPOD_VOICEBOT_CONFIG_H_
#define _HEXAPOD_VOICEBOT_CONFIG_H_

#include <driver/gpio.h>
#include <driver/uart.h>

// ============================================================================
// AUDIO CONFIGURATION (I2S for INMP441 microphone + MAX98357A amplifier)
// ============================================================================
#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

// Using Simplex I2S mode (separate TX/RX)
#define AUDIO_I2S_METHOD_SIMPLEX

// Microphone (INMP441 input)
#define AUDIO_I2S_MIC_GPIO_WS   ((gpio_num_t)4)
#define AUDIO_I2S_MIC_GPIO_SCK  ((gpio_num_t)5)
#define AUDIO_I2S_MIC_GPIO_DIN  ((gpio_num_t)6)

// Speaker (MAX98357A output)
#define AUDIO_I2S_SPK_GPIO_DOUT ((gpio_num_t)7)
#define AUDIO_I2S_SPK_GPIO_BCLK ((gpio_num_t)15)
#define AUDIO_I2S_SPK_GPIO_LRCK ((gpio_num_t)16)

// ============================================================================
// TFT ST7789 DISPLAY (SPI 240x240)
// ============================================================================
// SPI bus pins
#define TFT_SPI_MOSI ((gpio_num_t)11)
#define TFT_SPI_MISO ((gpio_num_t)13)
#define TFT_SPI_CLK  ((gpio_num_t)12)

// ST7789 control pins
#define TFT_SPI_CS   ((gpio_num_t)9)
#define TFT_DC_PIN   ((gpio_num_t)8)
#define TFT_RST_PIN  ((gpio_num_t)18)  // TFT Reset pin
#define TFT_BL_PIN   ((gpio_num_t)46)  // Backlight control

// Display dimensions
#define TFT_WIDTH    240
#define TFT_HEIGHT   240

// SPI frequency for TFT
#define TFT_SPI_FREQ_HZ (40 * 1000 * 1000)  // 40MHz

// ============================================================================
// BUTTONS
// ============================================================================
#define BOOT_BUTTON_GPIO        ((gpio_num_t)0)   // Wake-up button (boot button)
#define VOLUME_UP_BUTTON_GPIO   ((gpio_num_t)3)   // Volume up
#define VOLUME_DOWN_BUTTON_GPIO ((gpio_num_t)17)  // Volume down

// ============================================================================
// STATUS LED
// ============================================================================
#define BUILTIN_LED_GPIO        ((gpio_num_t)48)

// ============================================================================
// CAMERA - NOT AVAILABLE on VoiceBot (Camera is on Hexapod Bot via UART)
// ============================================================================
// Camera features are handled by the separate Hexapod Bot board (COM10)
// VoiceBot communicates with Bot via UART bridge for camera requests
#define CAMERA_NOT_AVAILABLE

// ============================================================================
// HEXAPOD COMMUNICATION (WebSocket client)
// ============================================================================
#define HEXAPOD_SERVER_IP       "192.168.1.100"  // Default Hexapod IP (configurable)
#define HEXAPOD_SERVER_PORT     8081             // WebSocket port

// ============================================================================
// UART LINK BETWEEN VOICEBOT AND HEXAPOD CONTROLLER
// ============================================================================
#define HEXAPOD_UART_PORT       UART_NUM_1
#define HEXAPOD_UART_BAUD_RATE  921600
#define HEXAPOD_UART_TX_PIN     ((gpio_num_t)43)
#define HEXAPOD_UART_RX_PIN     ((gpio_num_t)44)
#define HEXAPOD_UART_RX_BUF_SIZE 4096
#define HEXAPOD_UART_TX_BUF_SIZE 4096

#endif  // _HEXAPOD_VOICEBOT_CONFIG_H_
