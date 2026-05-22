#include "hexapod_status_display.h"
#include <esp_log.h>
#include <cmath>
#include <cstring>

#define TAG "HexapodStatusDisplay"

HexapodStatusDisplay::HexapodStatusDisplay(esp_lcd_panel_io_handle_t panel_io,
                                           esp_lcd_panel_handle_t panel,
                                           int width, int height)
    : panel_io_(panel_io), panel_(panel), width_(width), height_(height) {
    frame_buffer_ = new uint16_t[width * height];
    memset(frame_buffer_, 0, width * height * sizeof(uint16_t));
    Clear();
}

HexapodStatusDisplay::~HexapodStatusDisplay() {
    if (frame_buffer_) {
        delete[] frame_buffer_;
    }
}

void HexapodStatusDisplay::Clear() {
    memset(frame_buffer_, 0, width_ * height_ * sizeof(uint16_t));
    Render();
}

void HexapodStatusDisplay::DrawFilledRectangle(int x, int y, int w, int h, uint16_t color) {
    for (int row = y; row < y + h && row < height_; row++) {
        for (int col = x; col < x + w && col < width_; col++) {
            if (col >= 0 && row >= 0) {
                frame_buffer_[row * width_ + col] = color;
            }
        }
    }
}

void HexapodStatusDisplay::DrawRectangle(int x, int y, int w, int h, uint16_t color) {
    // Top and bottom lines
    for (int col = x; col < x + w && col < width_; col++) {
        if (y >= 0 && y < height_) {
            frame_buffer_[y * width_ + col] = color;
        }
        if (y + h - 1 >= 0 && y + h - 1 < height_) {
            frame_buffer_[(y + h - 1) * width_ + col] = color;
        }
    }
    // Left and right lines
    for (int row = y; row < y + h && row < height_; row++) {
        if (x >= 0 && x < width_) {
            frame_buffer_[row * width_ + x] = color;
        }
        if (x + w - 1 >= 0 && x + w - 1 < width_) {
            frame_buffer_[row * width_ + (x + w - 1)] = color;
        }
    }
}

void HexapodStatusDisplay::DrawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    int x = x0, y = y0;
    while (true) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            frame_buffer_[y * width_ + x] = color;
        }
        if (x == x1 && y == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void HexapodStatusDisplay::Update(float battery_pct, float fps, float temp_c,
                                   int latency_ms, int signal_strength,
                                   const std::string& motion_status) {
    Clear();

    // Left panel: Status info (Portrait)
    // Hexapod info box
    DrawRectangle(2, 2, 35, 70, COLOR_CYAN);
    DrawFilledRectangle(4, 4, 31, 10, COLOR_CYAN);

    // Right panel: Real-time data (Landscape orientation hint)
    // Battery bar
    int bat_bar_width = 30;
    int bat_filled = (battery_pct / 100.0f) * bat_bar_width;
    DrawRectangle(42, 8, bat_bar_width, 8, COLOR_WHITE);
    if (bat_filled > 0) {
        uint16_t bat_color = (battery_pct > 30) ? COLOR_GREEN : COLOR_RED;
        DrawFilledRectangle(42, 8, bat_filled, 8, bat_color);
    }

    // FPS indicator
    int fps_bar_height = (fps / 30.0f) * 20; // Max 30 FPS
    fps_bar_height = std::min(fps_bar_height, 20);
    DrawRectangle(42, 25, 8, 20, COLOR_WHITE);
    DrawFilledRectangle(42, 25 + 20 - fps_bar_height, 8, fps_bar_height, COLOR_YELLOW);

    // Signal strength indicator (simple bars)
    int signal_bars = (signal_strength / 100) * 5;
    signal_bars = std::min(signal_bars, 5);
    for (int i = 0; i < signal_bars; i++) {
        DrawFilledRectangle(52 + i * 5, 25 + (4 - i) * 3, 3, (i + 1) * 3, COLOR_GREEN);
    }

    Render();
}

void HexapodStatusDisplay::ShowText(const std::string& text, int x, int y) {
    // Placeholder: Simple text display (would need font rendering)
    // For now, just log it
    ESP_LOGI(TAG, "Text at (%d,%d): %s", x, y, text.c_str());
}

void HexapodStatusDisplay::Render() {
    // Write frame buffer to LCD panel
    esp_lcd_panel_draw_bitmap(panel_, 0, 0, width_, height_, (const uint8_t*)frame_buffer_);
}
