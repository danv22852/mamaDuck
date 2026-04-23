#ifndef CONFIG_H
#define CONFIG_H

constexpr unsigned long SERIAL_BAUD_RATE = 115200;
constexpr unsigned long STARTUP_DELAY_MS = 1000;

constexpr unsigned long DEFAULT_MOVE_DURATION_MS = 1000;
constexpr int DEFAULT_MOVE_SPEED = 255;

constexpr int DEFAULT_US_SCAN_ROTATE_SPEED = 250;
constexpr unsigned long DEFAULT_US_SCAN_STEP_ROTATE_MS = 200;
constexpr unsigned long US_SCAN_SETTLE_MS = 120;

constexpr float DEFAULT_US_MONITOR_ALARM_DISTANCE_MM = 180.0f;
constexpr unsigned long US_MONITOR_POLL_MS = 60;

constexpr unsigned long DRIVE_SLICE_MS = 20;
constexpr unsigned long ARM_IDLE_SLICE_MS = 1;

constexpr int COMMAND_QUEUE_LENGTH = 20;
constexpr int EVENT_QUEUE_LENGTH = 12;
constexpr int MAX_EVENT_SUBSCRIBERS = 8;

#endif