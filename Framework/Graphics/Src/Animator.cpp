#include "Precompiled.h"
#include "Animator.h"

#include "AnimationClip.h"
#include "Model.h"

using namespace Engine;
using namespace Engine::Graphics;

void Animator::Initialize(ModelId modelId)
{
    mModelId = modelId;
    mIsLooping = false;
    mAnimationTick = 0.0f;
    mClipIndex = -1;
}

void Animator::PlayAnimation(int clipIndex, bool looping)
{
    mClipIndex = clipIndex;
    mIsLooping = looping;
    mAnimationTick = 0.0f;
}

void Animator::Update(float deltaTime)
{
    if (mClipIndex < 0)
    {
        return;
    }

    const Model* model = ModelManager::Get()->GetModel(mModelId);
    const AnimationClip& animClip = model->animationClips[mClipIndex];
    mAnimationTick += animClip.ticksPerSecond * deltaTime;
    if (mIsLooping)
    {
        while (mAnimationTick > animClip.tickDuration)
        {
            mAnimationTick -= animClip.tickDuration;
        }
    }
    else
    {
        mAnimationTick = Math::Min(mAnimationTick, animClip.tickDuration);
    }
}

bool Animator::IsFinished() const
{
    if (mClipIndex < 0 || mIsLooping)
    {
        return false;
    }

    const Model* model = ModelManager::Get()->GetModel(mModelId);
    const AnimationClip& animClip = model->animationClips[mClipIndex];
    return mAnimationTick >= animClip.tickDuration;
}

size_t Animator::GetAnimationCount() const
{
    const Model* model = ModelManager::Get()->GetModel(mModelId);
    return model->animationClips.size();
}

bool Animator::GetToParentTransform(const Bone* bone, Math::Matrix4& transform) const
{
    if (mClipIndex < 0)
    {
        return false;
    }

    const Model* model = ModelManager::Get()->GetModel(mModelId);
    const AnimationClip& animClip = model->animationClips[mClipIndex];
    const Animation* animation = animClip.boneAnimations[bone->index].get();
    if (animation == nullptr)
    {
        return false;
    }
    Transform animTransform = animation->GetTransform(mAnimationTick);
    transform = animTransform.GetMatrix4();
    return true;
}
