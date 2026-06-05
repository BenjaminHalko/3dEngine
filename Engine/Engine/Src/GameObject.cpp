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

    for (GameObject* child : mChildren)
    {
        child->Initialize();
    }
}

void GameObject::Terminate()
{
    for (GameObject* child : mChildren)
    {
        child->Terminate();
    }

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

void GameObject::LateUpdate(float deltaTime)
{
    for (auto& component : mComponents)
    {
        component->LateUpdate(deltaTime);
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

void GameObject::AddChild(GameObject* child)
{
    mChildren.push_back(child);
}

uint32_t GameObject::GetChildCount() const
{
    return static_cast<uint32_t>(mChildren.size());
}

GameObject* GameObject::GetChild(uint32_t index)
{
    if (index < GetChildCount())
    {
        return mChildren[index];
    }
    return nullptr;
}

const GameObject* GameObject::GetChild(uint32_t index) const
{
    if (index < GetChildCount())
    {
        return mChildren[index];
    }
    return nullptr;
}

void GameObject::SetParent(GameObject* parent)
{
    mParent = parent;
}

GameObject* GameObject::GetParent()
{
    return mParent;
}

const GameObject* GameObject::GetParent() const
{
    return mParent;
}
