#pragma once

namespace Engine
{
enum class ComponentId
{
    Invalid,
    Transform,
    Camera,
    FPSCamera,
    TPCamera,
    Mesh,
    Model,
    Animator,
    RigidBody,
    PlayerController,
    SoundEffect,
    SoundBank,
    UIText,
    UISprite,
    UIButton,
    Count
};

enum class ServiceId
{
    Invalid,
    Camera,
    Render,
    Physics,
    UIRender,
    Count
};
} // namespace Engine

#define SET_TYPE_ID(id)                                                                            \
    static uint32_t StaticGetTypeId()                                                              \
    {                                                                                              \
        return static_cast<uint32_t>(id);                                                          \
    }                                                                                              \
    uint32_t GetTypeId() const override                                                            \
    {                                                                                              \
        return StaticGetTypeId();                                                                  \
    }
