#pragma once

#include "Bone.h"

namespace Engine::Graphics
{
struct Skeleton
{
    Bone* root = nullptr;
    std::vector<std::unique_ptr<Bone>> bones;
};
} // namespace Engine::Graphics
