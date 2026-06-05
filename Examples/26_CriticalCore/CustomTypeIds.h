#pragma once

#include <Engine/Inc/TypeIds.h>

namespace Engine::CriticalCore
{
// Example-local component ids. Numbered ABOVE Engine::ComponentId::Count so they
// never collide with the engine's built-in component ids. Each enumerator name
// matches the component class name AND the JSON template "Components" key string
// (e.g. CoreComponent -> "CoreComponent"). Tasks 20-29 implement these.
enum class CustomComponentId
{
    CoreComponent = static_cast<int>(Engine::ComponentId::Count),
    PlayerComponent,
    BubbleComponent,
    FireballComponent,
    SpikeComponent,
    WallComponent,
    TrailComponent,
    SparkleComponent,
    BackgroundComponent,
    MusicControllerComponent,
    ScoreComponent,
    GuiComponent,
    MenuComponent,
    Render2DComponent,
    // Trivial proof-of-path stub (this task). Real components above.
    Stub
};

// Example-local service ids. Numbered ABOVE Engine::ServiceId::Count. Real
// services land in tasks 15/18/19.
enum class CustomServiceId
{
    CriticalCore2DRenderService = static_cast<int>(Engine::ServiceId::Count),
    BeatService,
    CameraShakeService
};
} // namespace Engine::CriticalCore
