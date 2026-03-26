#ifndef CLAW_CONTROLLER_H
#define CLAW_CONTROLLER_H

#include "DRIVERS-claw_servo_driver.h"

class ClawController
{
public:
    ClawController();

    void init(int pin);

    void open();
    void close();
    void setAngle(int angle);
    void step(int delta);

    int getAngle() const;

private:
    int clampAngle(int angle) const;

private:
    ClawServoDriver m_servoDriver;
    int m_currentAngle;
};

#endif