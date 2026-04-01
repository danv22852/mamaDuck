#include <Arduino.h>
#include "main_loop.h"
#include "pins.h"
#include "config.h"


MainLoop::MainLoop()
    : m_inputBuffer(""),
      m_driveBase(),
      m_ultrasonicScanner(),
      m_commandParser(),
      m_commandDispatcher(m_driveBase, m_ultrasonicScanner)
{
}

void MainLoop::init()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.begin(SERIAL_BAUD_RATE);
    delay(STARTUP_DELAY_MS);

    m_driveBase.init();
    m_ultrasonicScanner.init(ULTRASONIC_TRIG_PIN, ULTRASONIC_ECHO_PIN, SERVO_PIN);

    m_ultrasonicScanner.readDistanceMm();

    // startup sweep kept from original test sketch
    delay(1000);

    Serial.println("ESP32 ready");
    Serial.println("Commands: PING, LED_ON, LED_OFF, BLINK, MOVE_FW, MOVE_BW, MOVE_LEFT, MOVE_RIGHT, ROTATE_CW, ROTATE_CCW, STOP, US_TEST, SCAN_US");
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