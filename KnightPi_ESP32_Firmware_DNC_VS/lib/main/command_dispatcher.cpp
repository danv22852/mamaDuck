#include <ArduinoJson.h>
#include "command_dispatcher.h"
#include "../common/config.h"

CommandDispatcher::CommandDispatcher(
    DriveBase& driveBase,
    UltrasonicScanner& ultrasonicScanner,
    RoboticArm& roboticArm
)
    : m_driveBase(driveBase),
      m_ultrasonicScanner(ultrasonicScanner),
      m_roboticArm(roboticArm)
{
}

void CommandDispatcher::dispatch(const Command& command)
{
    switch (command.type)
    {
        case CMD_PING:
            if (command.expectsJsonResponse) writeJsonOk(command, "PING");
            else Serial.println("ACK PING");
            break;

        case CMD_LED_ON:
            digitalWrite(2, HIGH);
            Serial.println("ACK LED_ON");
            break;

        case CMD_LED_OFF:
            digitalWrite(2, LOW);
            Serial.println("ACK LED_OFF");
            break;

        case CMD_BLINK:
            handleBlink();
            break;

        case CMD_MOVE_FW:
            handleTimedMove(
                CMD_MOVE_FW,
                command.moveAmountMs > 0 ? command.moveAmountMs : 300,
                command.moveSpeed > 0 ? command.moveSpeed : 50
            );
            Serial.print("ACK MOVE_FW ms=");
            Serial.print(command.moveAmountMs > 0 ? command.moveAmountMs : 300);
            Serial.print(" speed=");
            Serial.println(command.moveSpeed > 0 ? command.moveSpeed : 50);
            break;

        case CMD_MOVE_BW:
            handleTimedMove(
                CMD_MOVE_BW,
                command.moveAmountMs > 0 ? command.moveAmountMs : 300,
                command.moveSpeed > 0 ? command.moveSpeed : 50
            );
            Serial.print("ACK MOVE_BW ms=");
            Serial.print(command.moveAmountMs > 0 ? command.moveAmountMs : 300);
            Serial.print(" speed=");
            Serial.println(command.moveSpeed > 0 ? command.moveSpeed : 50);
            break;

        case CMD_MOVE_LEFT:
            handleTimedMove(
                CMD_MOVE_LEFT,
                command.moveAmountMs > 0 ? command.moveAmountMs : 300,
                command.moveSpeed > 0 ? command.moveSpeed : 50
            );
            Serial.print("ACK MOVE_LEFT ms=");
            Serial.print(command.moveAmountMs > 0 ? command.moveAmountMs : 300);
            Serial.print(" speed=");
            Serial.println(command.moveSpeed > 0 ? command.moveSpeed : 50);
            break;

        case CMD_MOVE_RIGHT:
            handleTimedMove(
                CMD_MOVE_RIGHT,
                command.moveAmountMs > 0 ? command.moveAmountMs : 300,
                command.moveSpeed > 0 ? command.moveSpeed : 50
            );
            Serial.print("ACK MOVE_RIGHT ms=");
            Serial.print(command.moveAmountMs > 0 ? command.moveAmountMs : 300);
            Serial.print(" speed=");
            Serial.println(command.moveSpeed > 0 ? command.moveSpeed : 50);
            break;

        case CMD_ROTATE_CW:
            handleTimedMove(
                CMD_ROTATE_CW,
                command.moveAmountMs > 0 ? command.moveAmountMs : 300,
                command.moveSpeed > 0 ? command.moveSpeed : 50
            );
            Serial.print("ACK ROTATE_CW ms=");
            Serial.print(command.moveAmountMs > 0 ? command.moveAmountMs : 300);
            Serial.print(" speed=");
            Serial.println(command.moveSpeed > 0 ? command.moveSpeed : 50);
            break;

        case CMD_ROTATE_CCW:
            handleTimedMove(
                CMD_ROTATE_CCW,
                command.moveAmountMs > 0 ? command.moveAmountMs : 300,
                command.moveSpeed > 0 ? command.moveSpeed : 50
            );
            Serial.print("ACK ROTATE_CCW ms=");
            Serial.print(command.moveAmountMs > 0 ? command.moveAmountMs : 300);
            Serial.print(" speed=");
            Serial.println(command.moveSpeed > 0 ? command.moveSpeed : 50);
            break;

        case CMD_STOP:
            m_driveBase.stop();
            Serial.println("ACK STOP");
            break;

        case CMD_US_TEST:
        {
            float distanceMm = m_ultrasonicScanner.readDistanceMm();

            if (command.expectsJsonResponse)
            {
                writeJsonUsTestResponse(command, distanceMm);
            }
            else
            {
                Serial.print("ACK US_TEST ");
                Serial.println(distanceMm);
            }
            break;
        }

        case CMD_US_SCAN:
        {
            UltrasonicSweepResult result;

            bool ok = m_ultrasonicScanner.usScan(
                command.usScanAngleDeg,
                command.usScanSteps,
                command.usScanStepRotateMs,
                command.usScanRotateSpeed,
                command.usScanRotation,
                result
            );

            if (!ok)
            {
                if (command.expectsJsonResponse)
                {
                    writeJsonError(command, "US_SCAN", "INVALID_ARGUMENT", "Invalid US_SCAN arguments");
                }
                else
                {
                    Serial.println("ERR US_SCAN INVALID_ARGUMENT");
                }
                break;
            }

            if (command.expectsJsonResponse)
            {
                writeJsonUsScanResponse(command, result);
            }
            else
            {
                Serial.print("ACK US_SCAN COUNT=");
                Serial.print(result.pointCount);
                Serial.print(" STEP_MS=");
                Serial.print(command.usScanStepRotateMs);
                Serial.print(" SPEED=");
                Serial.print(command.usScanRotateSpeed);
                Serial.print(" DIR=");
                Serial.print(command.usScanRotation == SCAN_CW ? "CW" : "CCW");
                Serial.print(" DATA=[");

                for (int i = 0; i < result.pointCount; i++)
                {
                    if (i > 0) Serial.print(",");
                    Serial.print("{A=");
                    Serial.print(result.points[i].angleDeg);
                    Serial.print(",D=");
                    Serial.print(result.points[i].distanceMm);
                    Serial.print("}\r\n");
                }

                Serial.println("]");
            }

            break;
        }

        case CMD_PICK_OBJECT:
        {
            Serial.print("ACK PICK_OBJECT START ");
            Serial.print("angle=");
            Serial.print(command.pickAngleDeg);
            Serial.print(" distanceMm=");
            Serial.print(command.pickDistanceMm);
            Serial.print(" heightMm=");
            Serial.println(command.pickHeightMm);

            ArmResult result = m_roboticArm.pickObject(
                command.pickAngleDeg,
                command.pickDistanceMm,
                command.pickHeightMm
            );

            if (result == ARM_RESULT_OK) Serial.println("ACK PICK_OBJECT OK");
            else if (result == ARM_RESULT_NOT_INITIALIZED) Serial.println("ERR PICK_OBJECT NOT_INITIALIZED");
            else if (result == ARM_RESULT_INVALID_ARGUMENT) Serial.println("ERR PICK_OBJECT INVALID_ARGUMENT");
            else if (result == ARM_RESULT_OUT_OF_REACH) Serial.println("ERR PICK_OBJECT OUT_OF_REACH");
            else Serial.println("ERR PICK_OBJECT UNKNOWN");

            break;
        }

        case CMD_PICK_ANGLES:
        {
            ArmMotionOptions pickupOptions;
            pickupOptions.mode = command.armPickupMotionMode;
            pickupOptions.servoOrderCount = command.armPickupServoOrderCount;
            for (int i = 0; i < pickupOptions.servoOrderCount; i++)
            {
                pickupOptions.servoOrder[i] = command.armPickupServoOrder[i];
            }

            ArmMotionOptions liftOptions;
            liftOptions.mode = command.armLiftMotionMode;
            liftOptions.servoOrderCount = command.armLiftServoOrderCount;
            for (int i = 0; i < liftOptions.servoOrderCount; i++)
            {
                liftOptions.servoOrder[i] = command.armLiftServoOrder[i];
            }

            ArmPickResult result = m_roboticArm.pickByAngles(
                command.armPickupChassisAngleDeg,
                command.armPickupShoulderAngleDeg,
                command.armPickupElbowAngleDeg,
                command.armPickupWristAngleDeg,
                command.armOpenClawAngleDeg,
                command.armCloseClawAngleDeg,
                command.armLiftChassisAngleDeg,
                command.armLiftShoulderAngleDeg,
                command.armLiftElbowAngleDeg,
                command.armLiftWristAngleDeg,
                command.armStepDelay,
                pickupOptions,
                liftOptions
            );

            if (command.expectsJsonResponse)
            {
                if (result.result == ARM_RESULT_OK) writeJsonPickAnglesResponse(command, result);
                else writeJsonError(command, "PICK_ANGLES", "INVALID_ARGUMENT", "Invalid PICK_ANGLES arguments");
            }
            else
            {
                if (result.result == ARM_RESULT_OK)
                {
                    Serial.print("ACK PICK_ANGLES PICKED=");
                    Serial.print(result.picked ? "YES" : "NO");
                    Serial.print(" LIFTED=");
                    Serial.println(result.transportedToSecurePose ? "YES" : "NO");
                }
                else if (result.result == ARM_RESULT_NOT_INITIALIZED) Serial.println("ERR PICK_ANGLES NOT_INITIALIZED");
                else if (result.result == ARM_RESULT_INVALID_ARGUMENT) Serial.println("ERR PICK_ANGLES INVALID_ARGUMENT");
                else Serial.println("ERR PICK_ANGLES UNKNOWN");
            }

            break;
        }

        case CMD_DROP_ANGLES:
        {
            ArmMotionOptions dropOptions;
            dropOptions.mode = command.armDropMotionMode;
            dropOptions.servoOrderCount = command.armDropServoOrderCount;
            for (int i = 0; i < dropOptions.servoOrderCount; i++)
            {
                dropOptions.servoOrder[i] = command.armDropServoOrder[i];
            }

            ArmMotionOptions retreatOptions;
            retreatOptions.mode = command.armRetreatMotionMode;
            retreatOptions.servoOrderCount = command.armRetreatServoOrderCount;
            for (int i = 0; i < retreatOptions.servoOrderCount; i++)
            {
                retreatOptions.servoOrder[i] = command.armRetreatServoOrder[i];
            }

            ArmDropResult result = m_roboticArm.dropByAngles(
                command.armDropChassisAngleDeg,
                command.armDropShoulderAngleDeg,
                command.armDropElbowAngleDeg,
                command.armDropWristAngleDeg,
                command.armDropOpenClawAngleDeg,
                command.armRetreatChassisAngleDeg,
                command.armRetreatShoulderAngleDeg,
                command.armRetreatElbowAngleDeg,
                command.armRetreatWristAngleDeg,
                command.armStepDelay,
                dropOptions,
                retreatOptions
            );

            if (command.expectsJsonResponse)
            {
                if (result.result == ARM_RESULT_OK) writeJsonDropAnglesResponse(command, result);
                else writeJsonError(command, "DROP_ANGLES", "INVALID_ARGUMENT", "Invalid DROP_ANGLES arguments");
            }
            else
            {
                if (result.result == ARM_RESULT_OK)
                {
                    Serial.print("ACK DROP_ANGLES DROPPED=");
                    Serial.print(result.dropped ? "YES" : "NO");
                    Serial.print(" RETREATED=");
                    Serial.println(result.retreatedToSafePose ? "YES" : "NO");
                }
                else if (result.result == ARM_RESULT_NOT_INITIALIZED) Serial.println("ERR DROP_ANGLES NOT_INITIALIZED");
                else if (result.result == ARM_RESULT_INVALID_ARGUMENT) Serial.println("ERR DROP_ANGLES INVALID_ARGUMENT");
                else Serial.println("ERR DROP_ANGLES UNKNOWN");
            }

            break;
        }

        case CMD_MOVE_ARM_ANGLES:
        {
            ArmMotionOptions moveOptions;
            moveOptions.mode = command.armMoveMotionMode;
            moveOptions.servoOrderCount = command.armMoveServoOrderCount;
            for (int i = 0; i < moveOptions.servoOrderCount; i++)
            {
                moveOptions.servoOrder[i] = command.armMoveServoOrder[i];
            }

            ArmResult result = m_roboticArm.moveByAngles(
                command.armMoveChassisAngleDeg,
                command.armMoveShoulderAngleDeg,
                command.armMoveElbowAngleDeg,
                command.armMoveWristAngleDeg,
                command.armMoveClawAngleDeg,
                command.armStepDelay,
                moveOptions
            );

            if (command.expectsJsonResponse)
            {
                if (result == ARM_RESULT_OK) writeJsonMoveArmAnglesResponse(command);
                else writeJsonError(command, "MOVE_ARM_ANGLES", "INVALID_ARGUMENT", "Invalid MOVE_ARM_ANGLES arguments");
            }
            else
            {
                if (result == ARM_RESULT_OK) Serial.println("ACK MOVE_ARM_ANGLES OK");
                else if (result == ARM_RESULT_NOT_INITIALIZED) Serial.println("ERR MOVE_ARM_ANGLES NOT_INITIALIZED");
                else if (result == ARM_RESULT_INVALID_ARGUMENT) Serial.println("ERR MOVE_ARM_ANGLES INVALID_ARGUMENT");
                else Serial.println("ERR MOVE_ARM_ANGLES UNKNOWN");
            }

            break;
        }

        default:
            Serial.println("ERR UNKNOWN_COMMAND");
            break;
    }
}

