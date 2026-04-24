#include "arm_driver.h"
#include "../common/runtime_state.h"
#include "../common/config.h"

ArmDriver::ArmDriver()
    : m_initialized(false),
      m_lastChassisAngleDeg(88),
      m_lastShoulderAngleDeg(40),
      m_lastElbowAngleDeg(50),
      m_lastWristAngleDeg(90),
      m_lastClawAngleDeg(100)
{
}

void ArmDriver::init(
    int chassisPin,
    int shoulderPin,
    int elbowPin,
    int wristPin,
    int clawPin,
    int chassisAngleAdjustDeg,
    int slightAdjustPositiveXDeg,
    int slightAdjustNegativeXDeg,
    int speedPercent
)
{
    m_arm.Chassis_angle_adjust(chassisAngleAdjustDeg);
    m_arm.Slight_adjust(slightAdjustPositiveXDeg, slightAdjustNegativeXDeg);

    m_arm.ARM_init(chassisPin, shoulderPin, elbowPin, wristPin, clawPin);
    m_arm.Speed(sanitizeSpeedPercent(speedPercent));

    m_initialized = true;
}

bool ArmDriver::isInitialized() const
{
    return m_initialized;
}

void ArmDriver::home()
{
    if (!m_initialized) return;
    m_arm.Zero();
}

void ArmDriver::setSpeed(int speedPercent)
{
    if (!m_initialized) return;
    m_arm.Speed(sanitizeSpeedPercent(speedPercent));
}

void ArmDriver::setWristAngle(int angleDeg)
{
    if (!m_initialized) return;
    m_arm.WristCmd(angleDeg);
    m_lastWristAngleDeg = angleDeg;
}

void ArmDriver::setClawAngle(int angleDeg)
{
    if (!m_initialized) return;
    m_arm.ClawsCmd(angleDeg);
    m_lastClawAngleDeg = angleDeg;
}

void ArmDriver::setChassisAngle(int angleDeg)
{
    if (!m_initialized) return;
    m_arm.ChassisCmd(angleDeg);
    m_lastChassisAngleDeg = angleDeg;
}

void ArmDriver::setShoulderAngle(int angleDeg)
{
    if (!m_initialized) return;
    m_arm.ShoulderCmd(angleDeg);
    m_lastShoulderAngleDeg = angleDeg;
}

void ArmDriver::setElbowAngle(int angleDeg)
{
    if (!m_initialized) return;
    m_arm.ElbowCmd(angleDeg);
    m_lastElbowAngleDeg = angleDeg;
}

bool ArmDriver::isValidServoAngle(int angleDeg) const
{
    return angleDeg >= 0 && angleDeg <= 180;
}

int ArmDriver::sanitizeSpeedPercent(int speedPercent) const
{
    if (speedPercent < 1) return 1;
    if (speedPercent > 100) return 100;
    return speedPercent;
}

int ArmDriver::getServoSpeedPercent(const ArmServoSpeed& speedProfile, ArmServoId servoId) const
{
    switch (servoId)
    {
        case ARM_SERVO_CHASSIS: return sanitizeSpeedPercent(speedProfile.chassisPercent);
        case ARM_SERVO_SHOULDER: return sanitizeSpeedPercent(speedProfile.shoulderPercent);
        case ARM_SERVO_ELBOW: return sanitizeSpeedPercent(speedProfile.elbowPercent);
        case ARM_SERVO_WRIST: return sanitizeSpeedPercent(speedProfile.wristPercent);
        case ARM_SERVO_CLAW: return sanitizeSpeedPercent(speedProfile.clawPercent);
        default: return 50;
    }
}

void ArmDriver::writeServoAngle(ArmServoId servoId, int angleDeg)
{
    switch (servoId)
    {
        case ARM_SERVO_CHASSIS: setChassisAngle(angleDeg); break;
        case ARM_SERVO_SHOULDER: setShoulderAngle(angleDeg); break;
        case ARM_SERVO_ELBOW: setElbowAngle(angleDeg); break;
        case ARM_SERVO_WRIST: setWristAngle(angleDeg); break;
        case ARM_SERVO_CLAW: setClawAngle(angleDeg); break;
        default: break;
    }
}

