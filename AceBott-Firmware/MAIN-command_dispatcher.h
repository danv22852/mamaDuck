#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <Arduino.h>
#include "COMMON-Types.h"
#include "COMPONENTS-drive_base.h"
#include "COMPONENTS-ultrasonic_scanner.h"
#include "COMPONENTS-claw_controller.h"

class CommandDispatcher
{
public:
    CommandDispatcher(
        DriveBase& driveBase,
        UltrasonicScanner& ultrasonicScanner,
        ClawController& clawController
    );

    void dispatch(const Command& command);

private:
    void handleBlink();
    void handleTimedMove(CommandType commandType);
    void printReadyBanner();

private:
    DriveBase& m_driveBase;
    UltrasonicScanner& m_ultrasonicScanner;
    ClawController& m_clawController;
};

#endif