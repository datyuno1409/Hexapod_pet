#ifndef _HEXAPOD_DUAL_CONFIG_H_
#define _HEXAPOD_DUAL_CONFIG_H_

#include <driver/gpio.h>
#include <driver/uart.h>

// ============================================================================
// AUDIO CONFIGURATION (I2S for INMP441 microphone + MAX98357A amplifier)
// ============================================================================
#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

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
// MAIN TFT ST7789 DISPLAY (SPI 240x240 - VoiceBot display)
// ============================================================================
// SPI bus pins
#define TFT_SPI_MOSI ((gpio_num_t)11)
#define TFT_SPI_MISO ((gpio_num_t)13)
#define TFT_SPI_CLK  ((gpio_num_t)12)

// ST7789 control pins
#define TFT_SPI_CS   ((gpio_num_t)9)
#define TFT_DC_PIN   ((gpio_num_t)8)
#define TFT_RST_PIN  ((gpio_num_t)18)
#define TFT_BL_PIN   ((gpio_num_t)46)

// Display dimensions
#define TFT_WIDTH    240
#define TFT_HEIGHT   240

// SPI frequency for TFT
#define TFT_SPI_FREQ_HZ (40 * 1000 * 1000)  // 40MHz

// ============================================================================
// EMOTION TFT ST7735 DISPLAY (SPI 80x160 - Bot emotion display)
// ============================================================================
#define TFT_SMALL_SPI_SCL       ((gpio_num_t)1)    // Clock (SCL)
#define TFT_SMALL_SPI_SDA       ((gpio_num_t)2)    // Serial Data (SDA / MOSI)
#define TFT_SMALL_SPI_RES       ((gpio_num_t)11)   // Reset (RES)
#define TFT_SMALL_SPI_DC        ((gpio_num_t)10)   // Data/Command (DC)
#define TFT_SMALL_SPI_CS        ((gpio_num_t)3)    // Chip Select (CS)
#define TFT_SMALL_SPI_BLK       ((gpio_num_t)14)   // Backlight (BLK)

#define TFT_SMALL_WIDTH         80
#define TFT_SMALL_HEIGHT        160
#define TFT_SMALL_SPI_FREQ_HZ   (10 * 1000 * 1000)  // 10MHz for stability

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
// CAMERA (OV5640 - DVP interface)
// ============================================================================
#define CAMERA_I2C_SDA          ((gpio_num_t)39)
#define CAMERA_I2C_SCL          ((gpio_num_t)40)
#define CAMERA_I2C_FREQ_HZ      (400 * 1000)  // 400 kHz
#define CAMERA_I2C_PORT         1

#define CAMERA_PWDN_PIN         ((gpio_num_t)47)   // Power down
#define CAMERA_RESET_PIN        ((gpio_num_t)21)   // Reset
#define CAMERA_VSYNC_PIN        ((gpio_num_t)34)   // VSYNC
#define CAMERA_HREF_PIN         ((gpio_num_t)35)   // HREF
#define CAMERA_PCLK_PIN         ((gpio_num_t)36)   // PCLK

// Camera D0-D7 data pins
#define CAMERA_D0_PIN           ((gpio_num_t)37)
#define CAMERA_D1_PIN           ((gpio_num_t)38)
#define CAMERA_D2_PIN           ((gpio_num_t)19)
#define CAMERA_D3_PIN           ((gpio_num_t)20)
#define CAMERA_D4_PIN           ((gpio_num_t)22)
#define CAMERA_D5_PIN           ((gpio_num_t)23)
#define CAMERA_D6_PIN           ((gpio_num_t)24)
#define CAMERA_D7_PIN           ((gpio_num_t)25)

// ============================================================================
// PCA9685 SERVO CONTROLLER (I2C for 18 servos)
// ============================================================================
#define SERVO_I2C_SDA           ((gpio_num_t)41)
#define SERVO_I2C_SCL           ((gpio_num_t)42)
#define SERVO_I2C_FREQ_HZ       (400 * 1000)  // 400 kHz
#define SERVO_I2C_PORT          0

// PCA9685 I2C addresses
// #define PCA9685_ADDR_1          0x40  // First PCA9685 (9 servos: 0-8)
// #define PCA9685_ADDR_2          0x41  // Second PCA9685 (9 servos: 9-17)

// PCA9685 frequency (50Hz for servo control)
#define PCA9685_FREQ_HZ         50    // 50 Hz for servo control

// Servo pulse range (in microseconds)
#define SERVO_MIN_PULSE_US      1000  // 1ms = 0 degrees
#define SERVO_MID_PULSE_US      1500  // 1.5ms = 90 degrees
#define SERVO_MAX_PULSE_US      2000  // 2ms = 180 degrees

// Servo count and layout
#define SERVO_COUNT             18
// #define SERVOS_PER_PCA9685      9
// #define PCA9685_COUNT           2

// Servo indices per leg (3 DOF per leg)
#define SERVO_FL_COXA           0     // Front Left Hip rotation
#define SERVO_FL_FEMUR          1     // Front Left Forward/backward
#define SERVO_FL_TIBIA          2     // Front Left Up/down

#define SERVO_FR_COXA           3     // Front Right
#define SERVO_FR_FEMUR          4
#define SERVO_FR_TIBIA          5

#define SERVO_ML_COXA           6     // Middle Left
#define SERVO_ML_FEMUR          7
#define SERVO_ML_TIBIA          8

#define SERVO_MR_COXA           9     // Middle Right
#define SERVO_MR_FEMUR          10
#define SERVO_MR_TIBIA          11

#define SERVO_BL_COXA           12    // Back Left
#define SERVO_BL_FEMUR          13
#define SERVO_BL_TIBIA          14

#define SERVO_BR_COXA           15    // Back Right
#define SERVO_BR_FEMUR          16
#define SERVO_BR_TIBIA          17

// ============================================================================
// HEXAPOD COMMUNICATION (Dual WebSocket: client + server)
// ============================================================================
#define HEXAPOD_SERVER_IP       "192.168.1.100"  // Default Hexapod IP (configurable)
#define HEXAPOD_SERVER_PORT     8081             // WebSocket port for client
#define HEXAPOD_WEBSOCKET_PORT  8081             // WebSocket server port
#define HEXAPOD_WEBSOCKET_STACK_SIZE  (8 * 1024)  // 8KB for WebSocket handler

// ============================================================================
// UART LINK (Internal communication or external device)
// ============================================================================
#define HEXAPOD_UART_PORT       UART_NUM_1
#define HEXAPOD_UART_BAUD_RATE  921600
#define HEXAPOD_UART_TX_PIN     ((gpio_num_t)43)
#define HEXAPOD_UART_RX_PIN     ((gpio_num_t)44)
#define HEXAPOD_UART_RX_BUF_SIZE 4096
#define HEXAPOD_UART_TX_BUF_SIZE 4096

// ============================================================================
// MOTION CONTROL
// ============================================================================
#define MOTION_DEFAULT_SPEED    50    // 0-100 speed scale
#define MOTION_DEFAULT_CYCLE_MS 500   // Cycle time for motion in milliseconds

#endif  // _HEXAPOD_DUAL_CONFIG_H_