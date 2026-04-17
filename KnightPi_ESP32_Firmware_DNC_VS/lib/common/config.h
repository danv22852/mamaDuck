#ifndef CONFIG_H
#define CONFIG_H

// Serial-Listener settings
constexpr unsigned long SERIAL_BAUD_RATE = 115200;

constexpr unsigned long STARTUP_DELAY_MS = 1000;
constexpr unsigned long DEFAULT_MOVE_DURATION_MS = 1000;
constexpr int DEFAULT_MOVE_SPEED = 255;

// Ultrasonic-Sensor settings
constexpr int DEFAULT_US_SCAN_ROTATE_SPEED = 250;
constexpr unsigned long DEFAULT_US_SCAN_STEP_ROTATE_MS = 200;
constexpr unsigned long US_SCAN_SETTLE_MS = 200;

#endif