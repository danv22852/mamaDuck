import serial
import time
import threading

# Update this to match your ESP32's port
ESP_PORT = '/dev/ttyUSB0' 
# MUST match Serial.begin() in ESP32 C++ code
BAUD_RATE = 115200 

def setup_connection():
    try:
        ser = serial.Serial(ESP_PORT, BAUD_RATE, timeout=1)
        time.sleep(2) 
        ser.reset_input_buffer() 
        print(f"Connected to ESP32 on {ESP_PORT}")
        return ser
    except serial.SerialException as e:
        print(f"Connection failed: {e}")
        return None

def send_command(ser, command):
    """Sends a string command to the ESP32."""
    if ser:
        formatted_msg = f"{command}\n"
        ser.write(formatted_msg.encode('utf-8'))
        # Optional: Print what the Pi sent for debugging
        # print(f"Pi Sent: {command}")

def listen_to_esp(ser):
    """Runs continuously in a background thread to catch ESP32 responses."""
    while True:
        try:
            if ser and ser.in_waiting > 0:
                raw_bytes = ser.readline()
                incoming_msg = raw_bytes.decode('utf-8', errors='replace').rstrip()
                if incoming_msg:
                    print(f"\n[ESP32]: {incoming_msg}")
                    # Reprint the prompt arrow so it looks clean after an incoming message
                    print(">> ", end="", flush=True) 
            time.sleep(0.01) # Prevent maxing out the CPU
        except Exception:
            # If the serial port closes, exit the thread quietly
            break

if __name__ == '__main__':
    esp32 = setup_connection()
    
    if esp32:
        print("\n--- Duck Tracker Interactive Terminal ---")
        print("Type commands (e.g., LED_ON, MOVE_FW, SCAN_US) and press Enter.")
        print("Type 'exit' to quit.")
        print("-----------------------------------------\n")
        
        # Start the background listener thread
        listener_thread = threading.Thread(target=listen_to_esp, args=(esp32,), daemon=True)
        listener_thread.start()
        
        try:
            while True:
                # Wait for you to type something in the SSH terminal
                user_input = input(">> ")
                
                if user_input.lower() in ['exit', 'quit']:
                    print("Closing connection...")
                    break
                    
                # If the user typed something, send it!
                if user_input.strip():
                    # Send as uppercase to match your C++ parser expectations
                    send_command(esp32, user_input.strip().upper())
                    
        except KeyboardInterrupt:
            print("\nForced shutdown.")
            
        finally:
            esp32.close()