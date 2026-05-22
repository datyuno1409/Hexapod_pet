@echo off
setlocal

if "%~1"=="" (
    echo ========================================================
    echo Vui long nhap ten board can chuyen doi!
    echo Cach dung:
    echo   .\select_board.bat voicebot    [Cho VoiceBot - COM4]
    echo   .\select_board.bat hexapod     [Cho Hexapod Bot - COM10]
    echo ========================================================
    exit /b 1
)

set "TARGET_BOARD=%~1"
set "CONFIG_FILE=sdkconfig.defaults.esp32s3"

if /i "%TARGET_BOARD%"=="voicebot" (
    echo [SelectBoard] Chuyen doi sang BOARD_TYPE_HEXAPOD_VOICEBOT...
    set "BOARD_CFG=CONFIG_BOARD_TYPE_HEXAPOD_VOICEBOT=y"
) else if /i "%TARGET_BOARD%"=="hexapod" (
    echo [SelectBoard] Chuyen doi sang BOARD_TYPE_HEXAPOD_BOT...
    set "BOARD_CFG=CONFIG_BOARD_TYPE_HEXAPOD_BOT=y"
) else (
    echo ========================================================
    echo Ten board khong hop le: %TARGET_BOARD%
    echo Vui long chon 'voicebot' hoac 'hexapod'.
    echo ========================================================
    exit /b 1
)

(
echo CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
echo CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
echo.
echo CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y
echo.
echo CONFIG_SPIRAM=y
echo CONFIG_SPIRAM_MODE_OCT=y
echo CONFIG_SPIRAM_SPEED_80M=y
echo CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=512
echo CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=65536
echo CONFIG_SPIRAM_MEMTEST=n
echo CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=y
echo.
echo CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=3
echo CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=6
echo CONFIG_ESP_WIFI_RX_BA_WIN=3
echo CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=16
echo CONFIG_MBEDTLS_DYNAMIC_FREE_CONFIG_DATA=y
echo.
echo CONFIG_ESP32S3_INSTRUCTION_CACHE_32KB=y
echo CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y
echo.
echo CONFIG_SR_WN_WN9_NIHAOXIAOZHI_TTS=y
echo.
echo CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=4096
echo.
echo # LVGL Graphics
echo CONFIG_LV_USE_SNAPSHOT=y
echo.
echo # LVGL Enable built-in fonts
echo CONFIG_LV_FONT_MONTSERRAT_20=y
echo CONFIG_LV_FONT_MONTSERRAT_22=y
echo CONFIG_LV_FONT_MONTSERRAT_28=y
echo CONFIG_LV_FONT_MONTSERRAT_48=y
echo.
echo # FAT Filesystem support
echo CONFIG_FATFS_LFN_HEAP=y
echo CONFIG_FATFS_API_ENCODING_UTF_8=y
echo CONFIG_FATFS_FS_LOCK=4
echo.
echo # Selected Board Type
echo %BOARD_CFG%
) > "%CONFIG_FILE%"

:: Xoa folder build va file sdkconfig cu de ESP-IDF bat buoc phai generate lai tu file defaults (giu lai managed_components de build nhanh)
if exist "build" (
    echo [SelectBoard] Xoa thu muc build cu...
    rmdir /s /q "build"
)
if exist "sdkconfig" (
    echo [SelectBoard] Xoa file sdkconfig cu de cap nhat cau hinh moi...
    del /f /q "sdkconfig"
)
if exist "sdkconfig.old" del /f /q "sdkconfig.old"

echo ========================================================
echo CHUYEN DOI BOARD THANH CONG: %BOARD_CFG%
echo Ban co hoan toan co the chay lenh idf.py build hoac flash.
echo ========================================================
exit /b 0
