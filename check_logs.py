import subprocess
import time
import sys

# First, reset the ESP32 via DTR/RTS
proc = subprocess.Popen(
    ['cmd', '/c', 'cd /d D:\\Robot\\Hexapod_pet\\xiaozhi-esp32_vietnam_new && call D:\\Espressif\\frameworks\\esp-idf-v5.5.4\\export.bat && python -m esptool --chip esp32s3 -p COM10 --no-stub reset'],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True
)
try:
    outs, _ = proc.communicate(timeout=10)
    print("Reset:", outs[-200:] if len(outs) > 200 else outs)
except:
    proc.terminate()

# Then connect monitor
proc2 = subprocess.Popen(
    ['cmd', '/c', 'cd /d D:\\Robot\\Hexapod_pet\\xiaozhi-esp32_vietnam_new && call D:\\Espressif\\frameworks\\esp-idf-v5.5.4\\export.bat && idf.py -p COM10 monitor'],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True
)

lines = []
start = time.time()
while time.time() - start < 20:
    try:
        line = proc2.stdout.readline()
        if line:
            lines.append(line.rstrip())
    except:
        break

proc2.terminate()
try:
    proc2.wait(timeout=3)
except:
    proc2.kill()

# Print only boot/initialization lines
for l in lines:
    if any(kw in l.lower() for kw in ['hexapod', 'display', 'backlight', 'emotion', 'st7735', 'st7789', 'panel', 'spi', 'gpio', 'error', 'i2c', 'servo', 'initialized', 'esp-idf', 'boot', 'reset', 'ready', 'failed', 'assert', 'panic', 'watchdog', 'found', 'chip']):
        print(l)