int ArmDriver::getTargetServoAngle(const ArmPoseAngles& pose, ArmServoId servoId) const
{
    switch (servoId)
    {
        case ARM_SERVO_CHASSIS: return pose.chassisDeg;
        case ARM_SERVO_SHOULDER: return pose.shoulderDeg;
        case ARM_SERVO_ELBOW: return pose.elbowDeg;
        case ARM_SERVO_WRIST: return pose.wristDeg;
        case ARM_SERVO_CLAW: return pose.clawDeg;
        default: return 0;
    }
}

bool ArmDriver::poseUsesServo(const ArmPoseAngles& pose, ArmServoId servoId) const
{
    switch (servoId)
    {
        case ARM_SERVO_CHASSIS: return pose.useChassis;
        case ARM_SERVO_SHOULDER: return pose.useShoulder;
        case ARM_SERVO_ELBOW: return pose.useElbow;
        case ARM_SERVO_WRIST: return pose.useWrist;
        case ARM_SERVO_CLAW: return pose.useClaw;
        default: return false;
    }
}

ArmResult ArmDriver::movePoseDirect(
    const ArmPoseAngles& targetPose,
    const ArmServoSpeed& speedProfile,
    const ArmMotionOptions& options
)
{
    if (!m_initialized) return ARM_RESULT_NOT_INITIALIZED;

    for (int i = 0; i < ARM_SERVO_COUNT; i++)
    {
        ArmServoId servoId = (ArmServoId)i;

        if (!poseUsesServo(targetPose, servoId))
        {
            continue;
        }

        int targetDeg = getTargetServoAngle(targetPose, servoId);
        if (!isValidServoAngle(targetDeg))
        {
            return ARM_RESULT_INVALID_ARGUMENT;
        }
    }

    if (RuntimeState::instance().shouldCancelArm())
    {
        return ARM_RESULT_CANCELLED;
    }

    int previousSpeed = sanitizeSpeedPercent(m_arm.speed);
    int orderCount = options.servoOrderCount > 0 ? options.servoOrderCount : ARM_SERVO_COUNT;
    ArmResult finalResult = ARM_RESULT_OK;

    if (options.mode == ARM_MOTION_ORDERED)
    {
        for (int i = 0; i < orderCount; i++)
        {
            if (RuntimeState::instance().shouldCancelArm())
            {
                finalResult = ARM_RESULT_CANCELLED;
                break;
            }

            ArmServoId servoId =
                options.servoOrderCount > 0 ? options.servoOrder[i] : (ArmServoId)i;

            if (!poseUsesServo(targetPose, servoId))
            {
                continue;
            }

            int servoSpeed = getServoSpeedPercent(speedProfile, servoId);
            m_arm.Speed(servoSpeed);
            writeServoAngle(servoId, getTargetServoAngle(targetPose, servoId));

            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
    else
    {
        for (int i = 0; i < orderCount; i++)
        {
            if (RuntimeState::instance().shouldCancelArm())
            {
                finalResult = ARM_RESULT_CANCELLED;
                break;
            }

            ArmServoId servoId =
                options.servoOrderCount > 0 ? options.servoOrder[i] : (ArmServoId)i;

            if (!poseUsesServo(targetPose, servoId))
            {
                continue;
            }

            int servoSpeed = getServoSpeedPercent(speedProfile, servoId);
            m_arm.Speed(servoSpeed);
            writeServoAngle(servoId, getTargetServoAngle(targetPose, servoId));
        }
    }

    m_arm.Speed(previousSpeed);
    return finalResult;
}

ArmResult ArmDriver::movePoseBySpeed(
    const ArmPoseAngles& targetPose,
    const ArmServoSpeed& speedProfile,
    const ArmMotionOptions& options
)
{
    return movePoseDirect(targetPose, speedProfile, options);
}