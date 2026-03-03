#include <ESP32Servo.h>
#include <ultrasonic.h>
#include <vehicle.h>

Servo myServo; //init the stuff
vehicle myCar;
ultrasonic myUltrasonic;

int myServo_PIN = 25;
int UT_distance = 0;
int LRDistance[2] = {0, 0};

/* -------- Servo Variables -------- */
int servoAngle = 0;
bool servoForward = true;
unsigned long previousServoMillis = 0;
unsigned long servoInterval = 15; // speed of sweep

/* -------- Ultrasonic Variables -------- */
unsigned long previousUltraMillis = 0;
unsigned long ultraInterval = 10; // check every 100ms

/* -------- Car State -------- */
enum CarState { MOVING_FORWARD, BACKING_UP, TURNING}; //
CarState carState = MOVING_FORWARD;
unsigned long actionStartTime = 0;

void setup() {
Serial.begin(9600);
myCar.Init();
myCar.Move(Stop, 0);
myUltrasonic.Init(13,14);
m
myServo.attach(myServo_PIN, 500, 2500);
myServo.write(0);

}

void loop() {
unsigned long currentMillis = millis();

if (currentMillis - previousServoMillis >= servoInterval) {
  previousServoMillis = currentMillis;
  if (servoForward)
    servoAngle++;
  else
    servoAngle--;
  if (servoAngle >= 180) 
    servoForward = false;
  if (servoAngle <= 0) 
    servoForward = true;
  myServo.write(servoAngle);
}
}

// if (currentMillis - previousUltraMillis >= ultraInterval) {
//   previousUltraMillis = currentMillis;
//   UT_distance = myUltrasonic.Ranging();
//   Serial.print(UT_distance);
//   Serial.println(" cm");
//     if(servoAngle == 135){ //store right distance
//       LRDistance[0] = UT_distance;
//     }
//     if(servoAngle == 45){ //store left distance 
//       LRDistance[1] = UT_distance;
//     }

//   if (UT_distance > 0 && UT_distance < 20 && carState == MOVING_FORWARD) {
//     carState = BACKING_UP;
//     actionStartTime = currentMillis;
//     myCar.Move(Backward, 150);
//     }
//   }

// switch (carState) {
//   case MOVING_FORWARD:
//   myCar.Move(Forward, 150);
//   break;
// case BACKING_UP:
//   if (currentMillis - actionStartTime >= 1000) {
//     if(LRDistance[0] > LRDistance[1]){
//       carState = TURNING;
//        myCar.Move(Clockwise, 150);
//     }
//     else {carState = TURNING;
//        myCar.Move(Contrarotate, 150);
//     }
//   actionStartTime = currentMillis;
// }
// break;
// case TURNING:
// if (currentMillis - actionStartTime >= 500) {
// carState = MOVING_FORWARD;
// myCar.Move(Forward, 150);
// }
// break;
// }
//}