#include <ArduinoJson.h>
#include "command_dispatcher.h"
#include "../common/config.h"
#include "../common/runtime_state.h"
#include "../common/event_bus.h"

CommandDispatcher::CommandDispatcher(
    DriveBase& driveBase,
    UltrasonicScanner& ultrasonicScanner,
    RoboticArm& roboticArm
)
    : m_commandQueue(nullptr),
      m_commandEventQueue(nullptr),
      m_serialEventQueue(nullptr),
      m_driveBase(driveBase),
      m_ultrasonicScanner(ultrasonicScanner),
      m_roboticArm(roboticArm)
{
}

void CommandDispatcher::init()
{
    if (m_commandQueue == nullptr)
    {
        m_commandQueue = xQueueCreate(COMMAND_QUEUE_LENGTH, sizeof(Command));
    }

    if (m_commandEventQueue == nullptr)
    {
        m_commandEventQueue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(RobotEvent));
        EventBus::instance().subscribe(m_commandEventQueue);
    }

    if (m_serialEventQueue == nullptr)
    {
        m_serialEventQueue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(RobotEvent));
        EventBus::instance().subscribe(m_serialEventQueue);
    }

    xTaskCreatePinnedToCore(
        CommandDispatcher::commandTaskEntry,
        "command_exec_task",
        12288,
        this,
        2,
        nullptr,
        1
    );

    xTaskCreatePinnedToCore(
        CommandDispatcher::serialNotificationTaskEntry,
        "serial_notify_task",
        6144,
        this,
        1,
        nullptr,
        1
    );
}

bool CommandDispatcher::submit(const Command& command)
{
    if (m_commandQueue == nullptr) return false;
    return xQueueSend(m_commandQueue, &command, 0) == pdTRUE;
}

bool CommandDispatcher::isImmediateCommand(CommandType type) const
{
    return type == CMD_STATUS ||
           type == CMD_CANCEL ||
           type == CMD_STOP ||
           type == CMD_US_MONITOR_ON ||
           type == CMD_US_MONITOR_OFF;
}

void CommandDispatcher::drainQueue(QueueHandle_t queue)
{
    if (queue == nullptr) return;

    RobotEvent ignored;
    while (xQueueReceive(queue, &ignored, 0) == pdTRUE)
    {
    }
}

void CommandDispatcher::drainPendingObstacleEvents()
{
    drainQueue(m_commandEventQueue);
    drainQueue(m_serialEventQueue);
}

void CommandDispatcher::stopDriveAndClearRuntime()
{
    RuntimeState& state = RuntimeState::instance();
    m_driveBase.stop();
    state.setDriveActive(false, CMD_UNKNOWN);
    state.clearDriveCancel();
}

bool CommandDispatcher::isObstacleCurrentlyBlockingDrive() const
{
    RuntimeSnapshot snapshot;
    RuntimeState::instance().snapshot(snapshot);

    return snapshot.ultrasonicMonitorEnabled &&
           snapshot.ultrasonicObstacleActive;
}

void CommandDispatcher::dispatchImmediate(const Command& command)
{
    RuntimeState& state = RuntimeState::instance();

    switch (command.type)
    {
        case CMD_STATUS:
            writeJsonStatusResponse(command);
            break;

        case CMD_STOP:
            state.requestCancelDrive();
            stopDriveAndClearRuntime();
            drainPendingObstacleEvents();
            writeJsonOk(command, "STOP");
            break;

        case CMD_CANCEL:
            if (command.cancelTarget == CANCEL_TARGET_DRIVE)
            {
                state.requestCancelDrive();
                stopDriveAndClearRuntime();
                drainPendingObstacleEvents();
            }
            else if (command.cancelTarget == CANCEL_TARGET_ARM)
            {
                state.requestCancelArm();
            }
            else
            {
                state.requestCancelAll();
                stopDriveAndClearRuntime();
                drainPendingObstacleEvents();
            }

            writeJsonOk(command, "CANCEL");
            break;

        case CMD_US_MONITOR_ON:
        {
            if (command.usMonitorAlarmDistanceMm > 0.0f)
            {
                state.setUltrasonicAlarmDistanceMm(command.usMonitorAlarmDistanceMm);
            }

            drainPendingObstacleEvents();
            state.setUltrasonicMonitorEnabled(true);

            JsonDocument doc;
            doc["transactionId"] = command.transactionId;
            doc["status"] = "OK";
            doc["command"] = "US_MONITOR_ON";

            JsonObject data = doc["data"].to<JsonObject>();
            data["enabled"] = true;
            data["alarmDistanceMm"] = state.getUltrasonicAlarmDistanceMm();

            serializeJson(doc, Serial);
            Serial.println();
            break;
        }

        case CMD_US_MONITOR_OFF:
        {
            state.setUltrasonicMonitorEnabled(false);
            drainPendingObstacleEvents();

            JsonDocument doc;
            doc["transactionId"] = command.transactionId;
            doc["status"] = "OK";
            doc["command"] = "US_MONITOR_OFF";

            JsonObject data = doc["data"].to<JsonObject>();
            data["enabled"] = false;

            serializeJson(doc, Serial);
            Serial.println();
            break;
        }

        default:
            writeJsonError(command, "UNKNOWN", "INVALID_REQUEST", "Unsupported immediate command");
            break;
    }
}

