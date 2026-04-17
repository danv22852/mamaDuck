#include <Arduino.h>
#include "config.h"
#include "ultrasonic_scanner.h"

void UltrasonicScanner::init(int trigPin, int echoPin, DriveBase* driveBase)
{
    m_ultrasonicDriver.init(trigPin, echoPin);
    m_driveBase = driveBase;
}

float UltrasonicScanner::readDistanceMm()
{
    return m_ultrasonicDriver.readDistanceMm();
}

bool UltrasonicScanner::usScan(
    float scanAngleDeg,
    int steps,
    unsigned long stepRotateMs,
    int rotateSpeed,
    ScanRotation rotation,
    UltrasonicSweepResult& result
)
{
    result.pointCount = 0;

    if (m_driveBase == nullptr) return false;
    if (scanAngleDeg <= 0.0f) return false;
    if (steps <= 0) return false;
    if (steps > MAX_US_SCAN_POINTS) return false;
    if (stepRotateMs == 0) return false;
    if (rotateSpeed <= 0) return false;
    if (rotateSpeed > 255) return false;

    const float stepAngleDeg = scanAngleDeg / (float)steps;

    result.points[result.pointCount].angleDeg = 0.0f;
    result.points[result.pointCount].distanceMm = m_ultrasonicDriver.readDistanceMm();
    result.pointCount++;

    for (int i = 1; i < steps; i++)
    {
        if (rotation == SCAN_CW)
        {
            m_driveBase->rotateClockwiseForMs(rotateSpeed, stepRotateMs);

            float angleDeg = 360.0f - (stepAngleDeg * i);
            while (angleDeg >= 360.0f) angleDeg -= 360.0f;

            result.points[result.pointCount].angleDeg = angleDeg;
        }
        else
        {
            m_driveBase->rotateCounterClockwiseForMs(rotateSpeed, stepRotateMs);

            float angleDeg = stepAngleDeg * i;
            while (angleDeg >= 360.0f) angleDeg -= 360.0f;

            result.points[result.pointCount].angleDeg = angleDeg;
        }

        delay(US_SCAN_SETTLE_MS);

        result.points[result.pointCount].distanceMm = m_ultrasonicDriver.readDistanceMm();
        result.pointCount++;
    }

    return true;
}