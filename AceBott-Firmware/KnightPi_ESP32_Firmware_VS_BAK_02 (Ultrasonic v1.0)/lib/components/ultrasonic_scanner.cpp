#include <Arduino.h>
#include "config.h"
#include "ultrasonic_scanner.h"

void UltrasonicScanner::init(int trigPin, int echoPin, int servoPin)
{
    m_ultrasonicDriver.init(trigPin, echoPin);
    m_servoDriver.init(servoPin);
}

float UltrasonicScanner::readDistanceMm()
{
    return m_ultrasonicDriver.readDistanceMm();
}

ScanResult UltrasonicScanner::scanLeftCenterRight()
{
    ScanResult result;

    m_servoDriver.writeAngle(SERVO_LEFT);
    delay(SERVO_SETTLE_MS);
    result.leftMm = m_ultrasonicDriver.readDistanceMm();

    m_servoDriver.writeAngle(SERVO_CENTER);
    delay(SERVO_SETTLE_MS);
    result.centerMm = m_ultrasonicDriver.readDistanceMm();

    m_servoDriver.writeAngle(SERVO_RIGHT);
    delay(SERVO_SETTLE_MS);
    result.rightMm = m_ultrasonicDriver.readDistanceMm();

    m_servoDriver.writeAngle(SERVO_CENTER);
    delay(SERVO_SETTLE_MS);

    return result;
}