void CommandDispatcher::commandTaskEntry(void* param)
{
    CommandDispatcher* self = static_cast<CommandDispatcher*>(param);
    self->commandTaskLoop();
}

void CommandDispatcher::serialNotificationTaskEntry(void* param)
{
    CommandDispatcher* self = static_cast<CommandDispatcher*>(param);
    self->serialNotificationTaskLoop();
}

void CommandDispatcher::commandTaskLoop()
{
    while (true)
    {
        Command command;
        if (xQueueReceive(m_commandQueue, &command, portMAX_DELAY) == pdTRUE)
        {
            executeQueuedCommand(command);
        }
    }
}

void CommandDispatcher::serialNotificationTaskLoop()
{
    while (true)
    {
        RobotEvent event;

        if (xQueueReceive(m_serialEventQueue, &event, portMAX_DELAY) == pdTRUE)
        {
            if (event.type == EVENT_US_OBSTACLE_DETECTED)
            {
                JsonDocument doc;
                doc["type"] = "NOTIFICATION";
                doc["event"] = "US_MONITOR_ALARM";

                JsonObject data = doc["data"].to<JsonObject>();
                data["sequence"] = event.sequence;
                data["distanceMm"] = event.distanceMm;
                data["alarmDistanceMm"] = event.alarmDistanceMm;

                serializeJson(doc, Serial);
                Serial.println();
            }
        }
    }
}

bool CommandDispatcher::canStartDriveTask() const
{
    RuntimeSnapshot snapshot;
    RuntimeState::instance().snapshot(snapshot);
    return !snapshot.armActive && !snapshot.ultrasonicSweepActive;
}

bool CommandDispatcher::canStartArmTask() const
{
    RuntimeSnapshot snapshot;
    RuntimeState::instance().snapshot(snapshot);
    return !snapshot.driveActive && !snapshot.ultrasonicSweepActive;
}

bool CommandDispatcher::receiveAnyObstacleEvent(unsigned long timeoutMs, RobotEvent& outEvent)
{
    unsigned long timeoutTicks = pdMS_TO_TICKS(timeoutMs);

    while (xQueueReceive(m_commandEventQueue, &outEvent, timeoutTicks) == pdTRUE)
    {
        if (outEvent.type == EVENT_US_OBSTACLE_DETECTED)
        {
            return true;
        }

        timeoutTicks = 0;
    }

    return false;
}

bool CommandDispatcher::receiveObstacleDetectedEventAfterSequence(
    unsigned long timeoutMs,
    unsigned long baselineSequence,
    RobotEvent& outEvent
)
{
    unsigned long timeoutTicks = pdMS_TO_TICKS(timeoutMs);

    while (xQueueReceive(m_commandEventQueue, &outEvent, timeoutTicks) == pdTRUE)
    {
        if (outEvent.type == EVENT_US_OBSTACLE_DETECTED &&
            outEvent.sequence > baselineSequence)
        {
            return true;
        }

        timeoutTicks = 0;
    }

    return false;
}

