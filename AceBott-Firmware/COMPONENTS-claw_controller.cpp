#include <Arduino.h>
#include "COMMON-Config.h"
#include "COMPONENTS-claw_controller.h"

ClawController::ClawController()
    : m_currentAngle(CLAW_DEFAULT_ANGLE)
{
}

void ClawController::init(int pin)
{
    m_servoDriver.init(pin);
    m_servoDriver.writeAngle(m_currentAngle);
    delay(CLAW_SETTLE_MS);
}

void ClawController::open()
{
    setAngle(CLAW_OPEN_ANGLE);
}

void ClawController::close()
{
    setAngle(CLAW_CLOSED_ANGLE);
}

void ClawController::setAngle(int angle)
{
    m_currentAngle = clampAngle(angle);
    m_servoDriver.writeAngle(m_currentAngle);
    delay(CLAW_SETTLE_MS);
}

void ClawController::step(int delta)
{
    setAngle(m_currentAngle + delta);
}

int ClawController::getAngle() const
{
    return m_currentAngle;
}

int ClawController::clampAngle(int angle) const
{
    if (angle < CLAW_MIN_ANGLE)
    {
        return CLAW_MIN_ANGLE;
    }

    if (angle > CLAW_MAX_ANGLE)
    {
        return CLAW_MAX_ANGLE;
    }

    return angle;
}