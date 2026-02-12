#pragma once

#include <Engine/Inc/Engine.h>
#include "BalatroEffect.h"

class GameState : public Engine::AppState
{
public:
    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override;
    void Render() override;
    void DebugUI() override;

private:
    Engine::Graphics::RenderObject mScreenQuad;
    BalatroEffect mBalatroEffect;
};
