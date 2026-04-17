#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <Arduino.h>
#include "types.h"

class CommandParser
{
public:
    Command parse(const String& rawCommand) const;

private:
    CommandType parseCommandType(String cmd) const;
    bool parseJsonEnvelope(const String& rawCommand, Command& command) const;
    bool parsePickObjectCommand(const String& rawCommand, Command& command) const;
    bool parseTimedMoveCommand(const String& rawCommand, Command& command) const;
    bool parseUsScanCommand(const String& rawCommand, Command& command) const;

    ArmMotionMode parseArmMotionMode(const String& text) const;
    ArmServoId parseServoId(const String& text) const;
};

#endif