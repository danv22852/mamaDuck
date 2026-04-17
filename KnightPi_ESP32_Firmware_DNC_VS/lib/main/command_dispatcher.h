#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <Arduino.h>
#include "types.h"

#include "../components/drive_base.h"
#include "../components/ultrasonic_scanner.h"
#include "../components/robotic_arm.h"

class CommandDispatcher
{
public:
    CommandDispatcher(
        DriveBase& driveBase,
        UltrasonicScanner& ultrasonicScanner,
        RoboticArm& roboticArm
    );

    void dispatch(const Command& command);

private:
    void writeJsonOk(const Command& command, const char* commandName) const;
    void writeJsonError(const Command& command, const char* commandName, const char* code, const char* message) const;
    void writeJsonUsTestResponse(const Command& command, float distanceMm) const;
    void writeJsonUsScanResponse(const Command& command, const UltrasonicSweepResult& result) const;
    void writeJsonPickAnglesResponse(const Command& command, const ArmPickResult& result) const;
    void writeJsonDropAnglesResponse(const Command& command, const ArmDropResult& result) const;
    void writeJsonMoveArmAnglesResponse(const Command& command) const;

    void handleBlink();
    void handleTimedMove(CommandType commandType, int amountMs, int speed);
    void printReadyBanner();

private:
    DriveBase& m_driveBase;
    UltrasonicScanner& m_ultrasonicScanner;
    RoboticArm& m_roboticArm;
};

#endif