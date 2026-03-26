#ifndef CLAW_SERVO_DRIVER_H
#define CLAW_SERVO_DRIVER_H

#include <ESP32Servo.h>

class ClawServoDriver
{
public:
    void init(int pin);
    void writeAngle(int angle);

private:
    Servo m_servo;
};

#endif