import serial
import time
import sys

def main():
    port = 'COM10'
    baud = 115200
    print(f"Opening {port} at {baud}...")
    try:
        ser = serial.Serial(port, baud, timeout=1.0)
    except Exception as e:
        print(f"Error opening port: {e}")
        return

    print("Resetting board...")
    # ESP32 Reset sequence
    ser.setDTR(False)
    ser.setRTS(True)
    time.sleep(0.2)
    ser.setRTS(False)
    ser.setDTR(True) # some boards need DTR high or toggle
    time.sleep(0.2)
    ser.setDTR(False)
    
    print("Reading serial output (15 seconds)...")
    start_time = time.time()
    while time.time() - start_time < 15:
        if ser.in_waiting:
            line = ser.readline()
            try:
                decoded = line.decode('utf-8', errors='ignore')
                sys.stdout.write(decoded)
                sys.stdout.flush()
            except Exception as e:
                pass
        else:
            time.sleep(0.01)
            
    ser.close()
    print("\nFinished reading.")

if __name__ == '__main__':
    main()
