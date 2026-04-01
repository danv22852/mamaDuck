#include "servo_ultrasonic_driver.h"

void ServoDriver::init(int pin)
{
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    m_servo.setPeriodHertz(50);
    m_servo.attach(pin, 1000, 2000);
}

void ServoDriver::writeAngle(int angle)
{
    m_servo.write(angle);
}