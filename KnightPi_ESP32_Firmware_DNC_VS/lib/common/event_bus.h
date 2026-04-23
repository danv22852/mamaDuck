#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <Arduino.h>
#include "types.h"

enum RobotEventType
{
    EVENT_NONE = 0,
    EVENT_US_OBSTACLE_DETECTED,
    EVENT_US_OBSTACLE_CLEARED
};

struct RobotEvent
{
    RobotEventType type;
    float distanceMm;
    float alarmDistanceMm;
    unsigned long sequence;

    RobotEvent()
        : type(EVENT_NONE),
          distanceMm(0.0f),
          alarmDistanceMm(0.0f),
          sequence(0)
    {
    }
};

class EventBus
{
public:
    static EventBus& instance();

    void begin();
    bool subscribe(QueueHandle_t queue);
    void unsubscribe(QueueHandle_t queue);
    void publish(const RobotEvent& event);

private:
    EventBus();

private:
    SemaphoreHandle_t m_mutex;
    QueueHandle_t m_subscribers[MAX_EVENT_SUBSCRIBERS];
};

#endif