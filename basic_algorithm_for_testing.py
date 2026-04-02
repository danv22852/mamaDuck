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

def listen_to_esp(ser):
    """Runs continuously in a background thread to catch ESP32 responses."""
    while True:
        try:
            if ser and ser.in_waiting > 0:
                raw_bytes = ser.readline()
                incoming_msg = raw_bytes.decode('utf-8', errors='replace').rstrip()
                if incoming_msg:
                    print(f"[ESP32]: {incoming_msg}")
            time.sleep(0.01) # Prevent maxing out the CPU
        except Exception:
            break

if __name__ == '__main__':
    esp32 = setup_connection()
    
    if esp32:
        print("\n--- Duck Tracker Autonomous Sequence ---")
        
        # We keep the listener thread so we can see the ESP32's responses live
        listener_thread = threading.Thread(target=listen_to_esp, args=(esp32,), daemon=True)
        listener_thread.start()
        
        try:
            # 1. Go Straight
            print("\nExecuting: GO STRAIGHT")
            send_command(esp32, "MOVE_FW")
            time.sleep(2) # Give the car 2 seconds to physically drive forward
            
            # 2. Turn Counter-Clockwise
            print("\nExecuting: TURN COUNTER-CLOCKWISE")
            send_command(esp32, "ROTATE_CCW")
            time.sleep(2) # Give the car 1 second to rotate
            
            # 3. Go Backwards
            print("\nExecuting: GO BACKWARDS")
            send_command(esp32, "MOVE_BW")
            time.sleep(2) # Give the car 2 seconds to drive backward
            
            # 4. Turn Clockwise
            print("\nExecuting: TURN CLOCKWISE")
            send_command(esp32, "ROTATE_CW")
            time.sleep(2) # Give the car 1 second to rotate back
            
            # 5. Stop the robot
            print("\nExecuting: STOP")
            send_command(esp32, "STOP")
            
            print("\nSequence complete. Shutting down in 2 seconds...")
            time.sleep(2) # Give it a moment to catch any final messages before closing
                    
        except KeyboardInterrupt:
            # If you panic and hit Ctrl+C, try to tell the robot to stop before exiting!
            print("\nEmergency stop triggered.")
            send_command(esp32, "STOP")
            
        finally:
            esp32.close()
