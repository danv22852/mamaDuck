#include "arm_driver.h"
#include <math.h>

ArmDriver::ArmDriver()
    : m_initialized(false),
      m_lastChassisAngleDeg(88),
      m_lastShoulderAngleDeg(35),
      m_lastElbowAngleDeg(35),
      m_lastWristAngleDeg(100),
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

    m_arm.ARM_init(
        chassisPin,
        shoulderPin,
        elbowPin,
        wristPin,
        clawPin
    );

    m_arm.Speed(speedPercent);

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
    m_arm.Speed(speedPercent);
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

int ArmDriver::getLastServoAngle(ArmServoId servoId) const
{
    switch (servoId)
    {
        case ARM_SERVO_CHASSIS: return m_lastChassisAngleDeg;
        case ARM_SERVO_SHOULDER: return m_lastShoulderAngleDeg;
        case ARM_SERVO_ELBOW: return m_lastElbowAngleDeg;
        case ARM_SERVO_WRIST: return m_lastWristAngleDeg;
        case ARM_SERVO_CLAW: return m_lastClawAngleDeg;
        default: return 0;
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

void ArmDriver::moveServoSlow(ArmServoId servoId, int targetAngleDeg, unsigned long stepDelayMs)
{
    int currentAngleDeg = getLastServoAngle(servoId);

    if (targetAngleDeg > currentAngleDeg)
    {
        for (int angle = currentAngleDeg; angle <= targetAngleDeg; angle++)
        {
            writeServoAngle(servoId, angle);
            delay(stepDelayMs);
        }
    }
    else
    {
        for (int angle = currentAngleDeg; angle >= targetAngleDeg; angle--)
        {
            writeServoAngle(servoId, angle);
            delay(stepDelayMs);
        }
    }
}

ArmResult ArmDriver::movePose(
    const ArmPoseAngles& targetPose,
    const ArmServoStepDelay& stepDelay,
    const ArmMotionOptions& options
)
{
    if (!m_initialized) return ARM_RESULT_NOT_INITIALIZED;

    for (int i = 0; i < ARM_SERVO_COUNT; i++)
    {
        ArmServoId servoId = (ArmServoId)i;

        if (!poseUsesServo(targetPose, servoId)) continue;

        int targetDeg = getTargetServoAngle(targetPose, servoId);
        if (!isValidServoAngle(targetDeg)) return ARM_RESULT_INVALID_ARGUMENT;
    }

    if (options.mode == ARM_MOTION_ORDERED)
    {
        for (int i = 0; i < options.servoOrderCount; i++)
        {
            ArmServoId servoId = options.servoOrder[i];
            if (!poseUsesServo(targetPose, servoId)) continue;

            unsigned long delayMs = 15;
            if (servoId == ARM_SERVO_CHASSIS) delayMs = stepDelay.chassisMs;
            else if (servoId == ARM_SERVO_SHOULDER) delayMs = stepDelay.shoulderMs;
            else if (servoId == ARM_SERVO_ELBOW) delayMs = stepDelay.elbowMs;
            else if (servoId == ARM_SERVO_WRIST) delayMs = stepDelay.wristMs;
            else if (servoId == ARM_SERVO_CLAW) delayMs = stepDelay.clawMs;

            moveServoSlow(servoId, getTargetServoAngle(targetPose, servoId), delayMs);
        }

        return ARM_RESULT_OK;
    }

    int current[ARM_SERVO_COUNT];
    int target[ARM_SERVO_COUNT];
    unsigned long delayMs[ARM_SERVO_COUNT];
    unsigned long lastStepAt[ARM_SERVO_COUNT];
    bool active[ARM_SERVO_COUNT];

    for (int i = 0; i < ARM_SERVO_COUNT; i++)
    {
        ArmServoId servoId = (ArmServoId)i;
        current[i] = getLastServoAngle(servoId);
        target[i] = getTargetServoAngle(targetPose, servoId);
        lastStepAt[i] = 0;
        active[i] = poseUsesServo(targetPose, servoId) && current[i] != target[i];

        if (servoId == ARM_SERVO_CHASSIS) delayMs[i] = stepDelay.chassisMs;
        else if (servoId == ARM_SERVO_SHOULDER) delayMs[i] = stepDelay.shoulderMs;
        else if (servoId == ARM_SERVO_ELBOW) delayMs[i] = stepDelay.elbowMs;
        else if (servoId == ARM_SERVO_WRIST) delayMs[i] = stepDelay.wristMs;
        else delayMs[i] = stepDelay.clawMs;
    }

    bool anyActive = true;

    while (anyActive)
    {
        anyActive = false;
        unsigned long now = millis();

        for (int i = 0; i < ARM_SERVO_COUNT; i++)
        {
            if (!active[i]) continue;
            anyActive = true;

            if (lastStepAt[i] == 0 || (now - lastStepAt[i]) >= delayMs[i])
            {
                if (current[i] < target[i]) current[i]++;
                else if (current[i] > target[i]) current[i]--;

                writeServoAngle((ArmServoId)i, current[i]);
                lastStepAt[i] = now;

                if (current[i] == target[i])
                {
                    active[i] = false;
                }
            }
        }

        delay(1);
    }

    return ARM_RESULT_OK;
}

ArmResult ArmDriver::moveToCartesianMm(const ArmPointMm& pointMm)
{
    if (!m_initialized) return ARM_RESULT_NOT_INITIALIZED;
    
    float xCm = pointMm.xMm / 10.0f;
    float yCm = pointMm.yMm / 10.0f;
    float zCm = pointMm.zMm / 10.0f;

    float dzCm = zCm - 11.0f;
    float sphereDistance = sqrtf(xCm * xCm + yCm * yCm + dzCm * dzCm);

    Serial.printf("MOVE ARM to X=%.2fcm Y=%.2fcm Z=%.2fcm\n", xCm, yCm, zCm);
    Serial.printf("SPHERE DIST from (0,0,11) = %.2fcm\n", sphereDistance);

    if (!isWithinWorkspaceCm(xCm, yCm, zCm)) return ARM_RESULT_OUT_OF_REACH;

    m_arm.PtpCmd(
        (int)roundf(xCm),
        (int)roundf(yCm),
        (int)roundf(zCm)
    );

    return ARM_RESULT_OK;
}

bool ArmDriver::isWithinWorkspaceCm(float xCm, float yCm, float zCm) const
{
    return true;
}