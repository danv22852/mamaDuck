#ifndef MAIN_LOOP_H
#define MAIN_LOOP_H

#include <Arduino.h>
#include "COMMON-Types.h"
#include "COMPONENTS-drive_base.h"
#include "COMPONENTS-ultrasonic_scanner.h"
#include "COMPONENTS-claw_controller.h"
#include "COMMS-command_parser.h"
#include "MAIN-command_dispatcher.h"

class MainLoop
{
public:
    MainLoop();
    void init();
    void tick();

private:
    void handleSerialInput();

private:
    String m_inputBuffer;
    DriveBase m_driveBase;
    UltrasonicScanner m_ultrasonicScanner;
    ClawController m_clawController;
    CommandParser m_commandParser;
    CommandDispatcher m_commandDispatcher;
};

#endif