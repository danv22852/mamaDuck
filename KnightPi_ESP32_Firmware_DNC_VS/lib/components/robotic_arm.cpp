#include "robotic_arm.h"
#include <math.h>

RoboticArm::RoboticArm()
    : m_initialized(false),
      m_openClawAngleDeg(150),
      m_closedClawAngleDeg(90),
      m_wristApproachAngleDeg(90),
      m_wristPickupAngleDeg(90),
      m_approachLiftMm(45.0f),
      m_postGripLiftMm(70.0f),
      m_minDistanceMm(60.0f),
      m_maxDistanceMm(250.0f),
      m_minHeightMm(0.0f),
      m_maxHeightMm(180.0f),
      m_settleMs(600),
      m_clawSettleMs(700)
{
}

void RoboticArm::init(
    int chassisPin,
    int shoulderPin,
    int elbowPin,
    int wristPin,
    int clawPin
)
{
    m_armDriver.init(
        chassisPin,
        shoulderPin,
        elbowPin,
        wristPin,
        clawPin,
        5,
        0,
        0,
        50
    );

    ArmRestPosition();
    
    m_initialized = true;
}

void RoboticArm::ArmCheck()
{
    // CHASIS Check 
    m_armDriver.setChassisAngle(30);
    delay(300);
    m_armDriver.setChassisAngle(150);
    delay(300);
    m_armDriver.setChassisAngle(88);
    delay(300);

    //SHOULDER Check
    m_armDriver.setElbowAngle(90);  // avoid collisions
    delay(300);
    m_armDriver.setShoulderAngle(35);
    delay(300);
    m_armDriver.setShoulderAngle(90);
    delay(300);
    m_armDriver.setShoulderAngle(120);
    delay(300);
    m_armDriver.setShoulderAngle(35);
    delay(300);

    // ELBOW Check
    m_armDriver.setShoulderAngle(90);
    delay(300);
    m_armDriver.setElbowAngle(35);
    delay(300);
    m_armDriver.setElbowAngle(90);
    delay(300);
    m_armDriver.setElbowAngle(120);
    delay(300);
    m_armDriver.setElbowAngle(160);
    delay(300);
    m_armDriver.setElbowAngle(35);
    delay(300);
    m_armDriver.setShoulderAngle(35);
    delay(300);

    // WRIST Check
    m_armDriver.setWristAngle(100);
    delay(300);
    m_armDriver.setWristAngle(120);
    delay(300);
    m_armDriver.setWristAngle(160);
    delay(300);
    m_armDriver.setWristAngle(120);
    delay(300);
    m_armDriver.setWristAngle(60);
    delay(300);
    m_armDriver.setWristAngle(30);
    delay(300);
    m_armDriver.setWristAngle(60);
    delay(300);
    m_armDriver.setWristAngle(100);
    delay(300);

    // CLAW Check
    m_armDriver.setClawAngle(100);
    delay(300);
    m_armDriver.setClawAngle(130);
    delay(300);
    m_armDriver.setClawAngle(150);
    delay(300);
    m_armDriver.setClawAngle(170);
    delay(300);
    m_armDriver.setClawAngle(90);
    delay(300);
    m_armDriver.setClawAngle(100);
    delay(300);
}

void RoboticArm::ArmRestPosition()
{
    m_armDriver.setChassisAngle(88);
    delay(300);

    m_armDriver.setShoulderAngle(35);
    delay(300);

    m_armDriver.setElbowAngle(35);
    delay(300);

    m_armDriver.setWristAngle(100);
    delay(300);

    m_armDriver.setClawAngle(100);
    delay(300);
}

bool RoboticArm::isInitialized() const
{
    return m_initialized && m_armDriver.isInitialized();
}

void RoboticArm::home()
{
    if (!isInitialized()) return;
    m_armDriver.home();
}

bool RoboticArm::validateServoAngle(float angleDeg) const
{
    return angleDeg >= 0.0f && angleDeg <= 180.0f;
}

