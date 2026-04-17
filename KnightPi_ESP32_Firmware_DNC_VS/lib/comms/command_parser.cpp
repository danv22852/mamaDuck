#include <ArduinoJson.h>
#include "command_parser.h"

Command CommandParser::parse(const String& rawCommand) const
{
    Command command;
    command.originalText = rawCommand;

    String trimmedCommand = rawCommand;
    trimmedCommand.trim();

    if (parseJsonEnvelope(trimmedCommand, command)) return command;

    if (parsePickObjectCommand(trimmedCommand, command)) return command;
    if (parseTimedMoveCommand(trimmedCommand, command)) return command;
    if (parseUsScanCommand(trimmedCommand, command)) return command;

    command.type = parseCommandType(trimmedCommand);
    return command;
}

CommandType CommandParser::parseCommandType(String cmd) const
{
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "PING") return CMD_PING;
    else if (cmd == "LED_ON") return CMD_LED_ON;
    else if (cmd == "LED_OFF") return CMD_LED_OFF;
    else if (cmd == "BLINK") return CMD_BLINK;
    else if (cmd == "MOVE_FW") return CMD_MOVE_FW;
    else if (cmd == "MOVE_BW") return CMD_MOVE_BW;
    else if (cmd == "MOVE_LEFT") return CMD_MOVE_LEFT;
    else if (cmd == "MOVE_RIGHT") return CMD_MOVE_RIGHT;
    else if (cmd == "ROTATE_CW") return CMD_ROTATE_CW;
    else if (cmd == "ROTATE_CCW") return CMD_ROTATE_CCW;
    else if (cmd == "STOP") return CMD_STOP;
    else if (cmd == "US_TEST") return CMD_US_TEST;
    else if (cmd == "US_SCAN") return CMD_US_SCAN;
    else if (cmd == "PICK_ANGLES") return CMD_PICK_ANGLES;
    else if (cmd == "DROP_ANGLES") return CMD_DROP_ANGLES;
    else if (cmd == "MOVE_ARM_ANGLES") return CMD_MOVE_ARM_ANGLES;

    return CMD_UNKNOWN;
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

