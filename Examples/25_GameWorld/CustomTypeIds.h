#pragma once

#include <Engine/Inc/TypeIds.h>

enum class CustomComponentId
{
    CustomDebugDraw = static_cast<int>(Engine::ComponentId::Count)
};

enum class CustomServiceId
{
    CustomDebugDrawDisplay = static_cast<int>(Engine::ServiceId::Count)
};
