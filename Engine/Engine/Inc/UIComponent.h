#pragma once

#include "Component.h"

namespace Engine
{
class UIComponent : public Component
{
  public:
    virtual void Render() = 0;
};
} // namespace Engine