bool CommandParser::parseJsonEnvelope(const String& rawCommand, Command& command) const
{
    if (rawCommand.length() == 0) return false;
    if (rawCommand[0] != '{') return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, rawCommand);
    if (err)
    {
        command.type = CMD_UNKNOWN;
        return true;
    }

    command.expectsJsonResponse = true;
    command.transactionId = doc["transactionId"] | "";
    String cmd = doc["command"] | "";
    cmd.toUpperCase();

    if (cmd == "PING")
    {
        command.type = CMD_PING;
        return true;
    }
    else if (cmd == "US_TEST")
    {
        command.type = CMD_US_TEST;
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
    else if (cmd == "ROTATE_CW")
    {
        command.type = CMD_ROTATE_CW;
        command.moveAmountMs = doc["args"]["durationMs"] | 300;
        command.moveSpeed = doc["args"]["speed"] | 50;
        return true;
    }
    else if (cmd == "PICK_ANGLES")
    {
        command.type = CMD_PICK_ANGLES;

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

        JsonObject stepDelay = doc["args"]["stepDelayMs"];
        command.armStepDelay.chassisMs = stepDelay["chassis"] | 15;
        command.armStepDelay.shoulderMs = stepDelay["shoulder"] | 15;
        command.armStepDelay.elbowMs = stepDelay["elbow"] | 15;
        command.armStepDelay.wristMs = stepDelay["wrist"] | 15;
        command.armStepDelay.clawMs = stepDelay["claw"] | 15;

        command.armPickupMotionMode = parseArmMotionMode((const char*)(doc["args"]["pickupMotionMode"] | "ORDERED"));
        command.armLiftMotionMode = parseArmMotionMode((const char*)(doc["args"]["liftMotionMode"] | "ORDERED"));

        command.armPickupServoOrderCount = 0;
        JsonArray pickupOrder = doc["args"]["pickupServoOrder"].as<JsonArray>();
        for (JsonVariant v : pickupOrder)
        {
            if (command.armPickupServoOrderCount >= ARM_SERVO_COUNT) break;
            command.armPickupServoOrder[command.armPickupServoOrderCount++] = parseServoId(v.as<String>());
        }

        if (command.armPickupServoOrderCount == 0)
        {
            command.armPickupServoOrderCount = 5;
            command.armPickupServoOrder[0] = ARM_SERVO_CLAW;
            command.armPickupServoOrder[1] = ARM_SERVO_CHASSIS;
            command.armPickupServoOrder[2] = ARM_SERVO_SHOULDER;
            command.armPickupServoOrder[3] = ARM_SERVO_ELBOW;
            command.armPickupServoOrder[4] = ARM_SERVO_WRIST;
        }

        command.armLiftServoOrderCount = 0;
        JsonArray liftOrder = doc["args"]["liftServoOrder"].as<JsonArray>();
        for (JsonVariant v : liftOrder)
        {
            if (command.armLiftServoOrderCount >= ARM_SERVO_COUNT) break;
            command.armLiftServoOrder[command.armLiftServoOrderCount++] = parseServoId(v.as<String>());
        }

        if (command.armLiftServoOrderCount == 0)
        {
            command.armLiftServoOrderCount = 4;
            command.armLiftServoOrder[0] = ARM_SERVO_SHOULDER;
            command.armLiftServoOrder[1] = ARM_SERVO_ELBOW;
            command.armLiftServoOrder[2] = ARM_SERVO_WRIST;
            command.armLiftServoOrder[3] = ARM_SERVO_CHASSIS;
        }

        return true;
    }
    else if (cmd == "DROP_ANGLES")
    {
        command.type = CMD_DROP_ANGLES;

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

        JsonObject stepDelay = doc["args"]["stepDelayMs"];
        command.armStepDelay.chassisMs = stepDelay["chassis"] | 15;
        command.armStepDelay.shoulderMs = stepDelay["shoulder"] | 15;
        command.armStepDelay.elbowMs = stepDelay["elbow"] | 15;
        command.armStepDelay.wristMs = stepDelay["wrist"] | 15;
        command.armStepDelay.clawMs = stepDelay["claw"] | 15;

        command.armDropMotionMode = parseArmMotionMode((const char*)(doc["args"]["dropMotionMode"] | "ORDERED"));
        command.armRetreatMotionMode = parseArmMotionMode((const char*)(doc["args"]["retreatMotionMode"] | "ORDERED"));

        command.armDropServoOrderCount = 0;
        JsonArray dropOrder = doc["args"]["dropServoOrder"].as<JsonArray>();
        for (JsonVariant v : dropOrder)
        {
            if (command.armDropServoOrderCount >= ARM_SERVO_COUNT) break;
            command.armDropServoOrder[command.armDropServoOrderCount++] = parseServoId(v.as<String>());
        }

        if (command.armDropServoOrderCount == 0)
        {
            command.armDropServoOrderCount = 4;
            command.armDropServoOrder[0] = ARM_SERVO_CHASSIS;
            command.armDropServoOrder[1] = ARM_SERVO_SHOULDER;
            command.armDropServoOrder[2] = ARM_SERVO_ELBOW;
            command.armDropServoOrder[3] = ARM_SERVO_WRIST;
        }

        command.armRetreatServoOrderCount = 0;
        JsonArray retreatOrder = doc["args"]["retreatServoOrder"].as<JsonArray>();
        for (JsonVariant v : retreatOrder)
        {
            if (command.armRetreatServoOrderCount >= ARM_SERVO_COUNT) break;
            command.armRetreatServoOrder[command.armRetreatServoOrderCount++] = parseServoId(v.as<String>());
        }

        if (command.armRetreatServoOrderCount == 0)
        {
            command.armRetreatServoOrderCount = 4;
            command.armRetreatServoOrder[0] = ARM_SERVO_SHOULDER;
            command.armRetreatServoOrder[1] = ARM_SERVO_ELBOW;
            command.armRetreatServoOrder[2] = ARM_SERVO_WRIST;
            command.armRetreatServoOrder[3] = ARM_SERVO_CHASSIS;
        }

        return true;
    }
    else if (cmd == "MOVE_ARM_ANGLES")
    {
        command.type = CMD_MOVE_ARM_ANGLES;

        JsonObject pose = doc["args"]["pose"];
        command.armMoveChassisAngleDeg = pose["chassisAngleDeg"] | 0.0f;
        command.armMoveShoulderAngleDeg = pose["shoulderAngleDeg"] | 0.0f;
        command.armMoveElbowAngleDeg = pose["elbowAngleDeg"] | 0.0f;
        command.armMoveWristAngleDeg = pose["wristAngleDeg"] | 0.0f;
        command.armMoveClawAngleDeg = pose["clawAngleDeg"] | 0.0f;

        JsonObject stepDelay = doc["args"]["stepDelayMs"];
        command.armStepDelay.chassisMs = stepDelay["chassis"] | 15;
        command.armStepDelay.shoulderMs = stepDelay["shoulder"] | 15;
        command.armStepDelay.elbowMs = stepDelay["elbow"] | 15;
        command.armStepDelay.wristMs = stepDelay["wrist"] | 15;
        command.armStepDelay.clawMs = stepDelay["claw"] | 15;

        command.armMoveMotionMode = parseArmMotionMode((const char*)(doc["args"]["motionMode"] | "ORDERED"));

        command.armMoveServoOrderCount = 0;
        JsonArray moveOrder = doc["args"]["servoOrder"].as<JsonArray>();
        for (JsonVariant v : moveOrder)
        {
            if (command.armMoveServoOrderCount >= ARM_SERVO_COUNT) break;
            command.armMoveServoOrder[command.armMoveServoOrderCount++] = parseServoId(v.as<String>());
        }

        if (command.armMoveServoOrderCount == 0)
        {
            command.armMoveServoOrderCount = 5;
            command.armMoveServoOrder[0] = ARM_SERVO_CHASSIS;
            command.armMoveServoOrder[1] = ARM_SERVO_SHOULDER;
            command.armMoveServoOrder[2] = ARM_SERVO_ELBOW;
            command.armMoveServoOrder[3] = ARM_SERVO_WRIST;
            command.armMoveServoOrder[4] = ARM_SERVO_CLAW;
        }

        return true;
    }

    command.type = CMD_UNKNOWN;
    return true;
}

