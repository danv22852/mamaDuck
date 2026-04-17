#include <Arduino.h>
#include "main_loop.h"
#include "pins.h"
#include "config.h"

MainLoop::MainLoop()
    : m_inputBuffer(""),
      m_driveBase(),
      m_ultrasonicScanner(),
      m_roboticArm(),
      m_commandParser(),
      m_commandDispatcher(m_driveBase, m_ultrasonicScanner, m_roboticArm)
{
}

void MainLoop::init()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.begin(SERIAL_BAUD_RATE);
    delay(STARTUP_DELAY_MS);

    m_driveBase.init();
    m_ultrasonicScanner.init(ULTRASONIC_TRIG_PIN, ULTRASONIC_ECHO_PIN, &m_driveBase);

    m_ultrasonicScanner.readDistanceMm();

    m_roboticArm.init(25, 26, 27, 33, 4);

    delay(1000);

    Serial.println("ESP32 ready");
    Serial.println("Commands: ");
    Serial.println("  PING");
    Serial.println("  LED_ON");
    Serial.println("  LED_OFF");
    Serial.println("  BLINK");
    Serial.println("  MOVE_FW  <duration> <speed> ");
    Serial.println("  MOVE_BW  <duration> <speed> ");  
    Serial.println("  MOVE_LEFT  <duration> <speed> ");
    Serial.println("  MOVE_RIGHT  <duration> <speed> "); 
    Serial.println("  ROTATE_CW  <duration> <speed> ");
    Serial.println("  ROTATE_CCW  <duration> <speed> ");
    Serial.println("  STOP");
    Serial.println("  US_TEST");
    Serial.println("  US_SCAN <scanAngleDeg> <steps> <stepRotateMs> <rotateSpeed> [CW|CCW]");
    Serial.println("  PICK_OBJECT <angleDeg> <distanceMm> <heightMm>");
    Serial.println("  PICK_ANGLES (JSON only)");
    Serial.println("  DROP_ANGLES (JSON only)");
    Serial.println("  MOVE_ARM_ANGLES (JSON only)");
}

void MainLoop::tick()
{
    handleSerialInput();
}

void MainLoop::handleSerialInput()
{
    while (Serial.available() > 0)
    {
        char c = Serial.read();

        if (c == '\n')
        {
            if (m_inputBuffer.length() > 0)
            {
                Command command = m_commandParser.parse(m_inputBuffer);
                m_commandDispatcher.dispatch(command);
                m_inputBuffer = "";
            }
        }
        else if (c != '\r')
        {
            m_inputBuffer += c;
        }
    }
}