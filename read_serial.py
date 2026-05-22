import serial
import time

s = serial.Serial('COM10', 115200, timeout=3)
time.sleep(1)
s.write(b'\n')  # send newline to trigger any pending output

lines = []
start = time.time()
while time.time() - start < 8:
    try:
        line = s.readline()
        if line:
            lines.append(line.decode(errors='replace'))
    except:
        break
s.close()

for l in lines[-60:]:
    print(l, end='')
