#ifndef COMMAND_IDS_H
#define COMMAND_IDS_H

enum CommandType
{
    CMD_UNKNOWN = 0,

    // ESP32 System
    CMD_PING,

    // LED
    CMD_LED_ON,
    CMD_LED_OFF,
    CMD_BLINK,

    // Motor Drive
    CMD_MOVE_FW,
    CMD_MOVE_BW,
    CMD_MOVE_LEFT,
    CMD_MOVE_RIGHT,
    CMD_ROTATE_CW,
    CMD_ROTATE_CCW,
    CMD_STOP,

    // Ultrasonic Sensor
    CMD_US_TEST,
    CMD_US_SCAN,

    // Robotic Arm
    CMD_PICK_OBJECT,
    CMD_PICK_ANGLES,
    CMD_DROP_ANGLES,
    CMD_MOVE_ARM_ANGLES
};

#endif