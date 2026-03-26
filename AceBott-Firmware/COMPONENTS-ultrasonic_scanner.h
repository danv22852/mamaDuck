#ifndef ULTRASONIC_SCANNER_H
#define ULTRASONIC_SCANNER_H

#include "DRIVERS-ultrasonic_driver.h"

class UltrasonicScanner
{
public:
    void init(int trigPin, int echoPin);
    float readDistanceMm();

private:
    UltrasonicDriver m_ultrasonicDriver;
};

#endif