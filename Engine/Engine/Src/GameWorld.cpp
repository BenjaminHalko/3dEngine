#include "Precompiled.h"
#include "GameWorld.h"
#include "GameObjectFactory.h"

using namespace Engine;

void GameWorld::Initialize(uint32_t capacity)
{
    ASSERT(!mInitialized, "GameWorld: Already initialized!");

    for (auto& service : mServices)
    {
        service->Initialize();
    }

    mGameObjectSlots.resize(capacity);
    mFreeSlots.resize(capacity);
    std::iota(mFreeSlots.begin(), mFreeSlots.end(), 0);

    mInitialized = true;
}

void GameWorld::Terminate()
{
    for (Slot& slot : mGameObjectSlots)
    {
        if (slot.gameObject != nullptr)
        {
            slot.gameObject->Terminate();
            slot.gameObject.reset();
        }
    }
    mGameObjectSlots.clear();
    mFreeSlots.clear();
    mToBeDestroyed.clear();

    for (auto& service : mServices)
    {
        service->Terminate();
        service.reset();
    }
    mServices.clear();

    mInitialized = false;
}

void GameWorld::Update(float deltaTime)
{
    for (Slot& slot : mGameObjectSlots)
    {
        if (slot.gameObject != nullptr)
        {
            slot.gameObject->Update(deltaTime);
        }
    }
    for (auto& service : mServices)
    {
        service->Update(deltaTime);
    }

    ProcessDestroyList();
}

void GameWorld::Render()
{
    for (auto& service : mServices)
    {
        service->Render();
    }
}

void GameWorld::DebugUI()
{
    for (Slot& slot : mGameObjectSlots)
    {
        if (slot.gameObject != nullptr)
        {
            slot.gameObject->DebugUI();
        }
    }
    for (auto& service : mServices)
    {
        service->DebugUI();
    }
}

GameObject* GameWorld::CreateGameObject(std::string name, const std::filesystem::path& templatePath)
{
    ASSERT(mInitialized, "GameWorld: Not initialized!");
    if (mFreeSlots.empty())
    {
        ASSERT(false, "GameWorld: No more free slots to create a new GameObject!");
        return nullptr;
    }

    const uint32_t freeSlot = mFreeSlots.back();
    mFreeSlots.pop_back();

    Slot& slot = mGameObjectSlots[freeSlot];
    slot.gameObject = std::make_unique<GameObject>();
    slot.gameObject->SetName(name);
    slot.gameObject->mHandle.mIndex = freeSlot;
    slot.gameObject->mHandle.mGeneration = slot.generation;
    slot.gameObject->mWorld = this;

    if (!templatePath.empty())
    {
        GameObjectFactory::Make(templatePath, *slot.gameObject, *this);
    }
    return slot.gameObject.get();
}

void GameWorld::DestroyGameObject(const GameObjectHandle& handle)
{
    if (!IsValid(handle))
    {
        return;
    }

    Slot& slot = mGameObjectSlots[handle.mIndex];
    ++slot.generation;
    mToBeDestroyed.push_back(handle.mIndex);
}

bool GameWorld::IsValid(const GameObjectHandle& handle)
{
    if (handle.mIndex < 0 || handle.mIndex >= static_cast<int>(mGameObjectSlots.size()))
    {
        return false;
    }
    if (mGameObjectSlots[handle.mIndex].generation != static_cast<uint32_t>(handle.mGeneration))
    {
        return false;
    }

    return true;
}

void GameWorld::ProcessDestroyList()
{
    for (uint32_t index : mToBeDestroyed)
    {
        Slot& slot = mGameObjectSlots[index];
        GameObject* gameObject = slot.gameObject.get();
        ASSERT(!IsValid(gameObject->GetHandle()), "GameWorld: gameObject is still alive!");

        gameObject->Terminate();
        slot.gameObject.reset();
        mFreeSlots.push_back(index);
    }

    mToBeDestroyed.clear();
}
