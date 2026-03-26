#include "COMPONENTS-drive_base.h"

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