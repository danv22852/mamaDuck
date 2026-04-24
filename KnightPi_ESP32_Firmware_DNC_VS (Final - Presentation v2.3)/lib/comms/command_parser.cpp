#include <ArduinoJson.h>
#include "command_parser.h"

void CommandParser::copyText(char* dest, size_t destSize, const char* src) const
{
    if (destSize == 0) return;
    if (src == nullptr) src = "";
    strncpy(dest, src, destSize - 1);
    dest[destSize - 1] = '\0';
}

ArmMotionMode CommandParser::parseArmMotionMode(const String& text) const
{
    String value = text;
    value.trim();
    value.toUpperCase();

    if (value == "COMBINED") return ARM_MOTION_COMBINED;
    return ARM_MOTION_ORDERED;
}

ArmServoId CommandParser::parseServoId(const String& text) const
{
    String value = text;
    value.trim();
    value.toUpperCase();

    if (value == "CHASSIS") return ARM_SERVO_CHASSIS;
    if (value == "SHOULDER") return ARM_SERVO_SHOULDER;
    if (value == "ELBOW") return ARM_SERVO_ELBOW;
    if (value == "WRIST") return ARM_SERVO_WRIST;
    return ARM_SERVO_CLAW;
}

CancelTarget CommandParser::parseCancelTarget(const String& text) const
{
    String value = text;
    value.trim();
    value.toUpperCase();

    if (value == "DRIVE") return CANCEL_TARGET_DRIVE;
    if (value == "ARM") return CANCEL_TARGET_ARM;
    return CANCEL_TARGET_ALL;
}

Command CommandParser::parse(const String& rawCommand) const
{
    Command command;

    String trimmedCommand = rawCommand;
    trimmedCommand.trim();

    if (parseJsonEnvelope(trimmedCommand, command))
    {
        return command;
    }

    return command;
}

