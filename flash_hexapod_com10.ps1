# Flash Hexapod Bot (COM10) - BOARD_TYPE_HEXAPOD_BOT
# Su dung: .\flash_hexapod_com10.ps1
# Yeu cau: ESP-IDF v5.5.4 tai D:\Espressif\frameworks\esp-idf-v5.5.4

$ProjectDir = "D:\Robot\Hexapod_pet\xiaozhi-esp32_vietnam_new"
$IdfExport  = "D:\Espressif\frameworks\esp-idf-v5.5.4\export.ps1"
$Port       = "COM10"
$Baud       = "921600"

Write-Host "========================================================"
Write-Host " FLASH HEXAPOD BOT (COM10) - BOARD_TYPE_HEXAPOD_BOT"
Write-Host "========================================================"

Set-Location $ProjectDir

# Buoc 1: Chon board
Write-Host "`n[1/4] Chon board: hexapod..."
& ".\select_board.ps1" hexapod
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] select_board.ps1 that bai!" -ForegroundColor Red
    exit 1
}

# Buoc 2: Load ESP-IDF environment
Write-Host "`n[2/4] Khoi tao ESP-IDF environment..."
. $IdfExport

# Buoc 3: Set target
Write-Host "`n[3/4] Set target esp32s3..."
idf.py set-target esp32s3
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] set-target that bai!" -ForegroundColor Red
    exit 1
}

# Buoc 4: Build va Flash
Write-Host "`n[4/4] Build va Flash len $Port (baud $Baud)..."
idf.py -p $Port -b $Baud build flash
if ($LASTEXITCODE -ne 0) {
    Write-Host "`n[ERROR] Build/Flash that bai! Kiem tra ket noi COM10." -ForegroundColor Red
    exit 1
}

Write-Host "`n========================================================"
Write-Host " FLASH HEXAPOD BOT THANH CONG!"
Write-Host " De monitor: idf.py -p $Port monitor"
Write-Host "========================================================"