void CommandDispatcher::handleBlink()
{
    digitalWrite(2, HIGH);
    delay(200);
    digitalWrite(2, LOW);
    delay(200);
    digitalWrite(2, HIGH);
    delay(200);
    digitalWrite(2, LOW);

    Serial.println("ACK BLINK");
}

void CommandDispatcher::handleTimedMove(CommandType commandType, int amountMs, int speed)
{
    if (amountMs <= 0) amountMs = 300;
    if (speed <= 0) speed = 50;

    switch (commandType)
    {
        case CMD_MOVE_FW:
            m_driveBase.moveForward(speed);
            break;

        case CMD_MOVE_BW:
            m_driveBase.moveBackward(speed);
            break;

        case CMD_MOVE_LEFT:
            m_driveBase.moveLeft(speed);
            break;

        case CMD_MOVE_RIGHT:
            m_driveBase.moveRight(speed);
            break;

        case CMD_ROTATE_CW:
            m_driveBase.rotateClockwise(speed);
            break;

        case CMD_ROTATE_CCW:
            m_driveBase.rotateCounterClockwise(speed);
            break;

        default:
            return;
    }

    delay(amountMs);
    m_driveBase.stop();
}

void CommandDispatcher::writeJsonOk(const Command& command, const char* commandName) const
{
    JsonDocument doc;
    doc["transactionId"] = command.transactionId;
    doc["status"] = "OK";
    doc["command"] = commandName;
    serializeJson(doc, Serial);
    Serial.println();
}

