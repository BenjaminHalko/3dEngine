#pragma once

#include <rapidjson/document.h>

namespace Engine
{
class GameObject;
class GameWorld;
class Component;

using CustomComponent = std::function<Component*(const std::string&, GameObject&)>;

namespace GameObjectFactory
{
void SetCustomMake(CustomComponent callback);
void SetCustomGet(CustomComponent callback);

void Make(const std::filesystem::path& templatePath, GameObject& gameObject, GameWorld& gameWorld);

void OverrideDeserialize(const rapidjson::Value& value, GameObject& gameObject);
} // namespace GameObjectFactory
} // namespace Engine
