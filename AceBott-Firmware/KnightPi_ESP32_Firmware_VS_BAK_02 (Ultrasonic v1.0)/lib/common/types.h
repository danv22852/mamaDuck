#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include "command_ids.h"

struct Command
{
    CommandType type;
    String originalText;
};

struct ScanResult
{
    float leftMm;
    float centerMm;
    float rightMm;
};

#endif