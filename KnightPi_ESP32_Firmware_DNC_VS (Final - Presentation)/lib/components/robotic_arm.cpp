#include "robotic_arm.h"
#include "../common/runtime_state.h"
#include <math.h>

RoboticArm::RoboticArm()
    : m_initialized(false),
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

    armRestPosition();
    m_initialized = true;
}

void RoboticArm::armRestPosition()
{
    m_armDriver.setChassisAngle(88);
    vTaskDelay(pdMS_TO_TICKS(300));

    m_armDriver.setShoulderAngle(40);
    vTaskDelay(pdMS_TO_TICKS(300));

    m_armDriver.setElbowAngle(50);
    vTaskDelay(pdMS_TO_TICKS(300));

    m_armDriver.setWristAngle(100);
    vTaskDelay(pdMS_TO_TICKS(300));

    m_armDriver.setClawAngle(100);
    vTaskDelay(pdMS_TO_TICKS(300));
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

ArmResult RoboticArm::waitCancelable(unsigned long waitMs)
{
    unsigned long startedAt = millis();

    while ((millis() - startedAt) < waitMs)
    {
        if (RuntimeState::instance().shouldCancelArm()) return ARM_RESULT_CANCELLED;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return ARM_RESULT_OK;
}

ArmPickResult RoboticArm::pickByAnglesWithSpeed(
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

    ArmResult result = m_armDriver.movePoseBySpeed(pickupPose, pickupSpeed, pickupOptions);
    if (result != ARM_RESULT_OK)
    {
        pickResult.result = result;
        return pickResult;
    }

    result = waitCancelable(m_settleMs);
    if (result != ARM_RESULT_OK)
    {
        pickResult.result = result;
        return pickResult;
    }

    ArmPoseAngles closePose;
    closePose.useClaw = true;
    closePose.clawDeg = (int)roundf(closeClawAngleDeg);

    ArmMotionOptions clawOnlyOptions;
    clawOnlyOptions.mode = ARM_MOTION_ORDERED;
    clawOnlyOptions.servoOrderCount = 1;
    clawOnlyOptions.servoOrder[0] = ARM_SERVO_CLAW;

    result = m_armDriver.movePoseBySpeed(closePose, pickupSpeed, clawOnlyOptions);
    if (result != ARM_RESULT_OK)
    {
        pickResult.result = result;
        return pickResult;
    }

    result = waitCancelable(m_clawSettleMs);
    if (result != ARM_RESULT_OK)
    {
        pickResult.result = result;
        return pickResult;
    }

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

    result = m_armDriver.movePoseBySpeed(liftPose, liftSpeed, liftOptions);
    if (result != ARM_RESULT_OK)
    {
        pickResult.result = result;
        return pickResult;
    }

    result = waitCancelable(m_settleMs);
    if (result != ARM_RESULT_OK)
    {
        pickResult.result = result;
        return pickResult;
    }

    pickResult.result = ARM_RESULT_OK;
    pickResult.picked = true;
    pickResult.transportedToSecurePose = true;
    return pickResult;
}

ArmDropResult RoboticArm::dropByAnglesWithSpeed(
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

    ArmResult result = m_armDriver.movePoseBySpeed(dropPose, dropSpeed, dropOptions);
    if (result != ARM_RESULT_OK)
    {
        dropResult.result = result;
        return dropResult;
    }

    result = waitCancelable(m_settleMs);
    if (result != ARM_RESULT_OK)
    {
        dropResult.result = result;
        return dropResult;
    }

    ArmPoseAngles openPose;
    openPose.useClaw = true;
    openPose.clawDeg = (int)roundf(openClawAngleDeg);

    ArmMotionOptions clawOnlyOptions;
    clawOnlyOptions.mode = ARM_MOTION_ORDERED;
    clawOnlyOptions.servoOrderCount = 1;
    clawOnlyOptions.servoOrder[0] = ARM_SERVO_CLAW;

    result = m_armDriver.movePoseBySpeed(openPose, dropSpeed, clawOnlyOptions);
    if (result != ARM_RESULT_OK)
    {
        dropResult.result = result;
        return dropResult;
    }

    result = waitCancelable(m_clawSettleMs);
    if (result != ARM_RESULT_OK)
    {
        dropResult.result = result;
        return dropResult;
    }

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

    result = m_armDriver.movePoseBySpeed(retreatPose, retreatSpeed, retreatOptions);
    if (result != ARM_RESULT_OK)
    {
        dropResult.result = result;
        return dropResult;
    }

    result = waitCancelable(m_settleMs);
    if (result != ARM_RESULT_OK)
    {
        dropResult.result = result;
        return dropResult;
    }

    dropResult.result = ARM_RESULT_OK;
    dropResult.retreatedToSafePose = true;
    return dropResult;
}

ArmResult RoboticArm::moveByAnglesWithSpeed(
    float chassisAngleDeg,
    float shoulderAngleDeg,
    float elbowAngleDeg,
    float wristAngleDeg,
    float clawAngleDeg,
    const ArmServoSpeed& speedProfile,
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

    ArmResult result = m_armDriver.movePoseBySpeed(pose, speedProfile, moveOptions);
    if (result != ARM_RESULT_OK) return result;

    return waitCancelable(m_settleMs);
}