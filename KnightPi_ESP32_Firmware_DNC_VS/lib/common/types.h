#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include "command_ids.h"
#include "config.h"

enum ScanRotation
{
    SCAN_CCW = 0,
    SCAN_CW
};

enum ArmServoId
{
    ARM_SERVO_CHASSIS = 0,
    ARM_SERVO_SHOULDER,
    ARM_SERVO_ELBOW,
    ARM_SERVO_WRIST,
    ARM_SERVO_CLAW,
    ARM_SERVO_COUNT
};

enum ArmMotionMode
{
    ARM_MOTION_ORDERED = 0,
    ARM_MOTION_COMBINED
};

struct ArmServoStepDelay
{
    unsigned long chassisMs;
    unsigned long shoulderMs;
    unsigned long elbowMs;
    unsigned long wristMs;
    unsigned long clawMs;

    ArmServoStepDelay()
        : chassisMs(15),
          shoulderMs(15),
          elbowMs(15),
          wristMs(15),
          clawMs(15)
    {
    }
};

struct ArmPoseAngles
{
    bool useChassis;
    bool useShoulder;
    bool useElbow;
    bool useWrist;
    bool useClaw;

    int chassisDeg;
    int shoulderDeg;
    int elbowDeg;
    int wristDeg;
    int clawDeg;

    ArmPoseAngles()
        : useChassis(false),
          useShoulder(false),
          useElbow(false),
          useWrist(false),
          useClaw(false),
          chassisDeg(0),
          shoulderDeg(0),
          elbowDeg(0),
          wristDeg(0),
          clawDeg(0)
    {
    }
};

struct ArmMotionOptions
{
    ArmMotionMode mode;
    int servoOrderCount;
    ArmServoId servoOrder[ARM_SERVO_COUNT];

    ArmMotionOptions()
        : mode(ARM_MOTION_ORDERED),
          servoOrderCount(0)
    {
    }
};

struct Command
{
    CommandType type;
    String originalText;

    // command envelope
    String transactionId;
    bool expectsJsonResponse;

    // existing cartesian arm command
    float pickAngleDeg;
    float pickDistanceMm;
    float pickHeightMm;

    // PICK_ANGLES - pickup pose
    float armPickupChassisAngleDeg;
    float armPickupShoulderAngleDeg;
    float armPickupElbowAngleDeg;
    float armPickupWristAngleDeg;
    float armOpenClawAngleDeg;
    float armCloseClawAngleDeg;

    // PICK_ANGLES - lift pose
    float armLiftChassisAngleDeg;
    float armLiftShoulderAngleDeg;
    float armLiftElbowAngleDeg;
    float armLiftWristAngleDeg;

    // PICK_ANGLES - motion
    ArmServoStepDelay armStepDelay;

    ArmMotionMode armPickupMotionMode;
    int armPickupServoOrderCount;
    ArmServoId armPickupServoOrder[ARM_SERVO_COUNT];

    ArmMotionMode armLiftMotionMode;
    int armLiftServoOrderCount;
    ArmServoId armLiftServoOrder[ARM_SERVO_COUNT];

    // DROP_ANGLES - drop pose
    float armDropChassisAngleDeg;
    float armDropShoulderAngleDeg;
    float armDropElbowAngleDeg;
    float armDropWristAngleDeg;
    float armDropOpenClawAngleDeg;

    // DROP_ANGLES - retreat pose
    float armRetreatChassisAngleDeg;
    float armRetreatShoulderAngleDeg;
    float armRetreatElbowAngleDeg;
    float armRetreatWristAngleDeg;

    // DROP_ANGLES - motion
    ArmMotionMode armDropMotionMode;
    int armDropServoOrderCount;
    ArmServoId armDropServoOrder[ARM_SERVO_COUNT];

    ArmMotionMode armRetreatMotionMode;
    int armRetreatServoOrderCount;
    ArmServoId armRetreatServoOrder[ARM_SERVO_COUNT];

    // MOVE_ARM_ANGLES - pose
    float armMoveChassisAngleDeg;
    float armMoveShoulderAngleDeg;
    float armMoveElbowAngleDeg;
    float armMoveWristAngleDeg;
    float armMoveClawAngleDeg;

    // MOVE_ARM_ANGLES - motion
    ArmMotionMode armMoveMotionMode;
    int armMoveServoOrderCount;
    ArmServoId armMoveServoOrder[ARM_SERVO_COUNT];

    // move commands
    int moveAmountMs;
    int moveSpeed;

    // robot rotation ultrasonic scan
    float usScanAngleDeg;
    int usScanSteps;
    unsigned long usScanStepRotateMs;
    int usScanRotateSpeed;
    ScanRotation usScanRotation;

    Command()
        : type(CMD_UNKNOWN),
          originalText(""),
          transactionId(""),
          expectsJsonResponse(false),

          pickAngleDeg(0.0f),
          pickDistanceMm(0.0f),
          pickHeightMm(0.0f),

          armPickupChassisAngleDeg(0.0f),
          armPickupShoulderAngleDeg(0.0f),
          armPickupElbowAngleDeg(0.0f),
          armPickupWristAngleDeg(0.0f),
          armOpenClawAngleDeg(0.0f),
          armCloseClawAngleDeg(0.0f),