void CommandDispatcher::writeJsonError(
    const Command& command,
    const char* commandName,
    const char* code,
    const char* message
) const
{
    JsonDocument doc;
    doc["transactionId"] = command.transactionId;
    doc["status"] = "ERROR";
    doc["command"] = commandName;

    JsonObject error = doc["error"].to<JsonObject>();
    error["code"] = code;
    error["message"] = message;

    serializeJson(doc, Serial);
    Serial.println();
}

void CommandDispatcher::writeJsonUsTestResponse(const Command& command, float distanceMm) const
{
    JsonDocument doc;
    doc["transactionId"] = command.transactionId;
    doc["status"] = "OK";
    doc["command"] = "US_TEST";

    JsonObject data = doc["data"].to<JsonObject>();
    data["distanceMm"] = distanceMm;

    serializeJson(doc, Serial);
    Serial.println();
}

void CommandDispatcher::writeJsonUsScanResponse(const Command& command, const UltrasonicSweepResult& result) const
{
    JsonDocument doc;
    doc["transactionId"] = command.transactionId;
    doc["status"] = "OK";
    doc["command"] = "US_SCAN";

    JsonObject data = doc["data"].to<JsonObject>();
    data["pointCount"] = result.pointCount;
    data["stepRotateMs"] = command.usScanStepRotateMs;
    data["rotateSpeed"] = command.usScanRotateSpeed;
    data["rotation"] = (command.usScanRotation == SCAN_CW) ? "CW" : "CCW";

    JsonArray points = data["points"].to<JsonArray>();

    for (int i = 0; i < result.pointCount; i++)
    {
        JsonObject p = points.add<JsonObject>();
        p["angleDeg"] = result.points[i].angleDeg;
        p["distanceMm"] = result.points[i].distanceMm;
    }

    serializeJson(doc, Serial);
    Serial.println();
}

