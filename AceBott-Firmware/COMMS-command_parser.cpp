#include "COMMS-command_parser.h"

Command CommandParser::parse(const String& rawCommand) const
{
    Command command;
    command.originalText = rawCommand;
    command.type = parseCommandType(rawCommand);
    command.hasIntValue = false;
    command.intValue = 0;

    String trimmed = rawCommand;
    trimmed.trim();

    if (trimmed.startsWith("CLAW_SET "))
    {
        String valueText = trimmed.substring(String("CLAW_SET ").length());
        command.hasIntValue = true;
        command.intValue = valueText.toInt();
    }
    else if (trimmed.startsWith("CLAW_STEP "))
    {
        String valueText = trimmed.substring(String("CLAW_STEP ").length());
        command.hasIntValue = true;
        command.intValue = valueText.toInt();
    }

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

    if (cmd == "CLAW_OPEN") return CMD_CLAW_OPEN;
    if (cmd == "CLAW_CLOSE") return CMD_CLAW_CLOSE;
    if (cmd == "CLAW_STATUS") return CMD_CLAW_STATUS;
    if (cmd.startsWith("CLAW_SET ")) return CMD_CLAW_SET;
    if (cmd.startsWith("CLAW_STEP ")) return CMD_CLAW_STEP;

    return CMD_UNKNOWN;
}