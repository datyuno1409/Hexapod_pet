# HƯỚNG DẪN SỬ DỤNG & TỔNG HỢP THAY ĐỔI FIRMWARE

Tài liệu này tổng hợp toàn bộ các thay đổi kỹ thuật đã thực hiện trên firmware của **VoiceBot (COM4)** và **Hexapod Bot (COM10)**, đồng thời hướng dẫn cách vận hành, cấu hình, biên dịch và nạp chương trình.

---

## 1. Các thay đổi kỹ thuật đã thực hiện (Changelog)

### A. Khôi phục tính năng gọi nhạc gốc `%self.music.play_song` trên VoiceBot (COM4)
*   **Vấn đề cũ:** Trước đây, công cụ gọi nhạc `self.music.play_song` bị vô hiệu hóa (comment out) để ép hệ thống phát trực tiếp qua WebSocket stream. Tuy nhiên, điều này khiến server LLM bị sai lệch luồng xử lý và gọi nhầm sang `%search.music` hoặc các lệnh tìm kiếm không hợp lệ khác.
*   **Giải pháp mới:** Đã **mở lại (uncomment)** `McpFeatureTools::RegisterMusicTools(music_)` trong tệp [application.cc](file:///D:/Robot/Hexapod_pet/xiaozhi-esp32_vietnam_new/main/application.cc).
*   **Kết quả:** Thiết bị đăng ký thành công công cụ gọi nhạc với server. Server LLM giờ đây sẽ gọi đúng lệnh gốc `%self.music.play_song` để phát nhạc trên VoiceBot qua COM4.

### B. Sửa lỗi Crash (Khởi động lại liên tục / Guru Meditation Error) trên Hexapod Bot (COM10)
*   **Vấn đề cũ:** Phần cứng Hexapod Bot (COM10) không tích hợp chip giải mã âm thanh (Audio Codec / DAC). Khi mã nguồn chạy đến các hàm khởi tạo hoặc tương tác âm thanh, con trỏ `codec_` bị rỗng (`nullptr`), gây lỗi truy cập bộ nhớ (`LoadProhibited`) dẫn đến crash mạch và reboot liên tục.
*   **Giải pháp:** Bổ sung các điều kiện kiểm tra an toàn (`if (codec_ != nullptr)`) trong toàn bộ logic khởi tạo và xử lý âm thanh ở lớp `AudioService`.
*   **Kết quả:** Hexapod Bot (COM10) hoạt động cực kỳ ổn định, không còn bị lỗi khởi động lại, màn hình LCD hiển thị bình thường.

### C. Đồng nhất môi trường và Sửa lỗi Build Target
*   **Vấn đề cũ:** Do tệp cấu hình cũ hoặc khi xóa thư mục `build`, công cụ biên dịch tự động nhận diện target mặc định là `esp32` (thay vì `esp32s3`), dẫn đến lỗi thư viện cảm biến camera và cấu hình RAM.
*   **Giải pháp:** Hướng dẫn và cấu hình quy trình biên dịch bắt buộc chạy lệnh `idf.py set-target esp32s3` ngay sau khi chuyển đổi board bằng `select_board.bat`.

---

## 2. Các tính năng hiện tại của 2 Robot

### 🎤 VoiceBot (COM4) - Thiết bị Tương tác Giọng nói & Phát nhạc
*   **Tương tác đàm thoại:** Thu âm qua Micro, truyền nhận giọng nói qua giao thức WebSocket thời gian thực cực kỳ mượt mà.
*   **Gọi nhạc thông minh:** Khi bạn yêu cầu phát nhạc, server sẽ gọi công cụ `%self.music.play_song` và thiết bị sẽ tự động tải / phát âm thanh từ máy chủ nhạc.
*   **Màn hình hiển thị:** Hiển thị giao diện trạng thái kết nối, biểu cảm hoạt họa khi nói chuyện hoặc nghe.

### 🕷️ Hexapod Bot (COM10) - Thiết bị Điều khiển Chuyển động
*   **Điều khiển động cơ:** Đóng vai trò là bộ não điều khiển chuyển động của Robot 6 chân thông qua giao tiếp I2C với chip điều khiển Servo PCA9685.
*   **Ổn định tuyệt đối:** Tắt toàn bộ luồng xử lý âm thanh để tránh xung đột phần cứng, tập trung tài nguyên cho việc xử lý cử động và camera.
*   **Giao diện LCD:** Hiển thị biểu cảm động, trạng thái kết nối WiFi và trạng thái hoạt động của các khớp servo.

---

## 3. Hướng dẫn Biên dịch và Nạp chương trình (Compile & Flash)

> [!IMPORTANT]
> Do dự án sử dụng chung một mã nguồn cho 2 thiết bị có phần cứng khác nhau, bạn **bắt buộc** phải chuyển đổi cấu hình (Board Type) trước khi tiến hành build và nạp.

### Bước chuẩn bị môi trường:
Mở terminal PowerShell (hoặc CMD), di chuyển vào thư mục dự án và kích hoạt môi trường ESP-IDF bằng lệnh:
```powershell
# Chuyển vào thư mục chứa code
cd D:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new

# Kích hoạt môi trường ESP-IDF (nếu chưa cấu hình tự động)
D:\Espressif\frameworks\esp-idf-v5.5.4\export.bat
```

---

### 🛠️ Quy trình cho VoiceBot (Cổng COM4):
1.  **Chuyển đổi cấu hình sang VoiceBot:**
    ```powershell
    .\select_board.bat voicebot
    ```
2.  **Đặt Target cho ESP32-S3:**
    ```powershell
    idf.py set-target esp32s3
    ```
3.  **Biên dịch và Nạp xuống mạch (COM4):**
    ```powershell
    idf.py -p COM4 build flash
    ```

---

### 🛠️ Quy trình cho Hexapod Bot (Cổng COM10):
1.  **Chuyển đổi cấu hình sang Hexapod Bot:**
    ```powershell
    .\select_board.bat hexapod
    ```
2.  **Đặt Target cho ESP32-S3:**
    ```powershell
    idf.py set-target esp32s3
    ```
3.  **Biên dịch và Nạp xuống mạch (COM10):**
    ```powershell
    idf.py -p COM10 build flash
    ```

---

## 4. Hướng dẫn Cấu hình WiFi và Âm lượng (Web Portal)

Cả hai thiết bị đều hỗ trợ giao diện cấu hình qua trình duyệt web (Web Portal) để bạn có thể đổi mạng WiFi hoặc chỉnh âm lượng mà không cần nạp lại code.

1.  **Kích hoạt chế độ Cấu hình (AP Mode):**
    *   Nhấn giữ nút **BOOT** trên mạch khoảng 3-5 giây.
    *   Màn hình thiết bị sẽ hiển thị thông báo chuyển sang chế độ cấu hình và hiển thị tên WiFi Access Point (AP).
2.  **Kết nối thiết bị:**
    *   Sử dụng điện thoại hoặc máy tính kết nối vào mạng WiFi do robot phát ra (thường bắt đầu bằng tên: `TienHuyIoT-XXXX`).
3.  **Truy cập trang cấu hình:**
    *   Mở trình duyệt web và truy cập địa chỉ IP mặc định: `http://192.168.4.1`
4.  **Cài đặt:**
    *   Chọn mạng WiFi nhà bạn, nhập mật khẩu.
    *   Đối với VoiceBot, bạn có thể kéo thanh trượt để tăng/giảm âm lượng loa hoặc cấu hình đường dẫn máy chủ âm nhạc (Music Server URL).

---

## 5. Các vấn đề cần khắc phục và Lưu ý tiếp theo

### A. Kiểm tra Kết nối Loa & Phần cứng âm thanh trên COM4
*   Nếu màn hình VoiceBot (COM4) đã sáng và tương tác tốt nhưng không phát ra âm thanh khi gọi nhạc, bạn hãy kiểm tra lại kết nối dây của module DAC (I2S) và Loa.
*   Cấu hình máy chủ âm nhạc mặc định là `http://www.xiaozhishop.xyz:5005`. Bạn có thể thay đổi địa chỉ này trong trang cấu hình Web Portal nếu server nhạc của bạn chạy trên một máy chủ riêng biệt.

### B. Kết nối UART giữa hai mạch
*   Để Hexapod Bot (COM10) có thể di chuyển dựa theo các câu lệnh giọng nói nhận được từ VoiceBot (COM4), hãy chắc chắn rằng bạn đã kết nối các chân TX/RX giữa 2 mạch một cách chính xác (TX mạch này nối với RX mạch kia).
*   Giao tiếp UART này sẽ truyền tải các lệnh chuyển động dạng chuỗi JSON hoặc mã lệnh nhị phân từ VoiceBot sang Hexapod Bot để thực hiện các hành động tương ứng.
