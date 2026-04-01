import serial
import time

# Update this to match what you found in Step 1
ESP_PORT = '/dev/ttyUSB0'
# This MUST match the Serial.begin(115200) in your ESP32's C code
BAUD_RATE = 115200

def setup_connection():
    try:
        # Establish the serial bridge
        ser = serial.Serial(ESP_PORT, BAUD_RATE, timeout=1)

        # When you open a serial port, the ESP32 often resets.
        # Give it 2 seconds to wake back up.
        time.sleep(2)

        # NEW: Clear out the boot garbage before we start reading
        ser.reset_input_buffer()

        print(f"Connected to ESP32 on {ESP_PORT}")
        return ser
    except serial.SerialException as e:
        print(f"Connection failed: {e}")
        return None

def send_command(ser, command):
    """Sends a string command to the ESP32."""
    if ser:
        # The ESP32 expects a newline '\n' to know the command is finished
        formatted_msg = f"{command}\n"
        # We have to encode the string into raw bytes before sending it over U>
        ser.write(formatted_msg.encode('utf-8'))
        print(f"Pi Sent: {command}")

def read_from_esp(ser):
    """Reads incoming data from the ESP32."""
    if ser and ser.in_waiting > 0:
        # NEW: Added errors='replace' so it doesn't crash on bad bytes
        # It will just put a '?' in place of any garbage characters
        raw_bytes = ser.readline()
        incoming_msg = raw_bytes.decode('utf-8', errors='replace').rstrip()

        # Only print if it's not an empty string
        if incoming_msg:
            print(f"ESP32 Says: {incoming_msg}")
            return incoming_msg
    return None

if __name__ == '__main__':
    # 1. Open the bridge
    esp32 = setup_connection()

    if esp32:
        # 2. Send a command to grab a duck!
        send_command(esp32, "CLOSE_CLAW")

        # 3. Listen continuously for what the ESP32 is doing
        print("Listening for ESP32 response...")
        try:
            while True:
                response = read_from_esp(esp32)

                # Example of reacting to the ESP32's feedback
                if response == "CLAW_SECURED":
                    print("Mission Accomplished: Duck is safe!")
                    break

                time.sleep(0.05) # Tiny pause so we don't max out the Pi's CPU

        except KeyboardInterrupt:
            print("\nShutting down comms.")

        finally:
            esp32.close()
