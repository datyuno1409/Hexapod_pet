import serial
import time
import sys

# Try to open COM10 directly
try:
    s = serial.Serial('COM10', 115200, timeout=1)
except Exception as e:
    print(f"Cannot open COM10: {e}")
    sys.exit(1)

# Toggle DTR/RTS for reset
s.dtr = False
s.rts = True
time.sleep(0.1)
s.rts = False
time.sleep(0.5)

# Read boot output
lines = []
start = time.time()
while time.time() - start < 10:
    try:
        line = s.readline()
        if line:
            decoded = line.decode('utf-8', errors='replace').rstrip()
            if decoded:
                lines.append(decoded)
    except:
        break

s.close()

# Filter for relevant lines
for l in lines:
    if l.strip():
        print(l)
