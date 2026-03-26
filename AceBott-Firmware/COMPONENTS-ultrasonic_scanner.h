#ifndef ULTRASONIC_SCANNER_H
#define ULTRASONIC_SCANNER_H

#include "COMMON-Types.h"
#include "DRIVERS-ultrasonic_driver.h"
#include "DRIVERS-servo_ultrasonic_driver.h"

class UltrasonicScanner
{
public:
    void init(int trigPin, int echoPin, int servoPin);
    float readDistanceMm();
    ScanResult scanLeftCenterRight();

private:
    UltrasonicDriver m_ultrasonicDriver;
    ServoDriver m_servoDriver;
};

#endif