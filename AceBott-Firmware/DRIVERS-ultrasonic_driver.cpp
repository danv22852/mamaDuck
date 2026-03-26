#include "DRIVERS-ultrasonic_driver.h"

void UltrasonicDriver::init(int trigPin, int echoPin)
{
    m_ultrasonic.Init(trigPin, echoPin);
}

float UltrasonicDriver::readDistanceCm()
{
    return m_ultrasonic.Ranging();
}

float UltrasonicDriver::readDistanceMm()
{
    return m_ultrasonic.Ranging() * 10.0f;
}