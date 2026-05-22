# Hướng dẫn Build và Flash Firmware

## Sửa lỗi "load failed" trên WiFi Configuration Page

### Các bước thực hiện:

1. **Mở Terminal PowerShell** (Run as Administrator nếu cần)

2. **Đi vào thư mục dự án:**
   ```powershell
   cd "d:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new"
   ```

3. **Setup ESP-IDF environment:**
   ```powershell
   & "D:\Espressif\frameworks\esp-idf-v5.5.4\export.bat"
   ```

4. **Build và Flash firmware:**
   ```powershell
   idf.py -p COM10 -b 921600 build flash
   ```

5. **Sau khi flash xong, Restart thiết bị** và truy cập http://192.168.4.1 để kiểm tra

## Mô tả lỗi đã sửa

**Lỗi:** Symbol name mismatch trong `wifi_configuration_ap.cc`

**Vị trí:** Dòng 24-25

**Nguyên nhân:** ESP-IDF sử dụng path của file để tạo symbol name:
- File: `assets/wifi_configuration.html` → Symbol: `_binary_assets_wifi_configuration_html_start`
- File: `assets/wifi_configuration_done.html` → Symbol: `_binary_assets_wifi_configuration_done_html_start`

**Đã sửa:** Đổi từ `_binary_wifi_configuration_html_start` → `_binary_assets_wifi_configuration_html_start`

## Lưu ý

- Nếu bị lỗi checksum mismatch sau khi build, hãy ensure bạn đã flash lại firmware
- Port COM10 có thể thay đổi tùy thuộc vào thiết bị của bạn
- Baud rate 921600 là cho ESP32-S3