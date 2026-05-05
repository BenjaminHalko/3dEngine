#pragma once

namespace Engine
{
    class GameObject;
    class GameWorld;
    class Component;

    namespace GameObjectFactory
    {
        void Make(const std::filesystem::path& templatePath, GameObject& gameObject, GameWorld& gameWorld);
    }
}
