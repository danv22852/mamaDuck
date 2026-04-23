#include "event_bus.h"

EventBus& EventBus::instance()
{
    static EventBus s_instance;
    return s_instance;
}

EventBus::EventBus()
    : m_mutex(nullptr)
{
    for (int i = 0; i < MAX_EVENT_SUBSCRIBERS; i++)
    {
        m_subscribers[i] = nullptr;
    }
}

void EventBus::begin()
{
    if (m_mutex == nullptr)
    {
        m_mutex = xSemaphoreCreateMutex();
    }
}

bool EventBus::subscribe(QueueHandle_t queue)
{
    if (queue == nullptr) return false;

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    for (int i = 0; i < MAX_EVENT_SUBSCRIBERS; i++)
    {
        if (m_subscribers[i] == queue)
        {
            xSemaphoreGive(m_mutex);
            return true;
        }
    }

    for (int i = 0; i < MAX_EVENT_SUBSCRIBERS; i++)
    {
        if (m_subscribers[i] == nullptr)
        {
            m_subscribers[i] = queue;
            xSemaphoreGive(m_mutex);
            return true;
        }
    }

    xSemaphoreGive(m_mutex);
    return false;
}

void EventBus::unsubscribe(QueueHandle_t queue)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);

    for (int i = 0; i < MAX_EVENT_SUBSCRIBERS; i++)
    {
        if (m_subscribers[i] == queue)
        {
            m_subscribers[i] = nullptr;
        }
    }

    xSemaphoreGive(m_mutex);
}

void EventBus::publish(const RobotEvent& event)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);

    for (int i = 0; i < MAX_EVENT_SUBSCRIBERS; i++)
    {
        if (m_subscribers[i] != nullptr)
        {
            xQueueSend(m_subscribers[i], &event, 0);
        }
    }

    xSemaphoreGive(m_mutex);
}