          armLiftChassisAngleDeg(0.0f),
          armLiftShoulderAngleDeg(0.0f),
          armLiftElbowAngleDeg(0.0f),
          armLiftWristAngleDeg(0.0f),

          armPickupMotionMode(ARM_MOTION_ORDERED),
          armPickupServoOrderCount(0),
          armLiftMotionMode(ARM_MOTION_ORDERED),
          armLiftServoOrderCount(0),

          armDropChassisAngleDeg(0.0f),
          armDropShoulderAngleDeg(0.0f),
          armDropElbowAngleDeg(0.0f),
          armDropWristAngleDeg(0.0f),
          armDropOpenClawAngleDeg(0.0f),

          armRetreatChassisAngleDeg(0.0f),
          armRetreatShoulderAngleDeg(0.0f),
          armRetreatElbowAngleDeg(0.0f),
          armRetreatWristAngleDeg(0.0f),

          armDropMotionMode(ARM_MOTION_ORDERED),
          armDropServoOrderCount(0),
          armRetreatMotionMode(ARM_MOTION_ORDERED),
          armRetreatServoOrderCount(0),

          armMoveChassisAngleDeg(0.0f),
          armMoveShoulderAngleDeg(0.0f),
          armMoveElbowAngleDeg(0.0f),
          armMoveWristAngleDeg(0.0f),
          armMoveClawAngleDeg(0.0f),

          armMoveMotionMode(ARM_MOTION_ORDERED),
          armMoveServoOrderCount(0),

          moveAmountMs(0),
          moveSpeed(50),

          usScanAngleDeg(0.0f),
          usScanSteps(0),
          usScanStepRotateMs(DEFAULT_US_SCAN_STEP_ROTATE_MS),
          usScanRotateSpeed(DEFAULT_US_SCAN_ROTATE_SPEED),
          usScanRotation(SCAN_CCW)
    {
        armPickupServoOrder[0] = ARM_SERVO_CLAW;
        armPickupServoOrder[1] = ARM_SERVO_CHASSIS;
        armPickupServoOrder[2] = ARM_SERVO_SHOULDER;
        armPickupServoOrder[3] = ARM_SERVO_ELBOW;
        armPickupServoOrder[4] = ARM_SERVO_WRIST;

        armLiftServoOrder[0] = ARM_SERVO_SHOULDER;
        armLiftServoOrder[1] = ARM_SERVO_ELBOW;
        armLiftServoOrder[2] = ARM_SERVO_WRIST;
        armLiftServoOrder[3] = ARM_SERVO_CHASSIS;
        armLiftServoOrder[4] = ARM_SERVO_CLAW;

        armDropServoOrder[0] = ARM_SERVO_CHASSIS;
        armDropServoOrder[1] = ARM_SERVO_SHOULDER;
        armDropServoOrder[2] = ARM_SERVO_ELBOW;
        armDropServoOrder[3] = ARM_SERVO_WRIST;
        armDropServoOrder[4] = ARM_SERVO_CLAW;

        armRetreatServoOrder[0] = ARM_SERVO_SHOULDER;
        armRetreatServoOrder[1] = ARM_SERVO_ELBOW;
        armRetreatServoOrder[2] = ARM_SERVO_WRIST;
        armRetreatServoOrder[3] = ARM_SERVO_CHASSIS;
        armRetreatServoOrder[4] = ARM_SERVO_CLAW;

        armMoveServoOrder[0] = ARM_SERVO_CHASSIS;
        armMoveServoOrder[1] = ARM_SERVO_SHOULDER;
        armMoveServoOrder[2] = ARM_SERVO_ELBOW;
        armMoveServoOrder[3] = ARM_SERVO_WRIST;
        armMoveServoOrder[4] = ARM_SERVO_CLAW;
    }
};

struct ScanResult
{
    float leftMm;
    float centerMm;
    float rightMm;
};

constexpr int MAX_US_SCAN_POINTS = 72;

struct UltrasonicScanPoint
{
    float angleDeg;
    float distanceMm;
};

struct UltrasonicSweepResult
{
    int pointCount;
    UltrasonicScanPoint points[MAX_US_SCAN_POINTS];

    UltrasonicSweepResult()
        : pointCount(0)
    {
    }
};

struct ErrorInfo
{
    String code;
    String message;
};

struct JsonResponseContext
{
    String transactionId;
    String command;
    bool ok;

    JsonResponseContext()
        : transactionId(""),
          command(""),
          ok(true)
    {
    }
};

struct ArmPointMm
{
    float xMm;
    float yMm;
    float zMm;
};

enum ArmResult
{
    ARM_RESULT_OK = 0,
    ARM_RESULT_NOT_INITIALIZED,
    ARM_RESULT_INVALID_ARGUMENT,
    ARM_RESULT_OUT_OF_REACH
};

struct ArmPickResult
{
    ArmResult result;
    bool picked;
    bool transportedToSecurePose;

    ArmPickResult()
        : result(ARM_RESULT_OK),
          picked(false),
          transportedToSecurePose(false)
    {
    }
};

struct ArmDropResult
{
    ArmResult result;
    bool dropped;
    bool retreatedToSafePose;

    ArmDropResult()
        : result(ARM_RESULT_OK),
          dropped(false),
          retreatedToSafePose(false)
    {
    }
};

#endif