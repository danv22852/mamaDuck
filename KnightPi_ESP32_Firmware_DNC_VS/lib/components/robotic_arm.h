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

    int m_openClawAngleDeg;
    int m_closedClawAngleDeg;
    int m_wristApproachAngleDeg;
    int m_wristPickupAngleDeg;

    float m_approachLiftMm;
    float m_postGripLiftMm;

    float m_minDistanceMm;
    float m_maxDistanceMm;
    float m_minHeightMm;
    float m_maxHeightMm;

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

    ArmResult pickObject(
        float chassisAngleDeg,
        float distanceFromChassisCenterMm,
        float pickupHeightFromFloorMm
    );

    ArmPickResult pickByAngles(
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
        const ArmServoStepDelay& stepDelay,
        const ArmMotionOptions& pickupOptions,
        const ArmMotionOptions& liftOptions
    );

    ArmDropResult dropByAngles(
        float dropChassisAngleDeg,
        float dropShoulderAngleDeg,
        float dropElbowAngleDeg,
        float dropWristAngleDeg,
        float openClawAngleDeg,
        float retreatChassisAngleDeg,
        float retreatShoulderAngleDeg,
        float retreatElbowAngleDeg,
        float retreatWristAngleDeg,
        const ArmServoStepDelay& stepDelay,
        const ArmMotionOptions& dropOptions,
        const ArmMotionOptions& retreatOptions
    );

    ArmResult moveByAngles(
        float chassisAngleDeg,
        float shoulderAngleDeg,
        float elbowAngleDeg,
        float wristAngleDeg,
        float clawAngleDeg,
        const ArmServoStepDelay& stepDelay,
        const ArmMotionOptions& moveOptions
    );

private:
    void ArmCheck();
    void ArmRestPosition();

    bool validatePickRequest(
        float chassisAngleDeg,
        float distanceMm,
        float heightMm
    ) const;

    bool validateServoAngle(float angleDeg) const;

    ArmPointMm polarToCartesianMm(
        float chassisAngleDeg,
        float distanceMm,
        float zMm
    ) const;

    ArmResult moveAndWait(
        const ArmPointMm& pointMm,
        unsigned long waitMs
    );
};

#endif