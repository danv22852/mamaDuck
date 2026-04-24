#ifndef ROBOTIC_ARM_H
#define ROBOTIC_ARM_H

#include <Arduino.h>
#include "arm_driver.h"
#include "types.h"

class RoboticArm
{
private:
    ArmDriver m_armDriver;
    bool m_initialized;

    unsigned long m_settleMs;
    unsigned long m_clawSettleMs;

public:
    RoboticArm();

    void init(
        int chassisPin,
        int shoulderPin,
        int elbowPin,
        int wristPin,
        int clawPin
    );

    bool isInitialized() const;
    void home();

    ArmPickResult pickByAnglesWithSpeed(
        float pickupChassisAngleDeg,
        float pickupShoulderAngleDeg,
        float pickupElbowAngleDeg,
        float pickupWristAngleDeg,
        float openClawAngleDeg,
        float closeClawAngleDeg,
        float liftChassisAngleDeg,
        float liftShoulderAngleDeg,
        float liftElbowAngleDeg,
        float liftWristAngleDeg,
        const ArmServoSpeed& pickupSpeed,
        const ArmServoSpeed& liftSpeed,
        const ArmMotionOptions& pickupOptions,
        const ArmMotionOptions& liftOptions
    );

    ArmDropResult dropByAnglesWithSpeed(
        float dropChassisAngleDeg,
        float dropShoulderAngleDeg,
        float dropElbowAngleDeg,
        float dropWristAngleDeg,
        float openClawAngleDeg,
        float retreatChassisAngleDeg,
        float retreatShoulderAngleDeg,
        float retreatElbowAngleDeg,
        float retreatWristAngleDeg,
        const ArmServoSpeed& dropSpeed,
        const ArmServoSpeed& retreatSpeed,
        const ArmMotionOptions& dropOptions,
        const ArmMotionOptions& retreatOptions
    );

    ArmResult moveByAnglesWithSpeed(
        float chassisAngleDeg,
        float shoulderAngleDeg,
        float elbowAngleDeg,
        float wristAngleDeg,
        float clawAngleDeg,
        const ArmServoSpeed& speedProfile,
        const ArmMotionOptions& moveOptions
    );

private:
    void armRestPosition();
    bool validateServoAngle(float angleDeg) const;
    ArmResult waitCancelable(unsigned long waitMs);
};

#endif