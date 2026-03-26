#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <Arduino.h>
#include "COMMON-Types.h"

class CommandParser
{
public:
    Command parse(const String& rawCommand) const;

private:
    CommandType parseCommandType(String cmd) const;
};

#endif