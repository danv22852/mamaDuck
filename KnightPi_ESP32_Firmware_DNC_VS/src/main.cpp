#include <Arduino.h>
#include "main_loop.h"

MainLoop g_mainLoop;

void setup()
{
    g_mainLoop.init();
}

void loop()
{
    g_mainLoop.tick();
}