bool CommandParser::parseJsonEnvelope(const String& rawCommand, Command& command) const
{
    if (rawCommand.length() == 0) return false;
    if (rawCommand[0] != '{') return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, rawCommand);

    if (err)
    {
        return false;
    }

    command.expectsJsonResponse = true;
    copyText(command.transactionId, sizeof(command.transactionId), doc["transactionId"] | "");

    String cmd = doc["command"] | "";
    cmd.toUpperCase();

    if (cmd == "PING")
    {
        command.type = CMD_PING;
        return true;
    }
    else if (cmd == "STATUS")
    {
        command.type = CMD_STATUS;
        return true;
    }
    else if (cmd == "CANCEL")
    {
        command.type = CMD_CANCEL;
        command.cancelTarget = parseCancelTarget((const char*)(doc["args"]["target"] | "ALL"));
        return true;
    }
    else if (cmd == "STOP")
    {
        command.type = CMD_STOP;
        return true;
    }
    else if (cmd == "US_MONITOR_ON")
    {
        command.type = CMD_US_MONITOR_ON;
        command.usMonitorAlarmDistanceMm = doc["args"]["alarmDistanceMm"] | DEFAULT_US_MONITOR_ALARM_DISTANCE_MM;
        return true;
    }
    else if (cmd == "US_MONITOR_OFF")
    {
        command.type = CMD_US_MONITOR_OFF;
        return true;
    }
    else if (cmd == "US_SCAN")
    {
        command.type = CMD_US_SCAN;
        command.usScanAngleDeg = doc["args"]["scanAngleDeg"] | 0.0f;
        command.usScanSteps = doc["args"]["steps"] | 0;
        command.usScanStepRotateMs = doc["args"]["stepRotateMs"] | DEFAULT_US_SCAN_STEP_ROTATE_MS;
        command.usScanRotateSpeed = doc["args"]["rotateSpeed"] | DEFAULT_US_SCAN_ROTATE_SPEED;

        String rotation = doc["args"]["rotation"] | "CCW";
        rotation.toUpperCase();
        command.usScanRotation = (rotation == "CW") ? SCAN_CW : SCAN_CCW;
        return true;
    }
    else if (cmd == "MOVE_FW")
    {
        command.type = CMD_MOVE_FW;
        command.moveAmountMs = doc["args"]["durationMs"] | 300;
        command.moveSpeed = doc["args"]["speed"] | 50;
        return true;
    }
    else if (cmd == "MOVE_BW")
    {
        command.type = CMD_MOVE_BW;
        command.moveAmountMs = doc["args"]["durationMs"] | 300;
        command.moveSpeed = doc["args"]["speed"] | 50;
        return true;
    }
    else if (cmd == "MOVE_LEFT")
    {
        command.type = CMD_MOVE_LEFT;
        command.moveAmountMs = doc["args"]["durationMs"] | 300;
        command.moveSpeed = doc["args"]["speed"] | 50;
        return true;
    }
    else if (cmd == "MOVE_RIGHT")
    {
        command.type = CMD_MOVE_RIGHT;
        command.moveAmountMs = doc["args"]["durationMs"] | 300;
        command.moveSpeed = doc["args"]["speed"] | 50;
        return true;
    }
    else if (cmd == "ROTATE_CW")
    {
        command.type = CMD_ROTATE_CW;
        command.moveAmountMs = doc["args"]["durationMs"] | 300;
        command.moveSpeed = doc["args"]["speed"] | 50;
        return true;
    }
    else if (cmd == "ROTATE_CCW")
    {
        command.type = CMD_ROTATE_CCW;
        command.moveAmountMs = doc["args"]["durationMs"] | 300;
        command.moveSpeed = doc["args"]["speed"] | 50;
        return true;
    }
    else if (cmd == "PICK_ANGLES_SPEED")
    {
        command.type = CMD_PICK_ANGLES_SPEED;

        JsonObject pickupPose = doc["args"]["pickupPose"];
        command.armPickupChassisAngleDeg = pickupPose["chassisAngleDeg"] | 0.0f;
        command.armPickupShoulderAngleDeg = pickupPose["shoulderAngleDeg"] | 0.0f;
        command.armPickupElbowAngleDeg = pickupPose["elbowAngleDeg"] | 0.0f;
        command.armPickupWristAngleDeg = pickupPose["wristAngleDeg"] | 0.0f;
        command.armOpenClawAngleDeg = pickupPose["openClawAngleDeg"] | 0.0f;
        command.armCloseClawAngleDeg = pickupPose["closeClawAngleDeg"] | 0.0f;

        JsonObject liftPose = doc["args"]["liftPose"];
        command.armLiftChassisAngleDeg = liftPose["chassisAngleDeg"] | 0.0f;
        command.armLiftShoulderAngleDeg = liftPose["shoulderAngleDeg"] | 0.0f;
        command.armLiftElbowAngleDeg = liftPose["elbowAngleDeg"] | 0.0f;
        command.armLiftWristAngleDeg = liftPose["wristAngleDeg"] | 0.0f;

        JsonObject pickupSpeed = doc["args"]["pickupSpeedPercent"];
        command.armPickupSpeed.chassisPercent = pickupSpeed["chassis"] | 50;
        command.armPickupSpeed.shoulderPercent = pickupSpeed["shoulder"] | 50;
        command.armPickupSpeed.elbowPercent = pickupSpeed["elbow"] | 50;
        command.armPickupSpeed.wristPercent = pickupSpeed["wrist"] | 50;
        command.armPickupSpeed.clawPercent = pickupSpeed["claw"] | 50;

        JsonObject liftSpeed = doc["args"]["liftSpeedPercent"];
        command.armLiftSpeed.chassisPercent = liftSpeed["chassis"] | 50;
        command.armLiftSpeed.shoulderPercent = liftSpeed["shoulder"] | 50;
        command.armLiftSpeed.elbowPercent = liftSpeed["elbow"] | 50;
        command.armLiftSpeed.wristPercent = liftSpeed["wrist"] | 50;
        command.armLiftSpeed.clawPercent = liftSpeed["claw"] | 50;

        command.armPickupMotionMode = parseArmMotionMode((const char*)(doc["args"]["pickupMotionMode"] | "ORDERED"));
        command.armLiftMotionMode = parseArmMotionMode((const char*)(doc["args"]["liftMotionMode"] | "ORDERED"));

        command.armPickupServoOrderCount = 0;
        JsonArray pickupOrder = doc["args"]["pickupServoOrder"].as<JsonArray>();
        for (JsonVariant v : pickupOrder)
        {
            if (command.armPickupServoOrderCount >= ARM_SERVO_COUNT) break;
            command.armPickupServoOrder[command.armPickupServoOrderCount++] = parseServoId(v.as<String>());
        }

        command.armLiftServoOrderCount = 0;
        JsonArray liftOrder = doc["args"]["liftServoOrder"].as<JsonArray>();
        for (JsonVariant v : liftOrder)
        {
            if (command.armLiftServoOrderCount >= ARM_SERVO_COUNT) break;
            command.armLiftServoOrder[command.armLiftServoOrderCount++] = parseServoId(v.as<String>());
        }

        return true;
    }
    else if (cmd == "DROP_ANGLES_SPEED")
    {
        command.type = CMD_DROP_ANGLES_SPEED;

        JsonObject dropPose = doc["args"]["dropPose"];
        command.armDropChassisAngleDeg = dropPose["chassisAngleDeg"] | 0.0f;
        command.armDropShoulderAngleDeg = dropPose["shoulderAngleDeg"] | 0.0f;
        command.armDropElbowAngleDeg = dropPose["elbowAngleDeg"] | 0.0f;
        command.armDropWristAngleDeg = dropPose["wristAngleDeg"] | 0.0f;
        command.armDropOpenClawAngleDeg = dropPose["openClawAngleDeg"] | 0.0f;

        JsonObject retreatPose = doc["args"]["retreatPose"];
        command.armRetreatChassisAngleDeg = retreatPose["chassisAngleDeg"] | 0.0f;
        command.armRetreatShoulderAngleDeg = retreatPose["shoulderAngleDeg"] | 0.0f;
        command.armRetreatElbowAngleDeg = retreatPose["elbowAngleDeg"] | 0.0f;
        command.armRetreatWristAngleDeg = retreatPose["wristAngleDeg"] | 0.0f;

        JsonObject dropSpeed = doc["args"]["dropSpeedPercent"];
        command.armDropSpeed.chassisPercent = dropSpeed["chassis"] | 50;
        command.armDropSpeed.shoulderPercent = dropSpeed["shoulder"] | 50;
        command.armDropSpeed.elbowPercent = dropSpeed["elbow"] | 50;
        command.armDropSpeed.wristPercent = dropSpeed["wrist"] | 50;
        command.armDropSpeed.clawPercent = dropSpeed["claw"] | 50;

        JsonObject retreatSpeed = doc["args"]["retreatSpeedPercent"];
        command.armRetreatSpeed.chassisPercent = retreatSpeed["chassis"] | 50;
        command.armRetreatSpeed.shoulderPercent = retreatSpeed["shoulder"] | 50;
        command.armRetreatSpeed.elbowPercent = retreatSpeed["elbow"] | 50;
        command.armRetreatSpeed.wristPercent = retreatSpeed["wrist"] | 50;
        command.armRetreatSpeed.clawPercent = retreatSpeed["claw"] | 50;

        command.armDropMotionMode = parseArmMotionMode((const char*)(doc["args"]["dropMotionMode"] | "ORDERED"));
        command.armRetreatMotionMode = parseArmMotionMode((const char*)(doc["args"]["retreatMotionMode"] | "ORDERED"));

        command.armDropServoOrderCount = 0;
        JsonArray dropOrder = doc["args"]["dropServoOrder"].as<JsonArray>();
        for (JsonVariant v : dropOrder)
        {
            if (command.armDropServoOrderCount >= ARM_SERVO_COUNT) break;
            command.armDropServoOrder[command.armDropServoOrderCount++] = parseServoId(v.as<String>());
        }

        command.armRetreatServoOrderCount = 0;
        JsonArray retreatOrder = doc["args"]["retreatServoOrder"].as<JsonArray>();
        for (JsonVariant v : retreatOrder)
        {
            if (command.armRetreatServoOrderCount >= ARM_SERVO_COUNT) break;
            command.armRetreatServoOrder[command.armRetreatServoOrderCount++] = parseServoId(v.as<String>());
        }

        return true;
    }
    else if (cmd == "MOVE_ARM_ANGLES_SPEED")
    {
        command.type = CMD_MOVE_ARM_ANGLES_SPEED;

        JsonObject pose = doc["args"]["pose"];
        command.armMoveChassisAngleDeg = pose["chassisAngleDeg"] | 0.0f;
        command.armMoveShoulderAngleDeg = pose["shoulderAngleDeg"] | 0.0f;
        command.armMoveElbowAngleDeg = pose["elbowAngleDeg"] | 0.0f;
        command.armMoveWristAngleDeg = pose["wristAngleDeg"] | 0.0f;
        command.armMoveClawAngleDeg = pose["clawAngleDeg"] | 0.0f;

        JsonObject speedPercent = doc["args"]["speedPercent"];
        command.armMoveSpeed.chassisPercent = speedPercent["chassis"] | 50;
        command.armMoveSpeed.shoulderPercent = speedPercent["shoulder"] | 50;
        command.armMoveSpeed.elbowPercent = speedPercent["elbow"] | 50;
        command.armMoveSpeed.wristPercent = speedPercent["wrist"] | 50;
        command.armMoveSpeed.clawPercent = speedPercent["claw"] | 50;

        command.armMoveMotionMode = parseArmMotionMode((const char*)(doc["args"]["motionMode"] | "ORDERED"));

        command.armMoveServoOrderCount = 0;
        JsonArray moveOrder = doc["args"]["servoOrder"].as<JsonArray>();
        for (JsonVariant v : moveOrder)
        {
            if (command.armMoveServoOrderCount >= ARM_SERVO_COUNT) break;
            command.armMoveServoOrder[command.armMoveServoOrderCount++] = parseServoId(v.as<String>());
        }

        return true;
    }

    command.type = CMD_UNKNOWN;
    return true;
}