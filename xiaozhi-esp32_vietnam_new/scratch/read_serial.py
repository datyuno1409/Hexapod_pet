import serial
import time
import sys

port = 'COM10'
baudrate = 460800
timeout = 1

try:
    with serial.Serial(port, baudrate, timeout=timeout) as ser:
        end_time = time.time() + 15  # listen for 15 seconds
        with open('com10_crash_log.txt', 'w', encoding='utf-8') as f:
            while time.time() < end_time:
                line = ser.readline()
                if line:
                    decoded = line.decode('utf-8', errors='ignore').strip()
                    f.write(decoded + '\n')
                    f.flush()
    print("Done listening to COM10")
except Exception as e:
    print(f"Error: {e}")
