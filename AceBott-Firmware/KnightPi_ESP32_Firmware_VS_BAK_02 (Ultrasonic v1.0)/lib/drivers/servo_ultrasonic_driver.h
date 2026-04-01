#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

#include <ESP32Servo.h>

class ServoDriver
{
public:
    void init(int pin);
    void writeAngle(int angle);

private:
    Servo m_servo;
};

#endif