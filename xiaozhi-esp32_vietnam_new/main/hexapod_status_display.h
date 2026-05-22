#ifndef HEXAPOD_STATUS_DISPLAY_H
#define HEXAPOD_STATUS_DISPLAY_H

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <cstdint>
#include <string>

class HexapodStatusDisplay {
public:
    HexapodStatusDisplay(esp_lcd_panel_io_handle_t panel_io,
                         esp_lcd_panel_handle_t panel,
                         int width, int height);
    ~HexapodStatusDisplay();

    void Clear();
    void Update(float battery_pct, float fps, float temp_c,
                int latency_ms, int signal_strength, const std::string& motion_status);
    void ShowText(const std::string& text, int x, int y);

private:
    esp_lcd_panel_io_handle_t panel_io_;
    esp_lcd_panel_handle_t panel_;
    int width_;
    int height_;
    uint16_t* frame_buffer_;

    void DrawRectangle(int x, int y, int w, int h, uint16_t color);
    void DrawFilledRectangle(int x, int y, int w, int h, uint16_t color);
    void DrawLine(int x0, int y0, int x1, int y1, uint16_t color);
    void Render();

    // Colors (RGB565)
    static constexpr uint16_t COLOR_BLACK = 0x0000;
    static constexpr uint16_t COLOR_WHITE = 0xFFFF;
    static constexpr uint16_t COLOR_GREEN = 0x07E0;
    static constexpr uint16_t COLOR_CYAN = 0x07FF;
    static constexpr uint16_t COLOR_RED = 0xF800;
    static constexpr uint16_t COLOR_YELLOW = 0xFFE0;
};

#endif  // HEXAPOD_STATUS_DISPLAY_H
