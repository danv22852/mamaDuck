#include <Arduino.h>
#include "drive_base.h"

void DriveBase::init()
{
    m_motorDriver.init();
}

void DriveBase::moveForward(int speed)
{
    m_motorDriver.moveForward(speed);
}

void DriveBase::moveBackward(int speed)
{
    m_motorDriver.moveBackward(speed);
}

void DriveBase::moveLeft(int speed)
{
    m_motorDriver.moveLeft(speed);
}

void DriveBase::moveRight(int speed)
{
    m_motorDriver.moveRight(speed);
}

void DriveBase::rotateClockwise(int speed)
{
    m_motorDriver.rotateClockwise(speed);
}

void DriveBase::rotateCounterClockwise(int speed)
{
    m_motorDriver.rotateCounterClockwise(speed);
}

void DriveBase::stop()
{
    m_motorDriver.stop();
}

void DriveBase::rotateClockwiseForMs(int speed, unsigned long amountMs)
{
    m_motorDriver.rotateClockwise(speed);
    delay(amountMs);
    m_motorDriver.stop();
}

void DriveBase::rotateCounterClockwiseForMs(int speed, unsigned long amountMs)
{
    m_motorDriver.rotateCounterClockwise(speed);
    delay(amountMs);
    m_motorDriver.stop();
}