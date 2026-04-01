#ifndef DRIVE_BASE_H
#define DRIVE_BASE_H

#include "motor_driver.h"

class DriveBase
{
public:
    void init();

    void moveForward(int speed);
    void moveBackward(int speed);
    void moveLeft(int speed);
    void moveRight(int speed);
    void rotateClockwise(int speed);
    void rotateCounterClockwise(int speed);
    void stop();

private:
    MotorDriver m_motorDriver;
};

#endif