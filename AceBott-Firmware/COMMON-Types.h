#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include "COMMON-CommandIds.h"

struct Command
{
    CommandType type;
    String originalText;
    bool hasIntValue;
    int intValue;
};

struct ScanResult
{
    float leftMm;
    float centerMm;
    float rightMm;
};

#endif