bool CommandDispatcher::runTimedMove(const Command& command)
{
    if (!canStartDriveTask()) return false;
    if (isObstacleCurrentlyBlockingDrive()) return false;

    RuntimeState& state = RuntimeState::instance();

    drainQueue(m_commandEventQueue);

    RuntimeSnapshot startSnapshot;
    state.snapshot(startSnapshot);
    unsigned long baselineSequence = startSnapshot.ultrasonicAlarmSequence;

    state.clearDriveCancel();
    state.setDriveActive(true, command.type);

    int amountMs = command.moveAmountMs > 0 ? command.moveAmountMs : 300;
    int speed = command.moveSpeed > 0 ? command.moveSpeed : 50;

    switch (command.type)
    {
        case CMD_MOVE_FW: m_driveBase.moveForward(speed); break;
        case CMD_MOVE_BW: m_driveBase.moveBackward(speed); break;
        case CMD_MOVE_LEFT: m_driveBase.moveLeft(speed); break;
        case CMD_MOVE_RIGHT: m_driveBase.moveRight(speed); break;
        case CMD_ROTATE_CW: m_driveBase.rotateClockwise(speed); break;
        case CMD_ROTATE_CCW: m_driveBase.rotateCounterClockwise(speed); break;
        default:
            state.setDriveActive(false, CMD_UNKNOWN);
            return false;
    }

    unsigned long startedAt = millis();

    while ((millis() - startedAt) < (unsigned long)amountMs)
    {
        if (state.shouldCancelDrive())
        {
            stopDriveAndClearRuntime();
            drainQueue(m_commandEventQueue);
            return true;
        }

        RuntimeSnapshot loopSnapshot;
        state.snapshot(loopSnapshot);

        if (loopSnapshot.ultrasonicMonitorEnabled &&
            loopSnapshot.ultrasonicObstacleActive &&
            loopSnapshot.ultrasonicAlarmSequence > baselineSequence)
        {
            stopDriveAndClearRuntime();
            drainQueue(m_commandEventQueue);
            return true;
        }

        RobotEvent event;
        if (receiveObstacleDetectedEventAfterSequence(DRIVE_SLICE_MS, baselineSequence, event))
        {
            stopDriveAndClearRuntime();
            drainQueue(m_commandEventQueue);
            return true;
        }
    }

    stopDriveAndClearRuntime();
    drainQueue(m_commandEventQueue);
    return true;
}

