#pragma once

namespace Engine
{
class GameWorld;
} // namespace Engine

namespace Engine::CriticalCore
{
// Wires every Critical Core 2 custom component + service factory into the engine
// via GameObjectFactory::SetCustomMake/SetCustomGet and GameWorld::SetCustomService.
// Call ONCE from GameState::Initialize BEFORE GameWorld::LoadLevel (task 34).
void RegisterCriticalCoreTypes(Engine::GameWorld& world);
} // namespace Engine::CriticalCore
