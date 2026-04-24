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

    void rotateClockwiseForMs(int speed, unsigned long amountMs);
    void rotateCounterClockwiseForMs(int speed, unsigned long amountMs);

private:
    MotorDriver m_motorDriver;
};

#endif