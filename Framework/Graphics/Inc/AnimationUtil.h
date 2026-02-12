#pragma once

#include "ModelManager.h"
#include "Animator.h"

namespace Engine::Graphics::AnimationUtil
{
using BoneTransforms = std::vector<Math::Matrix4>;

void ComputeBoneTransforms(ModelId modelId,
                           BoneTransforms& boneTransforms,
                           const Animator* animator = nullptr);

void DrawSkeleton(ModelId modelId, const BoneTransforms& boneTransforms);

void ApplyBoneOffsets(ModelId modelId, BoneTransforms& boneTransforms);
} // namespace Engine::Graphics::AnimationUtil