bool CommandParser::parsePickObjectCommand(const String& rawCommand, Command& command) const
{
    float angleDeg = 0.0f;
    float distanceMm = 0.0f;
    float heightMm = 0.0f;

    int parsed = sscanf(
        rawCommand.c_str(),
        "PICK_OBJECT %f %f %f",
        &angleDeg,
        &distanceMm,
        &heightMm
    );

    if (parsed == 3)
    {
        command.type = CMD_PICK_OBJECT;
        command.pickAngleDeg = angleDeg;
        command.pickDistanceMm = distanceMm;
        command.pickHeightMm = heightMm;
        return true;
    }

    return false;
}

bool CommandParser::parseTimedMoveCommand(const String& rawCommand, Command& command) const
{
    int amountMs = 0;
    int speed = 50;

    if (sscanf(rawCommand.c_str(), "MOVE_FW %d %d", &amountMs, &speed) >= 1)
    {
        command.type = CMD_MOVE_FW;
        command.moveAmountMs = amountMs;
        command.moveSpeed = speed;
        return true;
    }
    else if (sscanf(rawCommand.c_str(), "MOVE_BW %d %d", &amountMs, &speed) >= 1)
    {
        command.type = CMD_MOVE_BW;
        command.moveAmountMs = amountMs;
        command.moveSpeed = speed;
        return true;
    }
    else if (sscanf(rawCommand.c_str(), "MOVE_LEFT %d %d", &amountMs, &speed) >= 1)
    {
        command.type = CMD_MOVE_LEFT;
        command.moveAmountMs = amountMs;
        command.moveSpeed = speed;
        return true;
    }
    else if (sscanf(rawCommand.c_str(), "MOVE_RIGHT %d %d", &amountMs, &speed) >= 1)
    {
        command.type = CMD_MOVE_RIGHT;
        command.moveAmountMs = amountMs;
        command.moveSpeed = speed;
        return true;
    }
    else if (sscanf(rawCommand.c_str(), "ROTATE_CW %d %d", &amountMs, &speed) >= 1)
    {
        command.type = CMD_ROTATE_CW;
        command.moveAmountMs = amountMs;
        command.moveSpeed = speed;
        return true;
    }
    else if (sscanf(rawCommand.c_str(), "ROTATE_CCW %d %d", &amountMs, &speed) >= 1)
    {
        command.type = CMD_ROTATE_CCW;
        command.moveAmountMs = amountMs;
        command.moveSpeed = speed;
        return true;
    }

    return false;
}

bool CommandParser::parseUsScanCommand(const String& rawCommand, Command& command) const
{
    float scanAngleDeg = 0.0f;
    int steps = 0;
    unsigned long stepRotateMs = 0;
    int rotateSpeed = 0;
    char rotationText[8] = {0};

    int parsed = sscanf(
        rawCommand.c_str(),
        "US_SCAN %f %d %lu %d %7s",
        &scanAngleDeg,
        &steps,
        &stepRotateMs,
        &rotateSpeed,
        rotationText
    );

    if (parsed >= 4)
    {
        command.type = CMD_US_SCAN;
        command.usScanAngleDeg = scanAngleDeg;
        command.usScanSteps = steps;
        command.usScanStepRotateMs = stepRotateMs;
        command.usScanRotateSpeed = rotateSpeed;
        command.usScanRotation = SCAN_CCW;

        if (parsed == 5)
        {
            String dir = String(rotationText);
            dir.toUpperCase();

            if (dir == "CW")
            {
                command.usScanRotation = SCAN_CW;
            }
            else
            {
                command.usScanRotation = SCAN_CCW;
            }
        }

        return true;
    }

    return false;
}