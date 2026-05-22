# Sửa lỗi Build - ESP32-S3 Target

## Vấn đề đã fix
1. **Lỗi `hexapod_uart_bridge.cc` dòng 50**: Thêm `.c_str()` để convert `std::string` sang `const char*`
2. **Lỗi target ESP32 thay vì ESP32S3**: Script `select_board.bat` đã xử lý đúng
3. **Audio codec VoiceBot**: Sửa từ 10 tham số sang 8 tham số cho `NoAudioCodecSimplex`

## Script có sẵn
Script `select_board.bat` tự động:
- Cập nhật board type trong `sdkconfig.defaults.esp32s3`
- Xóa thư mục `build` cũ
- Xóa file `sdkconfig` cũ

## Build cho VoiceBot (COM4)

```cmd
cd D:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new

# Chọn board VoiceBot (tự động xóa build cũ)
.\select_board.bat voicebot

# Build
idf.py build

# Flash và monitor
idf.py -p COM4 flash monitor
```

## Build cho Hexapod Bot (COM10)

```cmd
cd D:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new

# Chọn board Hexapod Bot (tự động xóa build cũ)
.\select_board.bat hexapod

# Build
idf.py build

# Flash và monitor
idf.py -p COM10 flash monitor
```

## Quick commands

| Mục đích | Lệnh |
|----------|------|
| Build VoiceBot | `.\select_board.bat voicebot && idf.py build` |
| Build Hexapod Bot | `.\select_board.bat hexapod && idf.py build` |
| Flash VoiceBot | `idf.py -p COM4 flash` |
| Flash Hexapod Bot | `idf.py -p COM10 flash` |
| Monitor VoiceBot | `idf.py -p COM4 monitor` |
| Monitor Hexapod Bot | `idf.py -p COM10 monitor` |

## Build thành công sẽ hiển thị

### VoiceBot:
```
I (XXXX) HexapodVoicebotBoard: UART bridge initialized as MASTER for Hexapod Bot communication
I (XXXX) HexapodVoicebotBoard: Hexapod VoiceBot board initialized
```

### Hexapod Bot:
```
I (XXXX) HexapodBotBoard: UART bridge initialized as SLAVE for VoiceBot communication
I (XXXX) HexapodBotBoard: Hexapod Bot board initialized successfully
```

## Nếu có lỗi build

### Xóa toàn bộ và build lại:
```cmd
cd D:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new

.\select_board.bat voicebot    # Hoặc hexapod
idf.py fullclean
idf.py set-target esp32s3
idf.py build
```

## Lưu ý
- Luôn chạy `select_board.bat` trước mỗi lần build để đảm bảo board type đúng
- Script tự động xóa build cũ nên không cần chạy `idf.py fullclean` thủ công
- Sau khi switch board, phải chạy `idf.py build` trước khi `flash`