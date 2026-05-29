#include "Precompiled.h"

#include <rapidjson/filereadstream.h>

#include "GameObjectFactory.h"
#include "Component.h"
#include "GameObject.h"
#include "GameWorld.h"

#include "TransformComponent.h"
#include "CameraComponent.h"
#include "FPSCameraComponent.h"
#include "MeshComponent.h"
#include "ModelComponent.h"
#include "AnimatorComponent.h"
#include "RigidBodyComponent.h"
#include "SoundEventComponent.h"
#include "SoundBankComponent.h"
#include "UITextComponent.h"
#include "UISpriteComponent.h"
#include "UIButtonComponent.h"

using namespace Engine;

namespace
{
CustomComponent TryMakeComponent;
CustomComponent TryGetComponent;

Component* AddComponent(const std::string& componentName, GameObject& gameObject)
{
    Component* newComponent = nullptr;
    if (componentName == "TransformComponent")
    {
        newComponent = gameObject.AddComponent<TransformComponent>();
    }
    else if (componentName == "CameraComponent")
    {
        newComponent = gameObject.AddComponent<CameraComponent>();
    }
    else if (componentName == "FPSCameraComponent")
    {
        newComponent = gameObject.AddComponent<FPSCameraComponent>();
    }
    else if (componentName == "MeshComponent")
    {
        newComponent = gameObject.AddComponent<MeshComponent>();
    }
    else if (componentName == "ModelComponent")
    {
        newComponent = gameObject.AddComponent<ModelComponent>();
    }
    else if (componentName == "AnimatorComponent")
    {
        newComponent = gameObject.AddComponent<AnimatorComponent>();
    }
    else if (componentName == "RigidBodyComponent")
    {
        newComponent = gameObject.AddComponent<RigidBodyComponent>();
    }
    else if (componentName == "SoundEventComponent")
    {
        newComponent = gameObject.AddComponent<SoundEventComponent>();
    }
    else if (componentName == "SoundBankComponent")
    {
        newComponent = gameObject.AddComponent<SoundBankComponent>();
    }
    else if (componentName == "UITextComponent")
    {
        newComponent = gameObject.AddComponent<UITextComponent>();
    }
    else if (componentName == "UISpriteComponent")
    {
        newComponent = gameObject.AddComponent<UISpriteComponent>();
    }
    else if (componentName == "UIButtonComponent")
    {
        newComponent = gameObject.AddComponent<UIButtonComponent>();
    }
    else if (TryMakeComponent)
    {
        newComponent = TryMakeComponent(componentName, gameObject);
    }

    ASSERT(newComponent != nullptr,
           "GameObjectFactory: Component type [%s] not found!",
           componentName.c_str());

    return newComponent;
}

Component* GetComponent(const std::string& componentName, GameObject& gameObject)
{
    Component* component = nullptr;
    if (componentName == "TransformComponent")
    {
        component = gameObject.GetComponent<TransformComponent>();
    }
    else if (componentName == "CameraComponent")
    {
        component = gameObject.GetComponent<CameraComponent>();
    }
    else if (componentName == "FPSCameraComponent")
    {
        component = gameObject.GetComponent<FPSCameraComponent>();
    }
    else if (componentName == "MeshComponent")
    {
        component = gameObject.GetComponent<MeshComponent>();
    }
    else if (componentName == "ModelComponent")
    {
        component = gameObject.GetComponent<ModelComponent>();
    }
    else if (componentName == "AnimatorComponent")
    {
        component = gameObject.GetComponent<AnimatorComponent>();
    }
    else if (componentName == "RigidBodyComponent")
    {
        component = gameObject.GetComponent<RigidBodyComponent>();
    }
    else if (componentName == "UITextComponent")
    {
        component = gameObject.GetComponent<UITextComponent>();
    }
    else if (componentName == "UISpriteComponent")
    {
        component = gameObject.GetComponent<UISpriteComponent>();
    }
    else if (componentName == "UIButtonComponent")
    {
        component = gameObject.GetComponent<UIButtonComponent>();
    }
    else if (TryGetComponent)
    {
        component = TryGetComponent(componentName, gameObject);
    }

    ASSERT(component != nullptr,
           "GameObjectFactory: Component type [%s] not found!",
           componentName.c_str());
    return component;
}
} // namespace

void GameObjectFactory::SetCustomMake(CustomComponent callback)
{
    TryMakeComponent = callback;
}

void GameObjectFactory::SetCustomGet(CustomComponent callback)
{
    TryGetComponent = callback;
}

void GameObjectFactory::Make(const std::filesystem::path& templatePath,
                             GameObject& gameObject,
                             GameWorld& gameWorld)
{
    FILE* file = nullptr;
    auto err = fopen_s(&file, templatePath.string().c_str(), "r");
    ASSERT(err == 0, "GameObjectFactory: Failed to open file %s", templatePath.string().c_str());

    char readBuffer[65536];
    rapidjson::FileReadStream readStream(file, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(readStream);
    fclose(file);

    auto components = doc["Components"].GetObj();
    for (auto& component : components)
    {
        Component* newComponent = AddComponent(component.name.GetString(), gameObject);
        if (newComponent != nullptr)
        {
            newComponent->Deserialize(component.value);
        }
    }

    if (doc.HasMember("Children"))
    {
        auto children = doc["Children"].GetObj();
        for (auto& child : children)
        {
            std::string name = child.name.GetString();
            std::filesystem::path childTemplate = child.value["Template"].GetString();
            GameObject* childGO = gameWorld.CreateGameObject(name, childTemplate);

            OverrideDeserialize(child.value, *childGO);
            gameObject.AddChild(childGO);
            childGO->SetParent(&gameObject);
        }
    }
}

void GameObjectFactory::OverrideDeserialize(const rapidjson::Value& value, GameObject& gameObject)
{
    if (value.HasMember("Components"))
    {
        auto components = value["Components"].GetObj();
        for (auto& component : components)
        {
            Component* ownedComponent = GetComponent(component.name.GetString(), gameObject);
            if (ownedComponent != nullptr)
            {
                ownedComponent->Deserialize(component.value);
            }
        }
    }
}
