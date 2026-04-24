#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <vehicle.h>

class MotorDriver
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
    vehicle m_vehicle;
};

#endif