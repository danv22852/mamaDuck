#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <Arduino.h>
#include "types.h"
#include "drive_base.h"
#include "ultrasonic_scanner.h"

class CommandDispatcher
{
public:
    CommandDispatcher(DriveBase& driveBase, UltrasonicScanner& ultrasonicScanner);

    void dispatch(const Command& command);

private:
    void handleBlink();
    void handleTimedMove(CommandType commandType);
    void printReadyBanner();

private:
    DriveBase& m_driveBase;
    UltrasonicScanner& m_ultrasonicScanner;
};

#endif