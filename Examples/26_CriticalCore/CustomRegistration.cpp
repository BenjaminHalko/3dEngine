#include "CustomRegistration.h"

#include "CustomTypeIds.h"

// --- Component headers (one per registered component) -----------------------
#include "BackgroundComponent.h"
#include "BubbleComponent.h"
#include "CoreComponent.h"
#include "FireballComponent.h"
#include "GuiComponent.h"
#include "MenuComponent.h"
#include "MusicController.h" // MusicControllerComponent + BeatService
#include "PlayerComponent.h"
#include "Render2DComponent.h"
#include "ScoreComponent.h"
#include "SparkleComponent.h"
#include "SpikeComponent.h"
#include "StubComponent.h"
#include "TrailComponent.h"
#include "WallComponent.h"

// --- Service headers --------------------------------------------------------
#include "CameraShakeService.h"
#include "CriticalCore2DRenderService.h"

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
// the level/templates depend on). make/get stay symmetrical: Add vs Get of the
// SAME type. The keys here MUST match CustomComponentId enumerator spellings AND
// the level / template "Components" keys exactly (task-33 checklist).
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
        // Base 2D renderable (level core.json / player.json list it explicitly).
        {"Render2DComponent", MakeEntry<Render2DComponent>()},
        // Boss + player + projectiles (runtime-spawned by the flow / Core).
        {"CoreComponent", MakeEntry<CoreComponent>()},
        {"PlayerComponent", MakeEntry<PlayerComponent>()},
        {"BubbleComponent", MakeEntry<BubbleComponent>()},
        {"FireballComponent", MakeEntry<FireballComponent>()},
        {"SpikeComponent", MakeEntry<SpikeComponent>()},
        // Arena + particles + audio driver.
        {"WallComponent", MakeEntry<WallComponent>()},
        {"TrailComponent", MakeEntry<TrailComponent>()},
        {"SparkleComponent", MakeEntry<SparkleComponent>()},
        {"BackgroundComponent", MakeEntry<BackgroundComponent>()},
        {"MusicControllerComponent", MakeEntry<MusicControllerComponent>()},
        // HUD / score / menu.
        {"ScoreComponent", MakeEntry<ScoreComponent>()},
        {"GuiComponent", MakeEntry<GuiComponent>()},
        {"MenuComponent", MakeEntry<MenuComponent>()},
        // Proof-of-path stub (kept from scaffold).
        {"StubComponent", MakeEntry<StubComponent>()},
    };
    return table;
}

// --- Service registry -------------------------------------------------------
// One entry per custom service, keyed by the JSON "Services" name. Keys MUST
// match CustomServiceId enumerator spellings + the level "Services" keys.
using ServiceMaker = std::function<Engine::Service*(Engine::GameWorld&)>;

const std::unordered_map<std::string, ServiceMaker>& GetServiceTable()
{
    static const std::unordered_map<std::string, ServiceMaker> table = {
        {"CriticalCore2DRenderService",
         [](Engine::GameWorld& world) -> Engine::Service*
         { return world.AddService<CriticalCore2DRenderService>(); }},
        {"BeatService",
         [](Engine::GameWorld& world) -> Engine::Service*
         { return world.AddService<BeatService>(); }},
        {"CameraShakeService",
         [](Engine::GameWorld& world) -> Engine::Service*
         { return world.AddService<CameraShakeService>(); }},
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
