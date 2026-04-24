#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <Arduino.h>
#include "types.h"

class CommandParser
{
public:
    Command parse(const String& rawCommand) const;

private:
    bool parseJsonEnvelope(const String& rawCommand, Command& command) const;

    ArmMotionMode parseArmMotionMode(const String& text) const;
    ArmServoId parseServoId(const String& text) const;
    CancelTarget parseCancelTarget(const String& text) const;
    void copyText(char* dest, size_t destSize, const char* src) const;
};

#endif