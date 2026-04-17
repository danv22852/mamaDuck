#ifndef ARM_DRIVER_H
#define ARM_DRIVER_H

#include <Arduino.h>
#include <ACB_CAR_ARM.h>
#include "../common/types.h"

class ArmDriver
{
private:
    ACB_CAR_ARM m_arm;
    bool m_initialized;

    int m_lastChassisAngleDeg;
    int m_lastShoulderAngleDeg;
    int m_lastElbowAngleDeg;
    int m_lastWristAngleDeg;
    int m_lastClawAngleDeg;

public:
    ArmDriver();

    void init(
        int chassisPin,
        int shoulderPin,
        int elbowPin,
        int wristPin,
        int clawPin,
        int chassisAngleAdjustDeg = 5,
        int slightAdjustPositiveXDeg = 0,
        int slightAdjustNegativeXDeg = 0,
        int speedPercent = 50
    );

    bool isInitialized() const;

    void home();
    void setSpeed(int speedPercent);
    void setWristAngle(int angleDeg);
    void setClawAngle(int angleDeg);
    void setChassisAngle(int angleDeg);
    void setShoulderAngle(int angleDeg);
    void setElbowAngle(int angleDeg);

    ArmResult movePose(
        const ArmPoseAngles& targetPose,
        const ArmServoStepDelay& stepDelay,
        const ArmMotionOptions& options
    );

    ArmResult moveToCartesianMm(const ArmPointMm& pointMm);

private:
    bool isWithinWorkspaceCm(float xCm, float yCm, float zCm) const;
    bool isValidServoAngle(int angleDeg) const;
    void writeServoAngle(ArmServoId servoId, int angleDeg);
    int getLastServoAngle(ArmServoId servoId) const;
    int getTargetServoAngle(const ArmPoseAngles& pose, ArmServoId servoId) const;
    bool poseUsesServo(const ArmPoseAngles& pose, ArmServoId servoId) const;
    void moveServoSlow(ArmServoId servoId, int targetAngleDeg, unsigned long stepDelayMs);
};

#endif