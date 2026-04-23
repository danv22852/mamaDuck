#include <Arduino.h>
#include "config.h"
#include "ultrasonic_scanner.h"
#include "../common/runtime_state.h"
#include "../common/event_bus.h"

void UltrasonicScanner::init(int trigPin, int echoPin, DriveBase* driveBase)
{
    m_ultrasonicDriver.init(trigPin, echoPin);
    m_driveBase = driveBase;
}

void UltrasonicScanner::startBackgroundMonitorTask()
{
    xTaskCreatePinnedToCore(
        UltrasonicScanner::monitorTaskEntry,
        "us_monitor_task",
        4096,
        this,
        1,
        nullptr,
        1
    );
}

void UltrasonicScanner::monitorTaskEntry(void* param)
{
    UltrasonicScanner* self = static_cast<UltrasonicScanner*>(param);
    self->monitorTaskLoop();
}

void UltrasonicScanner::monitorTaskLoop()
{
    RuntimeState& state = RuntimeState::instance();
    EventBus& bus = EventBus::instance();

    while (true)
    {
        RuntimeSnapshot snapshot;
        state.snapshot(snapshot);

        bool canReadAndEvaluate =
            snapshot.ultrasonicMonitorEnabled &&
            !snapshot.ultrasonicMonitorPausedForSweep &&
            !snapshot.ultrasonicSweepActive;

        if (!canReadAndEvaluate)
        {
            vTaskDelay(pdMS_TO_TICKS(US_MONITOR_POLL_MS));
            continue;
        }

        float distanceMm = m_ultrasonicDriver.readDistanceMm();
        state.setLatestDistanceMm(distanceMm);

        bool inAlarm =
            distanceMm > 0.0f &&
            snapshot.ultrasonicAlarmDistanceMm > 0.0f &&
            distanceMm <= snapshot.ultrasonicAlarmDistanceMm;

        if (inAlarm && !snapshot.ultrasonicObstacleActive)
        {
            state.setUltrasonicObstacleActive(true, distanceMm, true);

            RuntimeSnapshot afterRaise;
            state.snapshot(afterRaise);

            RobotEvent event;
            event.type = EVENT_US_OBSTACLE_DETECTED;
            event.distanceMm = distanceMm;
            event.alarmDistanceMm = afterRaise.ultrasonicAlarmDistanceMm;
            event.sequence = afterRaise.ultrasonicAlarmSequence;
            bus.publish(event);
        }
        else if (!inAlarm && snapshot.ultrasonicObstacleActive)
        {
            state.setUltrasonicObstacleActive(false, distanceMm, false);

            RobotEvent event;
            event.type = EVENT_US_OBSTACLE_CLEARED;
            event.distanceMm = distanceMm;
            event.alarmDistanceMm = snapshot.ultrasonicAlarmDistanceMm;
            event.sequence = snapshot.ultrasonicAlarmSequence;
            bus.publish(event);
        }

        vTaskDelay(pdMS_TO_TICKS(US_MONITOR_POLL_MS));
    }
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

    RuntimeState& state = RuntimeState::instance();
    const float stepAngleDeg = scanAngleDeg / (float)steps;

    result.points[result.pointCount].angleDeg = 0.0f;
    result.points[result.pointCount].distanceMm = m_ultrasonicDriver.readDistanceMm();
    result.pointCount++;

    for (int i = 1; i < steps; i++)
    {
        if (state.shouldCancelDrive())
        {
            m_driveBase->stop();
            return false;
        }

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

        vTaskDelay(pdMS_TO_TICKS(US_SCAN_SETTLE_MS));
        result.points[result.pointCount].distanceMm = m_ultrasonicDriver.readDistanceMm();
        result.pointCount++;
    }

    m_driveBase->stop();
    return true;
}