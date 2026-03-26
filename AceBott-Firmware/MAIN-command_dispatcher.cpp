#include <Arduino.h>
#include "MAIN-command_dispatcher.h"
#include "COMMON-Pins.h"
#include "COMMON-Config.h"

CommandDispatcher::CommandDispatcher(
    DriveBase& driveBase,
    UltrasonicScanner& ultrasonicScanner,
    ClawController& clawController
)
    : m_driveBase(driveBase),
    m_ultrasonicScanner(ultrasonicScanner),
    m_clawController(clawController)
{
}

void CommandDispatcher::dispatch(const Command& command)
{
    String originalCmd = command.originalText;
    originalCmd.trim();
    originalCmd.toUpperCase();

    switch (command.type)
    {
    case CMD_PING:
        Serial.println("ACK PING");
        break;

    case CMD_LED_ON:
        digitalWrite(LED_PIN, HIGH);
        Serial.println("ACK LED_ON");
        break;

    case CMD_LED_OFF:
        digitalWrite(LED_PIN, LOW);
        Serial.println("ACK LED_OFF");
        break;

    case CMD_BLINK:
        Serial.println("ACK BLINK");
        handleBlink();
        break;

    case CMD_MOVE_FW:
        handleTimedMove(CMD_MOVE_FW);
        Serial.println("ACK MOVE_FW");
        break;

    case CMD_MOVE_BW:
        handleTimedMove(CMD_MOVE_BW);
        Serial.println("ACK MOVE_BW");
        break;

    case CMD_MOVE_LEFT:
        handleTimedMove(CMD_MOVE_LEFT);
        Serial.println("ACK MOVE_LEFT");
        break;

    case CMD_MOVE_RIGHT:
        handleTimedMove(CMD_MOVE_RIGHT);
        Serial.println("ACK MOVE_RIGHT");
        break;

    case CMD_ROTATE_CW:
        handleTimedMove(CMD_ROTATE_CW);
        Serial.println("ACK ROTATE_CW");
        break;

    case CMD_ROTATE_CCW:
        handleTimedMove(CMD_ROTATE_CCW);
        Serial.println("ACK ROTATE_CCW");
        break;

    case CMD_STOP:
        m_driveBase.stop();
        Serial.println("ACK STOP");
        break;

    case CMD_US_TEST:
    {
        float distanceMm = m_ultrasonicScanner.readDistanceMm();
        Serial.print("Distance: ");
        Serial.print(distanceMm);
        Serial.println(" mm");
        Serial.println("ACK US_TEST");
        break;
    }

    case CMD_SCAN_US:
    {
        ScanResult result = m_ultrasonicScanner.scanLeftCenterRight();

        Serial.print("LEFT: ");
        Serial.print(result.leftMm);
        Serial.print(" mm, CENTER: ");
        Serial.print(result.centerMm);
        Serial.print(" mm, RIGHT: ");
        Serial.print(result.rightMm);
        Serial.println(" mm");

        Serial.println("ACK SCAN_US");
        break;
    }

    case CMD_CLAW_OPEN:
        m_clawController.open();
        Serial.print("ACK CLAW_OPEN ANGLE ");
        Serial.println(m_clawController.getAngle());
        break;

    case CMD_CLAW_CLOSE:
        m_clawController.close();
        Serial.print("ACK CLAW_CLOSE ANGLE ");
        Serial.println(m_clawController.getAngle());
        break;

    case CMD_CLAW_SET:
        if (!command.hasIntValue)
        {
            Serial.println("ERR CLAW_SET requires angle");
            break;
        }

        m_clawController.setAngle(command.intValue);
        Serial.print("ACK CLAW_SET ANGLE ");
        Serial.println(m_clawController.getAngle());
        break;

    case CMD_CLAW_STEP:
        if (!command.hasIntValue)
        {
            Serial.println("ERR CLAW_STEP requires delta");
            break;
        }

        m_clawController.step(command.intValue);
        Serial.print("ACK CLAW_STEP ANGLE ");
        Serial.println(m_clawController.getAngle());
        break;

    case CMD_CLAW_STATUS:
        Serial.print("CLAW ANGLE ");
        Serial.println(m_clawController.getAngle());
        Serial.println("ACK CLAW_STATUS");
        break;

    default:
        Serial.print("ERR UNKNOWN_CMD: ");
        Serial.println(originalCmd);
        break;
    }
}

void CommandDispatcher::handleBlink()
{
    for (int i = 0; i < 3; i++)
    {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
    }
}

void CommandDispatcher::handleTimedMove(CommandType commandType)
{
    switch (commandType)
    {
    case CMD_MOVE_FW:
        m_driveBase.moveForward(DEFAULT_MOVE_SPEED);
        break;

    case CMD_MOVE_BW:
        m_driveBase.moveBackward(DEFAULT_MOVE_SPEED);
        break;

    case CMD_MOVE_LEFT:
        m_driveBase.moveLeft(DEFAULT_MOVE_SPEED);
        break;

    case CMD_MOVE_RIGHT:
        m_driveBase.moveRight(DEFAULT_MOVE_SPEED);
        break;

    case CMD_ROTATE_CW:
        m_driveBase.rotateClockwise(DEFAULT_MOVE_SPEED);
        break;

    case CMD_ROTATE_CCW:
        m_driveBase.rotateCounterClockwise(DEFAULT_MOVE_SPEED);
        break;

    default:
        return;
    }

    delay(DEFAULT_MOVE_DURATION_MS);
    m_driveBase.stop();
}