ArmPickResult RoboticArm::pickByAngles(
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
)
{
    ArmPickResult pickResult;

    if (!isInitialized())
    {
        pickResult.result = ARM_RESULT_NOT_INITIALIZED;
        return pickResult;
    }

    if (!validateServoAngle(pickupChassisAngleDeg) ||
        !validateServoAngle(pickupShoulderAngleDeg) ||
        !validateServoAngle(pickupElbowAngleDeg) ||
        !validateServoAngle(pickupWristAngleDeg) ||
        !validateServoAngle(openClawAngleDeg) ||
        !validateServoAngle(closeClawAngleDeg) ||
        !validateServoAngle(liftChassisAngleDeg) ||
        !validateServoAngle(liftShoulderAngleDeg) ||
        !validateServoAngle(liftElbowAngleDeg) ||
        !validateServoAngle(liftWristAngleDeg))
    {
        pickResult.result = ARM_RESULT_INVALID_ARGUMENT;
        return pickResult;
    }

    ArmPoseAngles pickupPose;
    pickupPose.useChassis = true;
    pickupPose.useShoulder = true;
    pickupPose.useElbow = true;
    pickupPose.useWrist = true;
    pickupPose.useClaw = true;

    pickupPose.chassisDeg = (int)roundf(pickupChassisAngleDeg);
    pickupPose.shoulderDeg = (int)roundf(pickupShoulderAngleDeg);
    pickupPose.elbowDeg = (int)roundf(pickupElbowAngleDeg);
    pickupPose.wristDeg = (int)roundf(pickupWristAngleDeg);
    pickupPose.clawDeg = (int)roundf(openClawAngleDeg);

    ArmResult result = m_armDriver.movePose(pickupPose, stepDelay, pickupOptions);
    if (result != ARM_RESULT_OK)
    {
        pickResult.result = result;
        return pickResult;
    }

    delay(m_settleMs);

    ArmPoseAngles closePose;
    closePose.useClaw = true;
    closePose.clawDeg = (int)roundf(closeClawAngleDeg);

    ArmMotionOptions clawOnlyOptions;
    clawOnlyOptions.mode = ARM_MOTION_ORDERED;
    clawOnlyOptions.servoOrderCount = 1;
    clawOnlyOptions.servoOrder[0] = ARM_SERVO_CLAW;

    result = m_armDriver.movePose(closePose, stepDelay, clawOnlyOptions);
    if (result != ARM_RESULT_OK)
    {
        pickResult.result = result;
        return pickResult;
    }

    delay(m_clawSettleMs);

    ArmPoseAngles liftPose;
    liftPose.useChassis = true;
    liftPose.useShoulder = true;
    liftPose.useElbow = true;
    liftPose.useWrist = true;
    liftPose.useClaw = false;

    liftPose.chassisDeg = (int)roundf(liftChassisAngleDeg);
    liftPose.shoulderDeg = (int)roundf(liftShoulderAngleDeg);
    liftPose.elbowDeg = (int)roundf(liftElbowAngleDeg);
    liftPose.wristDeg = (int)roundf(liftWristAngleDeg);

    result = m_armDriver.movePose(liftPose, stepDelay, liftOptions);
    if (result != ARM_RESULT_OK)
    {
        pickResult.result = result;
        return pickResult;
    }

    delay(m_settleMs);

    pickResult.result = ARM_RESULT_OK;
    pickResult.picked = true;
    pickResult.transportedToSecurePose = true;
    return pickResult;
}

ArmDropResult RoboticArm::dropByAngles(
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
)
{
    ArmDropResult dropResult;

    if (!isInitialized())
    {
        dropResult.result = ARM_RESULT_NOT_INITIALIZED;
        return dropResult;
    }

    if (!validateServoAngle(dropChassisAngleDeg) ||
        !validateServoAngle(dropShoulderAngleDeg) ||
        !validateServoAngle(dropElbowAngleDeg) ||
        !validateServoAngle(dropWristAngleDeg) ||
        !validateServoAngle(openClawAngleDeg) ||
        !validateServoAngle(retreatChassisAngleDeg) ||
        !validateServoAngle(retreatShoulderAngleDeg) ||
        !validateServoAngle(retreatElbowAngleDeg) ||
        !validateServoAngle(retreatWristAngleDeg))
    {
        dropResult.result = ARM_RESULT_INVALID_ARGUMENT;
        return dropResult;
    }

    ArmPoseAngles dropPose;
    dropPose.useChassis = true;
    dropPose.useShoulder = true;
    dropPose.useElbow = true;
    dropPose.useWrist = true;
    dropPose.useClaw = false;

    dropPose.chassisDeg = (int)roundf(dropChassisAngleDeg);
    dropPose.shoulderDeg = (int)roundf(dropShoulderAngleDeg);
    dropPose.elbowDeg = (int)roundf(dropElbowAngleDeg);
    dropPose.wristDeg = (int)roundf(dropWristAngleDeg);

    ArmResult result = m_armDriver.movePose(dropPose, stepDelay, dropOptions);
    if (result != ARM_RESULT_OK)
    {
        dropResult.result = result;
        return dropResult;
    }

    delay(m_settleMs);

    ArmPoseAngles openPose;
    openPose.useClaw = true;
    openPose.clawDeg = (int)roundf(openClawAngleDeg);

    ArmMotionOptions clawOnlyOptions;
    clawOnlyOptions.mode = ARM_MOTION_ORDERED;
    clawOnlyOptions.servoOrderCount = 1;
    clawOnlyOptions.servoOrder[0] = ARM_SERVO_CLAW;

    result = m_armDriver.movePose(openPose, stepDelay, clawOnlyOptions);
    if (result != ARM_RESULT_OK)
    {
        dropResult.result = result;
        return dropResult;
    }

    delay(m_clawSettleMs);
    dropResult.dropped = true;

    ArmPoseAngles retreatPose;
    retreatPose.useChassis = true;
    retreatPose.useShoulder = true;
    retreatPose.useElbow = true;
    retreatPose.useWrist = true;
    retreatPose.useClaw = false;

    retreatPose.chassisDeg = (int)roundf(retreatChassisAngleDeg);
    retreatPose.shoulderDeg = (int)roundf(retreatShoulderAngleDeg);
    retreatPose.elbowDeg = (int)roundf(retreatElbowAngleDeg);
    retreatPose.wristDeg = (int)roundf(retreatWristAngleDeg);

    result = m_armDriver.movePose(retreatPose, stepDelay, retreatOptions);
    if (result != ARM_RESULT_OK)
    {
        dropResult.result = result;
        return dropResult;
    }

    delay(m_settleMs);

    dropResult.result = ARM_RESULT_OK;
    dropResult.retreatedToSafePose = true;
    return dropResult;
}

