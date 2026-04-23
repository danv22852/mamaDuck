#include <ESP32Servo.h>
#include <vehicle.h>

Servo chassisServo;
Servo shoulderServo;
Servo elbowServo;
Servo wristServo;
Servo clawServo;

vehicle myCar;

void cruisePosition();
void pickObject();
void moveServoSlow(Servo& servo, int currentAngle, int targetAngle, int stepDelayMs);

void setup()
{
    chassisServo.attach(25);
    shoulderServo.attach(26);
    elbowServo.attach(27);
    wristServo.attach(33);
    clawServo.attach(4);

    myCar.Init();
    delay(1000);

    // CHASSIS
    chassisServo.write(45);
    delay(300);
    chassisServo.write(135);
    delay(300);
    chassisServo.write(82);
    delay(300);

    // SHOULDER
    shoulderServo.write(90);
    delay(300);
    shoulderServo.write(35);
    delay(300);

    // ELBOW
    elbowServo.write(90);
    delay(300);
    elbowServo.write(35);
    delay(300);

    // WRIST
    wristServo.write(45);
    delay(300);
    wristServo.write(135);
    delay(300);
    wristServo.write(95);
    delay(300);

    // CLAW
    clawServo.write(150);
    delay(300);
    clawServo.write(90);
    delay(300);

    // for(int i=0; i<3; i++)
    // {
    //     shoulderServo.write(0);
    //     delay(500);
    //     shoulderServo.write(45);
    //     delay(500);
    //     shoulderServo.write(90);
    //     delay(500);
    //     shoulderServo.write(135);
    //     delay(500);
    //     shoulderServo.write(180);
    //     delay(500);
    //     shoulderServo.write(90);
    //     delay(500);
    //     shoulderServo.write(0);
    //     delay(500);       
    // }    
}

void loop()
{
}




void CalibrationPorition()
{
    chassisServo.write(90);
    shoulderServo.write(90);
    elbowServo.write(90);
    wristServo.write(90);
    clawServo.write(90);
}

void cruisePosition()
{
    chassisServo.write(82);
    shoulderServo.write(45);
    elbowServo.write(45);

    wristServo.write(180);
    delay(300);
    wristServo.write(0);
    delay(300);
    wristServo.write(90);
    delay(300);

    clawServo.write(120);
    delay(300);
}

void pickObject()
{
    // Pickup
    // 1. pickup position
    clawServo.write(160);
    delay(300);
    shoulderServo.write(120);
    delay(300);
    shoulderServo.write(160);
    delay(300);
    elbowServo.write(100);
    delay(300);
    elbowServo.write(120);
    delay(2000);

    // 2. grab
    clawServo.write(90);
    delay(300);

    // 3. secure claw
    wristServo.write(0);
    delay(300);

    // 4. secure position
    shoulderServo.write(45);
    delay(300);
    elbowServo.write(45);

    // delay
    delay(3000);

        // // pickup slowly
    // int currentAngle = 90;
    // int targetAngle = 150;
    // for (int angle = currentAngle; angle <= targetAngle; angle++)
    // {
    //    clawServo.write(angle);
    //    delay(20);
    // }
}

void dropObject()
{
    // drop
    // 1. drop position
    chassisServo.write(30);
    delay(300);
    shoulderServo.write(160);
    delay(300);
    elbowServo.write(120);
    delay(300);
    wristServo.write(95);
    delay(300);

    // 2. release
    clawServo.write(160);
    delay(300);

    // 3. rest position
    chassisServo.write(82);
    delay(300);
    shoulderServo.write(45);
    delay(300);
    elbowServo.write(45);
    delay(300);
    clawServo.write(90);
}

void moveServoSlow(Servo& servo, int currentAngle, int targetAngle, int stepDelayMs)
{
    if (targetAngle > currentAngle)
    {
        for (int angle = currentAngle; angle <= targetAngle; angle++)
        {
            servo.write(angle);
            delay(stepDelayMs);
        }
    }
    else
    {
        for (int angle = currentAngle; angle >= targetAngle; angle--)
        {
            servo.write(angle);
            delay(stepDelayMs);
        }
    }
}

