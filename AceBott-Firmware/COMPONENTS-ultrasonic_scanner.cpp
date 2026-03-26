#include "COMPONENTS-ultrasonic_scanner.h"

void UltrasonicScanner::init(int trigPin, int echoPin, int servoPin)
{
    m_ultrasonicDriver.init(trigPin, echoPin);
}

float UltrasonicScanner::readDistanceMm()
{
    return m_ultrasonicDriver.readDistanceMm();
}
