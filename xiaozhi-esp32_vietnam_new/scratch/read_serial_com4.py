import serial
import time
import sys

port = 'COM4'
baudrate = 460800
timeout = 1

try:
    with serial.Serial(port, baudrate, timeout=timeout) as ser:
        end_time = time.time() + 45  # listen for 45 seconds
        with open('com4_music_log.txt', 'w', encoding='utf-8') as f:
            while time.time() < end_time:
                line = ser.readline()
                if line:
                    decoded = line.decode('utf-8', errors='ignore').strip()
                    f.write(decoded + '\n')
                    f.flush()
    print("Done listening to COM4")
except Exception as e:
    print(f"Error: {e}")
