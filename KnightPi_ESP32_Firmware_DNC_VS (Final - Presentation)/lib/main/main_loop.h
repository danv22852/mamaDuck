#ifndef MAIN_LOOP_H
#define MAIN_LOOP_H

#include <Arduino.h>
#include "types.h"
#include "drive_base.h"
#include "ultrasonic_scanner.h"
#include "robotic_arm.h"
#include "command_parser.h"
#include "command_dispatcher.h"

class MainLoop
{
public:
    MainLoop();
    void init();
    void tick();

private:
    static void serialTaskEntry(void* param);
    void serialTaskLoop();
    void handleSerialInput();

private:
    String m_inputBuffer;

    DriveBase m_driveBase;
    UltrasonicScanner m_ultrasonicScanner;
    RoboticArm m_roboticArm;

    CommandParser m_commandParser;
    CommandDispatcher m_commandDispatcher;
};

#endif