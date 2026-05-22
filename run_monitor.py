import subprocess
import time

proc = subprocess.Popen(
    ['cmd', '/c', 'cd /d D:\\Robot\\Hexapod_pet\\xiaozhi-esp32_vietnam_new && call D:\\Espressif\\frameworks\\esp-idf-v5.5.4\\export.bat && idf.py -p COM10 monitor'],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True
)

lines = []
start = time.time()
while time.time() - start < 15:
    line = proc.stdout.readline()
    if line:
        lines.append(line.rstrip())
    elif proc.poll() is not None:
        break

proc.terminate()
try:
    proc.wait(timeout=3)
except:
    proc.kill()

for l in lines[-50:]:
    print(l)