void CommandDispatcher::writeJsonPickAnglesResponse(const Command& command, const ArmPickResult& result) const
{
    JsonDocument doc;
    doc["transactionId"] = command.transactionId;
    doc["status"] = "OK";
    doc["command"] = "PICK_ANGLES";

    JsonObject data = doc["data"].to<JsonObject>();
    data["picked"] = result.picked;
    data["transportedToSecurePose"] = result.transportedToSecurePose;
    data["detectionMethod"] = "execution_only";

    serializeJson(doc, Serial);
    Serial.println();
}

void CommandDispatcher::writeJsonDropAnglesResponse(const Command& command, const ArmDropResult& result) const
{
    JsonDocument doc;
    doc["transactionId"] = command.transactionId;
    doc["status"] = "OK";
    doc["command"] = "DROP_ANGLES";

    JsonObject data = doc["data"].to<JsonObject>();
    data["dropped"] = result.dropped;
    data["retreatedToSafePose"] = result.retreatedToSafePose;

    serializeJson(doc, Serial);
    Serial.println();
}

void CommandDispatcher::writeJsonMoveArmAnglesResponse(const Command& command) const
{
    JsonDocument doc;
    doc["transactionId"] = command.transactionId;
    doc["status"] = "OK";
    doc["command"] = "MOVE_ARM_ANGLES";

    JsonObject data = doc["data"].to<JsonObject>();
    data["movedToPose"] = true;

    serializeJson(doc, Serial);
    Serial.println();
}

void CommandDispatcher::printReadyBanner()
{
    Serial.println("ESP32 ready");
    Serial.println("Commands:");
    Serial.println("PING");
    Serial.println("LED_ON");
    Serial.println("LED_OFF");
    Serial.println("BLINK");
    Serial.println("MOVE_FW <ms> <speed>");
    Serial.println("MOVE_BW <ms> <speed>");
    Serial.println("MOVE_LEFT <ms> <speed>");
    Serial.println("MOVE_RIGHT <ms> <speed>");
    Serial.println("ROTATE_CW <ms> <speed>");
    Serial.println("ROTATE_CCW <ms> <speed>");
    Serial.println("STOP");
    Serial.println("US_TEST");
    Serial.println("US_SCAN <scanAngleDeg> <steps> <stepRotateMs> <rotateSpeed> [CW|CCW]");
    Serial.println("PICK_OBJECT <angleDeg> <distanceMm> <heightMm>");
    Serial.println("PICK_ANGLES (JSON only)");
    Serial.println("DROP_ANGLES (JSON only)");
    Serial.println("MOVE_ARM_ANGLES (JSON only)");
}