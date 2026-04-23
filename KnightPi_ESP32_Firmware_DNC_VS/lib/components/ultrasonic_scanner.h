#ifndef ULTRASONIC_SCANNER_H
#define ULTRASONIC_SCANNER_H

#include "types.h"
#include "drive_base.h"
#include "ultrasonic_driver.h"

class UltrasonicScanner
{
public:
    void init(int trigPin, int echoPin, DriveBase* driveBase);
    void startBackgroundMonitorTask();
    float readDistanceMm();

    bool usScan(
        float scanAngleDeg,
        int steps,
        unsigned long stepRotateMs,
        int rotateSpeed,
        ScanRotation rotation,
        UltrasonicSweepResult& result
    );

private:
    static void monitorTaskEntry(void* param);
    void monitorTaskLoop();

private:
    UltrasonicDriver m_ultrasonicDriver;
    DriveBase* m_driveBase = nullptr;
};

#endif