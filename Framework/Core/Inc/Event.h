#pragma once

namespace Engine::Core
{
using EventTypeId = std::size_t;

class Event
{
  public:
    Event() = default;
    virtual ~Event() = default;

    virtual EventTypeId GetTypeId() const = 0;
};
} // namespace Engine::Core

#define SET_EVENT_TYPE_ID(id)                                                                      \
    static Engine::Core::EventTypeId StaticGetTypeId()                                             \
    {                                                                                              \
        return static_cast<Engine::Core::EventTypeId>(id);                                         \
    }                                                                                              \
    Engine::Core::EventTypeId GetTypeId() const override                                           \
    {                                                                                              \
        return StaticGetTypeId();                                                                  \
    }
