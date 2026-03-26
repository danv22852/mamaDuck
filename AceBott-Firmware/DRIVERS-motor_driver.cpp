#include "DRIVERS-motor_driver.h"

void MotorDriver::init()
{
    m_vehicle.Init();
}

void MotorDriver::moveForward(int speed)
{
    m_vehicle.Move(Forward, speed);
}

void MotorDriver::moveBackward(int speed)
{
    m_vehicle.Move(Backward, speed);
}

void MotorDriver::moveLeft(int speed)
{
    m_vehicle.Move(Move_Left, speed);
}

void MotorDriver::moveRight(int speed)
{
    m_vehicle.Move(Move_Right, speed);
}

void MotorDriver::rotateClockwise(int speed)
{
    m_vehicle.Move(Clockwise, speed);
}

void MotorDriver::rotateCounterClockwise(int speed)
{
    m_vehicle.Move(Contrarotate, speed);
}

void MotorDriver::stop()
{
    m_vehicle.Move(Stop, 0);
}