#include <Arduino.h>
#include <ArduinoJson.h>
#include "main_loop.h"
#include "pins.h"
#include "config.h"
#include "runtime_state.h"
#include "event_bus.h"

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
    Serial.begin(SERIAL_BAUD_RATE);
    delay(STARTUP_DELAY_MS);

    RuntimeState::instance().begin();
    EventBus::instance().begin();

    m_driveBase.init();
    m_ultrasonicScanner.init(ULTRASONIC_TRIG_PIN, ULTRASONIC_ECHO_PIN, &m_driveBase);
    m_ultrasonicScanner.startBackgroundMonitorTask();

    m_roboticArm.init(25, 26, 27, 33, 4);

    m_commandDispatcher.init();

    xTaskCreatePinnedToCore(
        MainLoop::serialTaskEntry,
        "serial_rx_task",
        4096,
        this,
        2,
        nullptr,
        1
    );

    Serial.println("ESP32 ready");
    Serial.println("JSON requests only");
    Serial.println("Supported commands:");
    Serial.println("  PING");
    Serial.println("  STATUS");
    Serial.println("  CANCEL");
    Serial.println("  STOP");
    Serial.println("  MOVE_FW");
    Serial.println("  MOVE_BW");
    Serial.println("  MOVE_LEFT");
    Serial.println("  MOVE_RIGHT");
    Serial.println("  ROTATE_CW");
    Serial.println("  ROTATE_CCW");
    Serial.println("  US_SCAN");
    Serial.println("  US_MONITOR_ON");
    Serial.println("  US_MONITOR_OFF");
    Serial.println("  PICK_ANGLES_SPEED");
    Serial.println("  DROP_ANGLES_SPEED");
    Serial.println("  MOVE_ARM_ANGLES_SPEED");
}

void MainLoop::tick()
{
    vTaskDelay(pdMS_TO_TICKS(20));
}

void MainLoop::serialTaskEntry(void* param)
{
    MainLoop* self = static_cast<MainLoop*>(param);
    self->serialTaskLoop();
}

void MainLoop::serialTaskLoop()
{
    while (true)
    {
        handleSerialInput();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
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
                String raw = m_inputBuffer;
                raw.trim();

                if (!raw.startsWith("{"))
                {
                    JsonDocument doc;
                    doc["status"] = "ERROR";

                    JsonObject error = doc["error"].to<JsonObject>();
                    error["code"] = "JSON_REQUIRED";
                    error["message"] = "Only JSON requests are supported";

                    serializeJson(doc, Serial);
                    Serial.println();
                }
                else
                {
                    Command command = m_commandParser.parse(raw);

                    if (command.type == CMD_UNKNOWN)
                    {
                        JsonDocument doc;
                        doc["transactionId"] = command.transactionId;
                        doc["status"] = "ERROR";
                        doc["command"] = "UNKNOWN";

                        JsonObject error = doc["error"].to<JsonObject>();
                        error["code"] = "INVALID_REQUEST";
                        error["message"] = "Unknown or invalid JSON command";

                        serializeJson(doc, Serial);
                        Serial.println();
                    }
                    else if (m_commandDispatcher.isImmediateCommand(command.type))
                    {
                        m_commandDispatcher.dispatchImmediate(command);
                    }
                    else
                    {
                        if (!m_commandDispatcher.submit(command))
                        {
                            JsonDocument doc;
                            doc["transactionId"] = command.transactionId;
                            doc["status"] = "ERROR";
                            doc["command"] = "QUEUE";

                            JsonObject error = doc["error"].to<JsonObject>();
                            error["code"] = "COMMAND_QUEUE_FULL";
                            error["message"] = "Command queue is full";

                            serializeJson(doc, Serial);
                            Serial.println();
                        }
                    }
                }

                m_inputBuffer = "";
            }
        }
        else if (c != '\r')
        {
            m_inputBuffer += c;
        }
    }
}