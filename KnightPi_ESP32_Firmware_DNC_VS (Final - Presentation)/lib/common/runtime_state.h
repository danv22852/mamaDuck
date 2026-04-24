#ifndef RUNTIME_STATE_H
#define RUNTIME_STATE_H

#include <Arduino.h>
#include "types.h"

class RuntimeState
{
public:
    static RuntimeState& instance();

    void begin();

    void setDriveActive(bool active, CommandType commandType);
    void setArmActive(bool active, CommandType commandType);
    void setUltrasonicSweepActive(bool active);

    void setUltrasonicMonitorEnabled(bool enabled);
    bool isUltrasonicMonitorEnabled() const;

    void setUltrasonicMonitorPausedForSweep(bool paused);
    bool isUltrasonicMonitorPausedForSweep() const;

    void setUltrasonicAlarmDistanceMm(float distanceMm);
    float getUltrasonicAlarmDistanceMm() const;

    void setLatestDistanceMm(float distanceMm);
    void setUltrasonicObstacleActive(bool active, float distanceMm, bool incrementSequence);

    void requestCancelAll();
    void requestCancelDrive();
    void requestCancelArm();

    void clearDriveCancel();
    void clearArmCancel();

    bool shouldCancelDrive() const;
    bool shouldCancelArm() const;

    void snapshot(RuntimeSnapshot& out) const;

private:
    RuntimeState();

private:
    mutable SemaphoreHandle_t m_mutex;
    RuntimeSnapshot m_state;
};

#endif