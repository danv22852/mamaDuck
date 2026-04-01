#include "command_parser.h"

Command CommandParser::parse(const String& rawCommand) const
{
    Command command;
    command.originalText = rawCommand;
    command.type = parseCommandType(rawCommand);
    return command;
}

CommandType CommandParser::parseCommandType(String cmd) const
{
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "PING") return CMD_PING;
    if (cmd == "LED_ON") return CMD_LED_ON;
    if (cmd == "LED_OFF") return CMD_LED_OFF;
    if (cmd == "BLINK") return CMD_BLINK;
    if (cmd == "MOVE_FW") return CMD_MOVE_FW;
    if (cmd == "MOVE_BW") return CMD_MOVE_BW;
    if (cmd == "MOVE_LEFT") return CMD_MOVE_LEFT;
    if (cmd == "MOVE_RIGHT") return CMD_MOVE_RIGHT;
    if (cmd == "ROTATE_CW") return CMD_ROTATE_CW;
    if (cmd == "ROTATE_CCW") return CMD_ROTATE_CCW;
    if (cmd == "STOP") return CMD_STOP;
    if (cmd == "US_TEST") return CMD_US_TEST;
    if (cmd == "SCAN_US") return CMD_SCAN_US;

    return CMD_UNKNOWN;
}