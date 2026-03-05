#include "Precompiled.h"
#include "EventManager.h"
#include "DebugUtil.h"

using namespace Engine;
using namespace Engine::Core;

namespace
{
std::unique_ptr<EventManager> sEventManager;
}

void EventManager::StaticInitialize()
{
    ASSERT(sEventManager == nullptr, "EventManager: Is already initialized!");
    sEventManager = std::make_unique<EventManager>();
    sEventManager->Initialize();
}

void EventManager::StaticTerminate()
{
    if (sEventManager != nullptr)
    {
        sEventManager->Terminate();
        sEventManager.reset();
    }
}

EventManager* EventManager::Get()
{
    ASSERT(sEventManager != nullptr, "EventManager: Isn't initialized!");
    return sEventManager.get();
}

void EventManager::Broadcast(const Event& e)
{
    sEventManager->BroadcastPrivate(e);
}

EventManager::~EventManager()
{
    ASSERT(mEventListeners.empty(), "EventManager: Terminate must be called!");
}

void EventManager::Initialize()
{
    mEventListeners.clear();
}

void EventManager::Terminate()
{
    mEventListeners.clear();
}

EventListenerId EventManager::AddListener(EventTypeId eventId, const EventCallback& cb)
{
    ++mNextListenerId;
    mEventListeners[eventId][mNextListenerId] = cb;
    return mNextListenerId;
}

void EventManager::RemoveListener(EventTypeId eventId, EventListenerId listenerId)
{
    auto eventGroupListeners = mEventListeners.find(eventId);
    if (eventGroupListeners != mEventListeners.end())
    {
        auto listener = eventGroupListeners->second.find(listenerId);
        if (listener != eventGroupListeners->second.end())
        {
            eventGroupListeners->second.erase(listener);
        }
    }
}

void EventManager::BroadcastPrivate(const Event& e)
{
    auto eventGroupListeners = mEventListeners.find(e.GetTypeId());
    if (eventGroupListeners != mEventListeners.end())
    {
        for (auto& cb : eventGroupListeners->second)
        {
            cb.second(e);
        }
    }
}
