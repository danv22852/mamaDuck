#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <Arduino.h>
#include "types.h"
#include "../common/event_bus.h"

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

    void init();
    bool submit(const Command& command);
    bool isImmediateCommand(CommandType type) const;
    void dispatchImmediate(const Command& command);

private:
    static void commandTaskEntry(void* param);
    static void serialNotificationTaskEntry(void* param);

    void commandTaskLoop();
    void serialNotificationTaskLoop();
    void executeQueuedCommand(const Command& command);

    bool canStartDriveTask() const;
    bool canStartArmTask() const;
    bool runTimedMove(const Command& command);

    bool receiveAnyObstacleEvent(unsigned long timeoutMs, RobotEvent& outEvent);
    bool receiveObstacleDetectedEventAfterSequence(unsigned long timeoutMs, unsigned long baselineSequence, RobotEvent& outEvent);

    void drainQueue(QueueHandle_t queue);
    void drainPendingObstacleEvents();
    void stopDriveAndClearRuntime();
    bool isObstacleCurrentlyBlockingDrive() const;

    void writeJsonStatusResponse(const Command& command) const;
    void writeJsonOk(const Command& command, const char* commandName) const;
    void writeJsonError(const Command& command, const char* commandName, const char* code, const char* message) const;
    void writeJsonUsScanResponse(const Command& command, const UltrasonicSweepResult& result) const;
    void writeJsonPickAnglesSpeedResponse(const Command& command, const ArmPickResult& result) const;
    void writeJsonDropAnglesSpeedResponse(const Command& command, const ArmDropResult& result) const;
    void writeJsonMoveArmAnglesSpeedResponse(const Command& command) const;

private:
    QueueHandle_t m_commandQueue;
    QueueHandle_t m_commandEventQueue;
    QueueHandle_t m_serialEventQueue;

    DriveBase& m_driveBase;
    UltrasonicScanner& m_ultrasonicScanner;
    RoboticArm& m_roboticArm;
};

#endif