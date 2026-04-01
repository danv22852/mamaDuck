#ifndef COMMAND_IDS_H
#define COMMAND_IDS_H

enum CommandType
{
    CMD_UNKNOWN = 0,
    CMD_PING,
    CMD_LED_ON,
    CMD_LED_OFF,
    CMD_BLINK,
    CMD_MOVE_FW,
    CMD_MOVE_BW,
    CMD_MOVE_LEFT,
    CMD_MOVE_RIGHT,
    CMD_ROTATE_CW,
    CMD_ROTATE_CCW,
    CMD_STOP,
    CMD_US_TEST,
    CMD_SCAN_US
};

#endif