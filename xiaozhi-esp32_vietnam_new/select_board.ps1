param (
    [string]$TargetBoard
)

if (-not $TargetBoard) {
    Write-Host "========================================================"
    Write-Host "Vui long nhap ten board can chuyen doi!"
    Write-Host "Cach dung:"
    Write-Host "  .\select_board.ps1 voicebot    [Cho VoiceBot - COM4]"
    Write-Host "  .\select_board.ps1 hexapod     [Cho Hexapod Bot - COM10]"
    Write-Host "========================================================"
    exit 1
}

$ConfigFile = "sdkconfig.defaults.esp32s3"

if ($TargetBoard -eq "voicebot") {
    Write-Host "[SelectBoard] Chuyen doi sang BOARD_TYPE_HEXAPOD_VOICEBOT..."
    $BoardCfg = "CONFIG_BOARD_TYPE_HEXAPOD_VOICEBOT=y"
} elseif ($TargetBoard -eq "hexapod") {
    Write-Host "[SelectBoard] Chuyen doi sang BOARD_TYPE_HEXAPOD_BOT..."
    $BoardCfg = "CONFIG_BOARD_TYPE_HEXAPOD_BOT=y"
} else {
    Write-Host "========================================================"
    Write-Host "Ten board khong hop le: $TargetBoard"
    Write-Host "Vui long chon 'voicebot' hoac 'hexapod'."
    Write-Host "========================================================"
    exit 1
}

# Noi dung config - su dung LF (Unix line ending) de ESP-IDF parser xu ly dung
# KHONG dung heredoc PowerShell vi co the tao CRLF
$lines = @(
    "CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y",
    "CONFIG_ESPTOOLPY_FLASHMODE_QIO=y",
    "",
    "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y",
    "",
    "CONFIG_SPIRAM=y",
    "CONFIG_SPIRAM_MODE_OCT=y",
    "CONFIG_SPIRAM_SPEED_80M=y",
    "CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=512",
    "CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=65536",
    "CONFIG_SPIRAM_MEMTEST=n",
    "CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=y",
    "",
    "CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=3",
    "CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=6",
    "CONFIG_ESP_WIFI_RX_BA_WIN=3",
    "CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=16",
    "CONFIG_MBEDTLS_DYNAMIC_FREE_CONFIG_DATA=y",
    "",
    "CONFIG_ESP32S3_INSTRUCTION_CACHE_32KB=y",
    "CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y",
    "",
    "CONFIG_SR_WN_WN9_NIHAOXIAOZHI_TTS=y",
    "",
    "CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=4096",
    "",
    "# LVGL Graphics",
    "CONFIG_LV_USE_SNAPSHOT=y",
    "",
    "# LVGL Enable built-in fonts",
    "CONFIG_LV_FONT_MONTSERRAT_20=y",
    "CONFIG_LV_FONT_MONTSERRAT_22=y",
    "CONFIG_LV_FONT_MONTSERRAT_28=y",
    "CONFIG_LV_FONT_MONTSERRAT_48=y",
    "",
    "# FAT Filesystem support",
    "CONFIG_FATFS_LFN_HEAP=y",
    "CONFIG_FATFS_API_ENCODING_UTF_8=y",
    "CONFIG_FATFS_FS_LOCK=4",
    "",
    "# Camera Configuration",
    "CONFIG_CAMERA_OV5640=y",
    "CONFIG_CAMERA_OV5640_AUTO_DETECT_DVP_INTERFACE_SENSOR=y",
    "",
    "# Enable ESP32-S3 Hardware JPEG Encoder for faster camera stream",
    "CONFIG_XIAOZHI_ENABLE_HARDWARE_JPEG_ENCODER=y",
    "",
    "# Selected Board Type (ghi boi select_board.ps1 - khong sua thu cong)",
    $BoardCfg,
    ""
)

# Ghi file voi LF line ending (khong phai CRLF) va UTF-8 khong BOM
$content = $lines -join "`n"
$encoding = New-Object System.Text.UTF8Encoding($false)  # $false = no BOM
$filePath = Join-Path (Get-Location) $ConfigFile
[System.IO.File]::WriteAllText($filePath, $content, $encoding)

# Xoa folder build va file sdkconfig cu de ESP-IDF generate lai tu defaults
if (Test-Path "build") {
    Write-Host "[SelectBoard] Xoa thu muc build cu..."
    try {
        Remove-Item -Recurse -Force "build" -ErrorAction Stop
    } catch {
        Write-Host "========================================================" -ForegroundColor Red
        Write-Host "LOI: Khong the xoa thu muc 'build'!" -ForegroundColor Red
        Write-Host "Co the ban dang mo Terminal chay 'idf.py monitor'." -ForegroundColor Red
        Write-Host "Vui long nhan Ctrl+C o cac Terminal khac roi thu lai!" -ForegroundColor Red
        Write-Host "========================================================" -ForegroundColor Red
        exit 1
    }
}
if (Test-Path "sdkconfig") {
    Write-Host "[SelectBoard] Xoa file sdkconfig cu de cap nhat cau hinh moi..."
    Remove-Item -Force "sdkconfig" -ErrorAction Ignore
}
if (Test-Path "sdkconfig.old") {
    Remove-Item -Force "sdkconfig.old" -ErrorAction Ignore
}

Write-Host "========================================================"
Write-Host "CHUYEN DOI BOARD THANH CONG: $BoardCfg"
Write-Host "Chay lenh tiep theo de build va flash:"
Write-Host "  . D:\Espressif\frameworks\esp-idf-v5.5.4\export.ps1"
Write-Host "  idf.py set-target esp32s3"
Write-Host "  idf.py -p COMx -b 921600 build flash"
Write-Host "========================================================"

exit 0

