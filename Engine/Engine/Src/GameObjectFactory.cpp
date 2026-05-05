#include "Precompiled.h"

#include <rapidjson/filereadstream.h>

#include "GameObjectFactory.h"
#include "Component.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "FPSCameraComponent.h"
#include "MeshComponent.h"
#include "ModelComponent.h"

using namespace Engine;

namespace
{
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

        ASSERT(newComponent != nullptr, "GameObjectFactory: Component type [%s] not found!", componentName.c_str());

        return newComponent;
    }
}

void GameObjectFactory::Make(const std::filesystem::path& templatePath, GameObject& gameObject, GameWorld& gameWorld)
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
}
