#ifndef ACB_CAR_ARM_H
#define ACB_CAR_ARM_H

#include <Arduino.h>

struct ServoState {
  int pos1;
  int pos2;
  int pos3;
  int pos4;
  int pos5;
};

class ACB_CAR_ARM 
{
public:
  ACB_CAR_ARM();

  static int val;
  static int Chassis_Silde_Angle;
  static int Shoulder_Silde_Angle;
  static int Elbow_Silde_Angle;
  static int Wrist_Silde_Angle;
  static int Claws_Silde_Angle;

  static int PTP_X;
  static int PTP_Y;
  static int PTP_Z;

  static int mode;

  static int stateCount1;
  static int stateCount2;
  static int stateCount3;
  static int stateCount4;
  static int stateCount5;
  static int stateCount6;

  void ARM_init(int servo1, int servo2, int servo3, int servo4, int servo5);

  void limitZ(float pos__2, float pos__3);
  
  void ClawsCmd(int poss);
  void ElbowCmd(int poss);
  void ShoulderCmd(int poss);
  void ChassisCmd(int poss);
  void WristCmd(int poss);

  void Speed(int speeds);

  void PtpCmd (float x, float y, float z);

  void Zero();
  void Chassis_angle_adjust(float chassis_pos);
  void Slight_adjust(float right_pos,float left_pos);

  void saveState();
  void executeStates();
  void clearSavedStates();

  void getPositon();


  void startWebServer();
  void startAppServer();

  void Silde_ChassisCmd(float chassis_angle);
  void Silde_ShoulderCmd(float shoulder_angle);
  void Silde_ElbowCmd(float elbow_angle);
  void Silde_WristCmd(float wrist_angle);
  void Silde_ClawsCmd(float claws_angle);

  

  int chassis_angle = 90, shoulder_angle = 40, elbow_angle = 50, claws_angle = 90, wrist_angle = 90;  // define the variable of 4 servo angle,and assign the initial value (that is the boot posture
  //angle value)
  int middleDistance = 0;
  const int PWMRES_Min = 0; // PWM Resolution 0
  const int PWMRES_Max = 180; // PWM Resolution 180
  const int SERVOMIN = 400; // 400
  const int SERVOMAX = 2400; // 2400
  unsigned long lastMovementTime = 0;
  const unsigned long printDelay = 3000; // 3 seconds

  // Define arm length (in inches) and base position
  const float arm1_length = 11;  // Length of arm 1
  const float arm2_length = 7.5;  // Lexngth of arm 2 
  const float arm3_length = 17.5;  // Length of arm 3

  float limit_z;
  float servo_angle1,servo_angle2,servo_angle3,servo_angle4;

  int number = 10;//Record the number of actions

  int maxStates = 20;              // 去除 const 修饰
  int stateCount = 0;
  // int stateCount1 = 0;
  // int stateCount2 = 0;
  // int stateCount3 = 0;
  // int stateCount4 = 0;
  // int stateCount5 = 0;
  // int stateCount6 = 0;
  int currentState1 = 0;
  int currentState2 = 0;
  int currentState3 = 0;
  int currentState4 = 0;
  int currentState5 = 0;
  int currentState6 = 0;
  int currentState = 0;
  
  ServoState states[20];
  ServoState states1[20];
  ServoState states2[20];
  ServoState states3[20];
  ServoState states4[20];
  ServoState states5[20];
  ServoState states6[20];

  float chassis_pos = 0;
  float right_poss;
  float left_poss;

  int speed = 20;

  // int mode = 1;
  int chassis_pos2 = 0;
  int WuchaPos = 0;
  

private:
  
};

#endif // ACB_CAR_ARM_H