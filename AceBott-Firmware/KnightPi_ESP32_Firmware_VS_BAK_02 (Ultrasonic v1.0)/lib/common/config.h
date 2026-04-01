#ifndef CONFIG_H
#define CONFIG_H

constexpr unsigned long SERIAL_BAUD_RATE = 115200;
constexpr unsigned long STARTUP_DELAY_MS = 1000;
constexpr unsigned long DEFAULT_MOVE_DURATION_MS = 1000;
constexpr int DEFAULT_MOVE_SPEED = 255;

constexpr int SERVO_LEFT = 170;
constexpr int SERVO_CENTER = 80;
constexpr int SERVO_RIGHT = 0;

constexpr unsigned long SERVO_SETTLE_MS = 1000;

#endif