void CommandDispatcher::executeQueuedCommand(const Command& command)
{
    RuntimeState& state = RuntimeState::instance();

    switch (command.type)
    {
        case CMD_PING:
            writeJsonOk(command, "PING");
            break;

        case CMD_MOVE_FW:
        case CMD_MOVE_BW:
        case CMD_MOVE_LEFT:
        case CMD_MOVE_RIGHT:
        case CMD_ROTATE_CW:
        case CMD_ROTATE_CCW:
        {
            if (!canStartDriveTask())
            {
                writeJsonError(command, "MOVE", "BUSY", "Drive task cannot start while arm or sweep is active");
                break;
            }

            if (isObstacleCurrentlyBlockingDrive())
            {
                writeJsonError(command, "MOVE", "OBSTACLE_ACTIVE", "Drive command rejected because ultrasonic monitor currently sees an obstacle within alarm distance");
                break;
            }

            bool ok = runTimedMove(command);

            if (!ok)
            {
                RuntimeSnapshot snapshot;
                state.snapshot(snapshot);

                if (snapshot.ultrasonicMonitorEnabled && snapshot.ultrasonicObstacleActive)
                {
                    writeJsonError(command, "MOVE", "OBSTACLE_ACTIVE", "Drive command rejected because ultrasonic monitor currently sees an obstacle within alarm distance");
                }
                else
                {
                    writeJsonError(command, "MOVE", "BUSY", "Drive task cannot start while arm or sweep is active");
                }
            }
            else
            {
                const char* name =
                    command.type == CMD_MOVE_FW ? "MOVE_FW" :
                    command.type == CMD_MOVE_BW ? "MOVE_BW" :
                    command.type == CMD_MOVE_LEFT ? "MOVE_LEFT" :
                    command.type == CMD_MOVE_RIGHT ? "MOVE_RIGHT" :
                    command.type == CMD_ROTATE_CW ? "ROTATE_CW" : "ROTATE_CCW";

                writeJsonOk(command, name);
            }

            break;
        }

        case CMD_US_SCAN:
        {
            if (!canStartDriveTask())
            {
                writeJsonError(command, "US_SCAN", "BUSY", "US_SCAN needs exclusive drivetrain ownership");
                break;
            }

            UltrasonicSweepResult result;
            state.clearDriveCancel();
            state.setUltrasonicSweepActive(true);
            state.setUltrasonicMonitorPausedForSweep(true);

            bool ok = m_ultrasonicScanner.usScan(
                command.usScanAngleDeg,
                command.usScanSteps,
                command.usScanStepRotateMs,
                command.usScanRotateSpeed,
                command.usScanRotation,
                result
            );

            state.setUltrasonicMonitorPausedForSweep(false);
            state.setUltrasonicSweepActive(false);

            if (!ok)
            {
                writeJsonError(command, "US_SCAN", "INVALID_ARGUMENT", "Invalid US_SCAN arguments or scan cancelled");
                break;
            }

            writeJsonUsScanResponse(command, result);
            break;
        }

        case CMD_PICK_ANGLES_SPEED:
        {
            if (!canStartArmTask())
            {
                writeJsonError(command, "PICK_ANGLES_SPEED", "BUSY", "Arm requires robot drive to be idle");
                break;
            }

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

            state.clearArmCancel();
            state.setArmActive(true, command.type);

            ArmPickResult result = m_roboticArm.pickByAnglesWithSpeed(
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
                command.armPickupSpeed,
                command.armLiftSpeed,
                pickupOptions,
                liftOptions
            );

            state.setArmActive(false, CMD_UNKNOWN);

            if (result.result == ARM_RESULT_OK)
            {
                writeJsonPickAnglesSpeedResponse(command, result);
            }
            else if (result.result == ARM_RESULT_CANCELLED)
            {
                writeJsonError(command, "PICK_ANGLES_SPEED", "CANCELLED", "Operation cancelled");
            }
            else if (result.result == ARM_RESULT_BUSY)
            {
                writeJsonError(command, "PICK_ANGLES_SPEED", "BUSY", "Arm busy");
            }
            else if (result.result == ARM_RESULT_NOT_INITIALIZED)
            {
                writeJsonError(command, "PICK_ANGLES_SPEED", "NOT_INITIALIZED", "Arm not initialized");
            }
            else
            {
                writeJsonError(command, "PICK_ANGLES_SPEED", "INVALID_ARGUMENT", "Invalid PICK_ANGLES_SPEED arguments");
            }

            break;
        }

        case CMD_DROP_ANGLES_SPEED:
        {
            if (!canStartArmTask())
            {
                writeJsonError(command, "DROP_ANGLES_SPEED", "BUSY", "Arm requires robot drive to be idle");
                break;
            }

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

            state.clearArmCancel();
            state.setArmActive(true, command.type);

            ArmDropResult result = m_roboticArm.dropByAnglesWithSpeed(
                command.armDropChassisAngleDeg,
                command.armDropShoulderAngleDeg,
                command.armDropElbowAngleDeg,
                command.armDropWristAngleDeg,
                command.armDropOpenClawAngleDeg,
                command.armRetreatChassisAngleDeg,
                command.armRetreatShoulderAngleDeg,
                command.armRetreatElbowAngleDeg,
                command.armRetreatWristAngleDeg,
                command.armDropSpeed,
                command.armRetreatSpeed,
                dropOptions,
                retreatOptions
            );

            state.setArmActive(false, CMD_UNKNOWN);

            if (result.result == ARM_RESULT_OK)
            {
                writeJsonDropAnglesSpeedResponse(command, result);
            }
            else if (result.result == ARM_RESULT_CANCELLED)
            {
                writeJsonError(command, "DROP_ANGLES_SPEED", "CANCELLED", "Operation cancelled");
            }
            else if (result.result == ARM_RESULT_BUSY)
            {
                writeJsonError(command, "DROP_ANGLES_SPEED", "BUSY", "Arm busy");
            }
            else if (result.result == ARM_RESULT_NOT_INITIALIZED)
            {
                writeJsonError(command, "DROP_ANGLES_SPEED", "NOT_INITIALIZED", "Arm not initialized");
            }
            else
            {
                writeJsonError(command, "DROP_ANGLES_SPEED", "INVALID_ARGUMENT", "Invalid DROP_ANGLES_SPEED arguments");
            }

            break;
        }

        case CMD_MOVE_ARM_ANGLES_SPEED:
        {
            if (!canStartArmTask())
            {
                writeJsonError(command, "MOVE_ARM_ANGLES_SPEED", "BUSY", "Arm requires robot drive to be idle");
                break;
            }

            ArmMotionOptions moveOptions;
            moveOptions.mode = command.armMoveMotionMode;
            moveOptions.servoOrderCount = command.armMoveServoOrderCount;
            for (int i = 0; i < moveOptions.servoOrderCount; i++)
            {
                moveOptions.servoOrder[i] = command.armMoveServoOrder[i];
            }

            state.clearArmCancel();
            state.setArmActive(true, command.type);

            ArmResult result = m_roboticArm.moveByAnglesWithSpeed(
                command.armMoveChassisAngleDeg,
                command.armMoveShoulderAngleDeg,
                command.armMoveElbowAngleDeg,
                command.armMoveWristAngleDeg,
                command.armMoveClawAngleDeg,
                command.armMoveSpeed,
                moveOptions
            );

            state.setArmActive(false, CMD_UNKNOWN);

            if (result == ARM_RESULT_OK)
            {
                writeJsonMoveArmAnglesSpeedResponse(command);
            }
            else if (result == ARM_RESULT_CANCELLED)
            {
                writeJsonError(command, "MOVE_ARM_ANGLES_SPEED", "CANCELLED", "Operation cancelled");
            }
            else if (result == ARM_RESULT_BUSY)
            {
                writeJsonError(command, "MOVE_ARM_ANGLES_SPEED", "BUSY", "Arm busy");
            }
            else if (result == ARM_RESULT_NOT_INITIALIZED)
            {
                writeJsonError(command, "MOVE_ARM_ANGLES_SPEED", "NOT_INITIALIZED", "Arm not initialized");
            }
            else
            {
                writeJsonError(command, "MOVE_ARM_ANGLES_SPEED", "INVALID_ARGUMENT", "Invalid MOVE_ARM_ANGLES_SPEED arguments");
            }

            break;
        }

        default:
            writeJsonError(command, "UNKNOWN", "UNKNOWN_COMMAND", "Unknown command");
            break;
    }
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

void CommandDispatcher::writeJsonError(const Command& command, const char* commandName, const char* code, const char* message) const
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

void CommandDispatcher::writeJsonPickAnglesSpeedResponse(const Command& command, const ArmPickResult& result) const
{
    JsonDocument doc;
    doc["transactionId"] = command.transactionId;
    doc["status"] = "OK";
    doc["command"] = "PICK_ANGLES_SPEED";

    JsonObject data = doc["data"].to<JsonObject>();
    data["picked"] = result.picked;
    data["transportedToSecurePose"] = result.transportedToSecurePose;
    data["detectionMethod"] = "execution_only";

    serializeJson(doc, Serial);
    Serial.println();
}

void CommandDispatcher::writeJsonDropAnglesSpeedResponse(const Command& command, const ArmDropResult& result) const
{
    JsonDocument doc;
    doc["transactionId"] = command.transactionId;
    doc["status"] = "OK";
    doc["command"] = "DROP_ANGLES_SPEED";

    JsonObject data = doc["data"].to<JsonObject>();
    data["dropped"] = result.dropped;
    data["retreatedToSafePose"] = result.retreatedToSafePose;

    serializeJson(doc, Serial);
    Serial.println();
}

void CommandDispatcher::writeJsonMoveArmAnglesSpeedResponse(const Command& command) const
{
    JsonDocument doc;
    doc["transactionId"] = command.transactionId;
    doc["status"] = "OK";
    doc["command"] = "MOVE_ARM_ANGLES_SPEED";

    JsonObject data = doc["data"].to<JsonObject>();
    data["movedToPose"] = true;

    serializeJson(doc, Serial);
    Serial.println();
}

void CommandDispatcher::writeJsonStatusResponse(const Command& command) const
{
    RuntimeSnapshot snapshot;
    RuntimeState::instance().snapshot(snapshot);

    JsonDocument doc;
    doc["transactionId"] = command.transactionId;
    doc["status"] = "OK";
    doc["command"] = "STATUS";

    JsonObject data = doc["data"].to<JsonObject>();
    data["driveActive"] = snapshot.driveActive;
    data["armActive"] = snapshot.armActive;
    data["ultrasonicSweepActive"] = snapshot.ultrasonicSweepActive;
    data["ultrasonicMonitorEnabled"] = snapshot.ultrasonicMonitorEnabled;
    data["ultrasonicMonitorPausedForSweep"] = snapshot.ultrasonicMonitorPausedForSweep;
    data["ultrasonicObstacleActive"] = snapshot.ultrasonicObstacleActive;
    data["latestDistanceMm"] = snapshot.latestDistanceMm;
    data["ultrasonicAlarmDistanceMm"] = snapshot.ultrasonicAlarmDistanceMm;
    data["ultrasonicObstacleDistanceMm"] = snapshot.ultrasonicObstacleDistanceMm;
    data["ultrasonicAlarmSequence"] = snapshot.ultrasonicAlarmSequence;
    data["cancelDriveRequested"] = snapshot.cancelDriveRequested;
    data["cancelArmRequested"] = snapshot.cancelArmRequested;
    data["driveCommand"] = (int)snapshot.driveCommand;
    data["armCommand"] = (int)snapshot.armCommand;

    serializeJson(doc, Serial);
    Serial.println();
}