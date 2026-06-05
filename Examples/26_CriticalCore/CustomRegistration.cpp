#include "CustomRegistration.h"

#include "CustomTypeIds.h"
#include "StubComponent.h"

#include <Engine/Inc/Engine.h>

#include <functional>
#include <string>
#include <unordered_map>

namespace Engine::CriticalCore
{
namespace
{
// --- Component registry -----------------------------------------------------
// One ComponentEntry per custom component, keyed by the component NAME exactly
// as it appears in the JSON template "Components" object (this is the contract
// task 33's level/templates depend on). make/get stay symmetrical: Add vs Get
// of the SAME type. Tasks 20-29 each append a single line to the table below.
struct ComponentEntry
{
    std::function<Engine::Component*(Engine::GameObject&)> make;
    std::function<Engine::Component*(Engine::GameObject&)> get;
};

template <class ComponentType> ComponentEntry MakeEntry()
{
    return {[](Engine::GameObject& gameObject) -> Engine::Component*
            { return gameObject.AddComponent<ComponentType>(); },
            [](Engine::GameObject& gameObject) -> Engine::Component*
            { return gameObject.GetComponent<ComponentType>(); }};
}

const std::unordered_map<std::string, ComponentEntry>& GetComponentTable()
{
    static const std::unordered_map<std::string, ComponentEntry> table = {
        {"StubComponent", MakeEntry<StubComponent>()},
        // Tasks 20-29: add one line per component, e.g.
        // {"CoreComponent",   MakeEntry<CoreComponent>()},
        // {"PlayerComponent", MakeEntry<PlayerComponent>()},
    };
    return table;
}

// --- Service registry -------------------------------------------------------
// One entry per custom service, keyed by the JSON "Services" name. Real services
// land in tasks 15/18/19; register them here as they come online.
using ServiceMaker = std::function<Engine::Service*(Engine::GameWorld&)>;

const std::unordered_map<std::string, ServiceMaker>& GetServiceTable()
{
    static const std::unordered_map<std::string, ServiceMaker> table = {
        // Tasks 15/18/19: add one line per service, e.g.
        // {"CriticalCore2DRenderService",
        //  [](Engine::GameWorld& world) -> Engine::Service*
        //  { return world.AddService<CriticalCore2DRenderService>(); }},
    };
    return table;
}

// --- Dispatch callbacks (installed into the engine) -------------------------
Engine::Component* MakeCustomComponent(const std::string& name, Engine::GameObject& gameObject)
{
    const auto& table = GetComponentTable();
    const auto it = table.find(name);
    return it != table.end() ? it->second.make(gameObject) : nullptr;
}

Engine::Component* GetCustomComponent(const std::string& name, Engine::GameObject& gameObject)
{
    const auto& table = GetComponentTable();
    const auto it = table.find(name);
    return it != table.end() ? it->second.get(gameObject) : nullptr;
}

Engine::Service* MakeCustomService(const std::string& name, Engine::GameWorld& gameWorld)
{
    const auto& table = GetServiceTable();
    const auto it = table.find(name);
    return it != table.end() ? it->second(gameWorld) : nullptr;
}
} // namespace

void RegisterCriticalCoreTypes(Engine::GameWorld& world)
{
    Engine::GameObjectFactory::SetCustomMake(&MakeCustomComponent);
    Engine::GameObjectFactory::SetCustomGet(&GetCustomComponent);
    Engine::GameWorld::SetCustomService(&MakeCustomService);

    // GameWorld custom services are registered through a static callback (above);
    // the world reference is reserved for future per-world registration needs.
    (void)world;
}
} // namespace Engine::CriticalCore
