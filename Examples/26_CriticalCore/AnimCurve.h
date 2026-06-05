#pragma once

#include <array>
#include <string>

namespace Engine::CriticalCore
{
    // Baked LUT + evaluator for the GameMaker "CoreCurves" -> "movement" channel
    // (animcurves/CoreCurves/CoreCurves.yy, function:2 = cubic spline).
    //
    // The source channel has two control points, each with tangent handles:
    //   P0 (0,0): th1= 0.09618321, tv1=-0.006666649  (outgoing handle)
    //   P1 (1,1): th0=-0.38053435, tv0=-0.026666641  (incoming handle)
    // These form a parametric cubic Bezier. We bake 256 samples of y(x) over the
    // input domain x in [0,1] and linearly interpolate at runtime. This replaces
    // GML animcurve_channel_evaluate(movementCurve, movementPercent), input/output
    // domain [0,1] -> [0,1] (see oCore/Step_0.gml dash lerp).
    class AnimCurve
    {
    public:
        static constexpr int kSampleCount = 256;

        AnimCurve();

        // Clamps t to [0,1], linearly interpolates between baked LUT samples.
        float Evaluate(float t) const;

        // Dumps t,value for t in {0,.25,.5,.75,1} into 'out' and asserts the
        // mid-samples land within 1e-3 of a freshly solved spline value.
        // Returns true on success. Used to emit task-5-animcurve.csv evidence.
        bool SelfTest(std::string& out) const;

    private:
        std::array<float, kSampleCount> mLut{};
    };
}
