#ifndef ULTRASONIC_DRIVER_H
#define ULTRASONIC_DRIVER_H

#include <ultrasonic.h>

class UltrasonicDriver
{
public:
    void init(int trigPin, int echoPin);
    float readDistanceCm();
    float readDistanceMm();

private:
    ultrasonic m_ultrasonic;
};

#endif