import serial
import time
import sys

port = 'COM4'
baudrate = 115200
timeout = 1

try:
    print(f"Opening {port} at {baudrate} baud...")
    with serial.Serial(port, baudrate, timeout=timeout) as ser:
        # Toggle DTR/RTS to reset the ESP32
        ser.setDTR(False)
        ser.setRTS(False)
        time.sleep(0.1)
        ser.setDTR(True)
        ser.setRTS(True)
        time.sleep(0.1)
        ser.setDTR(False)
        ser.setRTS(False)

        print("Waiting for data...")
        end_time = time.time() + 15  # listen for 15 seconds
        while time.time() < end_time:
            line = ser.readline()
            if line:
                try:
                    text = line.decode('utf-8').strip()
                    print(text)
                except UnicodeDecodeError:
                    print(f"[raw] {line.hex()}")
except Exception as e:
    print(f"Error: {e}")
