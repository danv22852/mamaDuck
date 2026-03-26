import requests
import os
import time
import subprocess

# --- Configuration ---
ENDPOINT_URL = "http://10.40.82.150:5043/upload" # Replace with your C# server IP
IMAGE_PATH = "/home/pi/Pictures/latest_capture.jpg"
CAPTURE_INTERVAL = 5 # How many seconds to wait between uploads

def capture_image(file_path):
    """
    Captures an image using the modern Raspberry Pi camera stack (libcamera).
    """
    print("📸 Capturing image...")
    try:
        # 'libcamera-jpeg' or 'rpicam-jpeg' takes a picture. 
        # -o specifies the output file.
        # -t 1000 gives the camera sensor 1 second (1000ms) to warm up and adjust exposure/white balance.
        # --width and --height shrink the image to save bandwidth (optional but recommended for fast uploads).
        command = [
            "libcamera-jpeg", 
            "-o", file_path, 
            "-t", "1000",
            "--width", "1280", 
            "--height", "720",
            "--nopreview" # Don't try to open a preview window on the Pi
        ]
        
        # Note: If you are on the newest Pi OS (Bookworm), you might need to change 
        # "libcamera-jpeg" to "rpicam-jpeg" in the list above.
        subprocess.run(command, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return True
    except subprocess.CalledProcessError as e:
        print(f"❌ Camera capture failed. Is the camera connected? Error: {e}")
        return False
    except FileNotFoundError:
        print("❌ 'libcamera-jpeg' command not found. If you are on Raspberry Pi OS Bookworm, change 'libcamera-jpeg' to 'rpicam-jpeg' in the script.")
        return False

def upload_image(file_path, url):
    """
    Uploads the image file to the .NET endpoint.
    """
    if not os.path.exists(file_path):
        return False

    try:
        with open(file_path, 'rb') as image_file:
            files = {'file': image_file} # Matches [FromForm] IFormFile file
            
            # Timeout set to 10 seconds so the script doesn't hang forever if the server drops
            response = requests.post(url, files=files, timeout=10)

            if response.status_code == 200:
                print(f"✅ Uploaded successfully. Server saved at: {response.json().get('savedAt')}")
                return True
            else:
                print(f"❌ Upload failed. Status: {response.status_code}")
                return False

    except requests.exceptions.RequestException as e:
        print(f"❌ Network Error: Could not connect to {url}.")
        return False

if __name__ == "__main__":
    print(f"🚀 Starting timelapse upload to {ENDPOINT_URL}")
    print("Press Ctrl+C to stop.")
    
    try:
        # The infinite loop that runs every few seconds
        while True:
            # 1. Take the picture
            capture_success = capture_image(IMAGE_PATH)
            
            # 2. If picture was taken successfully, upload it
            if capture_success:
                upload_image(IMAGE_PATH, ENDPOINT_URL)
            
            # 3. Wait before taking the next one
            print(f"⏳ Waiting {CAPTURE_INTERVAL} seconds...\n")
            time.sleep(CAPTURE_INTERVAL)
            
    except KeyboardInterrupt:
        # This catches the Ctrl+C command so the script exits cleanly
        print("\n🛑 Script stopped by user. Goodbye!")