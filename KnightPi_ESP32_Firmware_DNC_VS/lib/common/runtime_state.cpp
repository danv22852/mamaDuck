#include "runtime_state.h"

RuntimeState& RuntimeState::instance()
{
    static RuntimeState s_instance;
    return s_instance;
}

RuntimeState::RuntimeState()
    : m_mutex(nullptr),
      m_state()
{
}

void RuntimeState::begin()
{
    if (m_mutex == nullptr)
    {
        m_mutex = xSemaphoreCreateMutex();
    }
}

void RuntimeState::setDriveActive(bool active, CommandType commandType)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.driveActive = active;
    m_state.driveCommand = active ? commandType : CMD_UNKNOWN;

    if (!active)
    {
        m_state.cancelDriveRequested = false;
    }

    xSemaphoreGive(m_mutex);
}

void RuntimeState::setArmActive(bool active, CommandType commandType)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.armActive = active;
    m_state.armCommand = active ? commandType : CMD_UNKNOWN;

    if (!active)
    {
        m_state.cancelArmRequested = false;
    }

    xSemaphoreGive(m_mutex);
}

void RuntimeState::setUltrasonicSweepActive(bool active)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.ultrasonicSweepActive = active;
    xSemaphoreGive(m_mutex);
}

void RuntimeState::setUltrasonicMonitorEnabled(bool enabled)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.ultrasonicMonitorEnabled = enabled;

    if (!enabled)
    {
        m_state.ultrasonicObstacleActive = false;
        m_state.ultrasonicObstacleDistanceMm = 0.0f;
    }

    xSemaphoreGive(m_mutex);
}

bool RuntimeState::isUltrasonicMonitorEnabled() const
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    bool value = m_state.ultrasonicMonitorEnabled;
    xSemaphoreGive(m_mutex);
    return value;
}

void RuntimeState::setUltrasonicMonitorPausedForSweep(bool paused)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.ultrasonicMonitorPausedForSweep = paused;
    xSemaphoreGive(m_mutex);
}

bool RuntimeState::isUltrasonicMonitorPausedForSweep() const
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    bool value = m_state.ultrasonicMonitorPausedForSweep;
    xSemaphoreGive(m_mutex);
    return value;
}

void RuntimeState::setUltrasonicAlarmDistanceMm(float distanceMm)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.ultrasonicAlarmDistanceMm = distanceMm;
    xSemaphoreGive(m_mutex);
}

float RuntimeState::getUltrasonicAlarmDistanceMm() const
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    float value = m_state.ultrasonicAlarmDistanceMm;
    xSemaphoreGive(m_mutex);
    return value;
}

void RuntimeState::setLatestDistanceMm(float distanceMm)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.latestDistanceMm = distanceMm;
    xSemaphoreGive(m_mutex);
}

void RuntimeState::setUltrasonicObstacleActive(bool active, float distanceMm, bool incrementSequence)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.ultrasonicObstacleActive = active;
    m_state.ultrasonicObstacleDistanceMm = distanceMm;

    if (incrementSequence)
    {
        m_state.ultrasonicAlarmSequence++;
    }

    xSemaphoreGive(m_mutex);
}

void RuntimeState::requestCancelAll()
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.cancelDriveRequested = true;
    m_state.cancelArmRequested = true;
    xSemaphoreGive(m_mutex);
}

void RuntimeState::requestCancelDrive()
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.cancelDriveRequested = true;
    xSemaphoreGive(m_mutex);
}

void RuntimeState::requestCancelArm()
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.cancelArmRequested = true;
    xSemaphoreGive(m_mutex);
}

void RuntimeState::clearDriveCancel()
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.cancelDriveRequested = false;
    xSemaphoreGive(m_mutex);
}

void RuntimeState::clearArmCancel()
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.cancelArmRequested = false;
    xSemaphoreGive(m_mutex);
}

bool RuntimeState::shouldCancelDrive() const
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    bool value = m_state.cancelDriveRequested;
    xSemaphoreGive(m_mutex);
    return value;
}

bool RuntimeState::shouldCancelArm() const
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    bool value = m_state.cancelArmRequested;
    xSemaphoreGive(m_mutex);
    return value;
}

void RuntimeState::snapshot(RuntimeSnapshot& out) const
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    out = m_state;
    xSemaphoreGive(m_mutex);
}