ArmResult RoboticArm::moveByAngles(
    float chassisAngleDeg,
    float shoulderAngleDeg,
    float elbowAngleDeg,
    float wristAngleDeg,
    float clawAngleDeg,
    const ArmServoStepDelay& stepDelay,
    const ArmMotionOptions& moveOptions
)
{
    if (!isInitialized()) return ARM_RESULT_NOT_INITIALIZED;

    if (!validateServoAngle(chassisAngleDeg) ||
        !validateServoAngle(shoulderAngleDeg) ||
        !validateServoAngle(elbowAngleDeg) ||
        !validateServoAngle(wristAngleDeg) ||
        !validateServoAngle(clawAngleDeg))
    {
        return ARM_RESULT_INVALID_ARGUMENT;
    }

    ArmPoseAngles pose;
    pose.useChassis = true;
    pose.useShoulder = true;
    pose.useElbow = true;
    pose.useWrist = true;
    pose.useClaw = true;

    pose.chassisDeg = (int)roundf(chassisAngleDeg);
    pose.shoulderDeg = (int)roundf(shoulderAngleDeg);
    pose.elbowDeg = (int)roundf(elbowAngleDeg);
    pose.wristDeg = (int)roundf(wristAngleDeg);
    pose.clawDeg = (int)roundf(clawAngleDeg);

    ArmResult result = m_armDriver.movePose(pose, stepDelay, moveOptions);
    if (result != ARM_RESULT_OK)
    {
        return result;
    }

    delay(m_settleMs);
    return ARM_RESULT_OK;
}

ArmResult RoboticArm::pickObject(
    float chassisAngleDeg,
    float distanceFromChassisCenterMm,
    float pickupHeightFromFloorMm
)
{
    if (!isInitialized()) return ARM_RESULT_NOT_INITIALIZED;

    if (!validatePickRequest(chassisAngleDeg, distanceFromChassisCenterMm, pickupHeightFromFloorMm))
    {
        return ARM_RESULT_INVALID_ARGUMENT;
    }

    ArmPointMm target = polarToCartesianMm(
        chassisAngleDeg,
        distanceFromChassisCenterMm,
        pickupHeightFromFloorMm
    );

    ArmPointMm approach = target;
    approach.zMm += m_approachLiftMm;

    ArmPointMm retreat = target;
    retreat.zMm += m_postGripLiftMm;

    m_armDriver.setWristAngle(m_wristApproachAngleDeg);
    delay(250);

    m_armDriver.setClawAngle(m_openClawAngleDeg);
    delay(m_clawSettleMs);

    ArmResult result = moveAndWait(approach, m_settleMs);
    if (result != ARM_RESULT_OK)
    {
        return result;
    }

    m_armDriver.setWristAngle(m_wristPickupAngleDeg);
    delay(250);

    result = moveAndWait(target, m_settleMs);
    if (result != ARM_RESULT_OK)
    {
        return result;
    }

    m_armDriver.setClawAngle(m_closedClawAngleDeg);
    delay(m_clawSettleMs);

    result = moveAndWait(retreat, m_settleMs);
    if (result != ARM_RESULT_OK)
    {
        return result;
    }

    return ARM_RESULT_OK;
}

bool RoboticArm::validatePickRequest(
    float chassisAngleDeg,
    float distanceMm,
    float heightMm
) const
{
    (void)chassisAngleDeg;

    if (!isInitialized()) return false;
  
    if (distanceMm < m_minDistanceMm || distanceMm > m_maxDistanceMm)
    {
        return false;
    }

    if (heightMm < m_minHeightMm || heightMm > m_maxHeightMm)
    {
        return false;
    }

    return true;
}

ArmPointMm RoboticArm::polarToCartesianMm(
    float chassisAngleDeg,
    float distanceMm,
    float zMm
) const
{
    float thetaRad = chassisAngleDeg * DEG_TO_RAD;

    ArmPointMm point;
    point.xMm = distanceMm * sinf(thetaRad);
    point.yMm = distanceMm * cosf(thetaRad);
    point.zMm = zMm;

    return point;
}

ArmResult RoboticArm::moveAndWait(
    const ArmPointMm& pointMm,
    unsigned long waitMs
)
{
    ArmResult result = m_armDriver.moveToCartesianMm(pointMm);
    if (result != ARM_RESULT_OK)
    {
        return result;
    }

    delay(waitMs);
    return ARM_RESULT_OK;
}