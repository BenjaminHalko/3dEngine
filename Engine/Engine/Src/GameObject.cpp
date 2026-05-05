#include "Precompiled.h"
#include "GameObject.h"

using namespace Engine;

static uint32_t gUniqueId = 0;

void GameObject::Initialize()
{
    ASSERT(!mInitialized, "GameObject: Already initialized!");
    for (auto& component : mComponents)
    {
        component->Initialize();
    }

    mId = ++gUniqueId;
    mInitialized = true;
}

void GameObject::Terminate()
{
    for (auto& component : mComponents)
    {
        component->Terminate();
        component.reset();
    }
    mComponents.clear();
}

void GameObject::Update(float deltaTime)
{
    for (auto& component : mComponents)
    {
        component->Update(deltaTime);
    }
}

void GameObject::DebugUI()
{
    ImGui::PushID(mId);
    if (ImGui::CollapsingHeader(mName.c_str()))
    {
        for (auto& component : mComponents)
        {
            component->DebugUI();
        }
    }
    ImGui::PopID();
}

void GameObject::SetName(std::string& name)
{
    mName = std::move(name);
}

const std::string& GameObject::GetName() const
{
    return mName;
}

uint32_t GameObject::GetId() const
{
    return mId;
}

const GameObjectHandle& GameObject::GetHandle() const
{
    return mHandle;
}

GameWorld& GameObject::GetWorld()
{
    return *mWorld;
}

const GameWorld& GameObject::GetWorld() const
{
    return *mWorld;
}
