#pragma once

#include <Engine/Inc/Engine.h>

enum class GameEventType
{
    PressSpace = 1,
    PressEnter
};

class PressSpaceEvent : public Engine::Core::Event
{
  public:
    PressSpaceEvent()
    {
    }
    SET_EVENT_TYPE_ID(GameEventType::PressSpace)
};

class PressEnterEvent : public Engine::Core::Event
{
  public:
    PressEnterEvent()
    {
    }
    SET_EVENT_TYPE_ID(GameEventType::PressEnter)
};
