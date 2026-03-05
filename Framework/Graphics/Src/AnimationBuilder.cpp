#include "Precompiled.h"
#include "AnimationBuilder.h"

using namespace Engine;
using namespace Engine::Graphics;

namespace
{
template <class T> inline void PushKey(Keyframes<T>& keyframes, const T& value, float t)
{
    ASSERT(keyframes.empty() || keyframes.back().time <= t,
           "AnimationBuilder: Can't add keyframe with time less than previous keyframe time.");
    keyframes.emplace_back(value, t);
}
} // namespace

AnimationBuilder& AnimationBuilder::AddPositionKey(const Math::Vector3& position, float time)
{
    PushKey(mWorkingCopy.mPositionKeys, position, time);
    mWorkingCopy.mDuration = Math::Max(mWorkingCopy.mDuration, time);
    return *this;
}

AnimationBuilder& AnimationBuilder::AddRotationKey(const Math::Quaternion& rotation, float time)
{
    PushKey(mWorkingCopy.mRotationKeys, rotation, time);
    mWorkingCopy.mDuration = Math::Max(mWorkingCopy.mDuration, time);
    return *this;
}

AnimationBuilder& AnimationBuilder::AddScaleKey(const Math::Vector3& scale, float time)
{
    PushKey(mWorkingCopy.mScaleKeys, scale, time);
    mWorkingCopy.mDuration = Math::Max(mWorkingCopy.mDuration, time);
    return *this;
}

AnimationBuilder& AnimationBuilder::AddEventKey(const AnimationCallback& callback, float time)
{
    PushKey(mWorkingCopy.mEventKeys, callback, time);
    mWorkingCopy.mDuration = Math::Max(mWorkingCopy.mDuration, time);
    return *this;
}

Animation AnimationBuilder::Build()
{
    ASSERT(!mWorkingCopy.mPositionKeys.empty() || !mWorkingCopy.mRotationKeys.empty() ||
               !mWorkingCopy.mScaleKeys.empty() || !mWorkingCopy.mEventKeys.empty(),
           "AnimationBuilder: No animations are present!");

    return std::move(mWorkingCopy);
}
