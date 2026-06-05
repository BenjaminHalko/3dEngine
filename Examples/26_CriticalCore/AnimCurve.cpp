#include "AnimCurve.h"

#include <cmath>

namespace Engine::CriticalCore
{
    namespace
    {
        // Parametric cubic Bezier control points derived from the CoreCurves
        // "movement" channel tangent handles.
        //   B0 = P0                      = (0, 0)
        //   B1 = P0 + (th1, tv1)         = (0.09618321, -0.006666649)
        //   B2 = P1 + (th0, tv0)         = (1 - 0.38053435, 1 - 0.026666641)
        //   B3 = P1                      = (1, 1)
        constexpr float kB0x = 0.0f, kB0y = 0.0f;
        constexpr float kB1x = 0.09618321f, kB1y = -0.006666649f;
        constexpr float kB2x = 0.61946565f, kB2y = 0.973333359f;
        constexpr float kB3x = 1.0f, kB3y = 1.0f;

        float Bezier(float p0, float p1, float p2, float p3, float s)
        {
            const float u = 1.0f - s;
            const float u2 = u * u;
            const float s2 = s * s;
            return u2 * u * p0 + 3.0f * u2 * s * p1 + 3.0f * u * s2 * p2 + s2 * s * p3;
        }

        float BezierX(float s)
        {
            return Bezier(kB0x, kB1x, kB2x, kB3x, s);
        }

        float BezierY(float s)
        {
            return Bezier(kB0y, kB1y, kB2y, kB3y, s);
        }

        // Solve for the parameter s such that BezierX(s) == x (x in [0,1]).
        // X is monotonically increasing here, so a bisection is robust.
        float SolveForX(float x)
        {
            if (x <= 0.0f)
            {
                return 0.0f;
            }
            if (x >= 1.0f)
            {
                return 1.0f;
            }

            float lo = 0.0f;
            float hi = 1.0f;
            float s = x; // reasonable initial guess
            for (int i = 0; i < 32; ++i)
            {
                s = 0.5f * (lo + hi);
                const float bx = BezierX(s);
                if (std::fabs(bx - x) < 1e-6f)
                {
                    break;
                }
                if (bx < x)
                {
                    lo = s;
                }
                else
                {
                    hi = s;
                }
            }
            return s;
        }

        float SplineValue(float x)
        {
            return BezierY(SolveForX(x));
        }
    }

    AnimCurve::AnimCurve()
    {
        for (int i = 0; i < kSampleCount; ++i)
        {
            const float x = static_cast<float>(i) / static_cast<float>(kSampleCount - 1);
            mLut[i] = SplineValue(x);
        }
        // Pin endpoints exactly.
        mLut[0] = 0.0f;
        mLut[kSampleCount - 1] = 1.0f;
    }

    float AnimCurve::Evaluate(float t) const
    {
        if (t <= 0.0f)
        {
            return mLut[0];
        }
        if (t >= 1.0f)
        {
            return mLut[kSampleCount - 1];
        }

        const float scaled = t * static_cast<float>(kSampleCount - 1);
        const int idx = static_cast<int>(scaled);
        const float frac = scaled - static_cast<float>(idx);
        return mLut[idx] + (mLut[idx + 1] - mLut[idx]) * frac;
    }

    bool AnimCurve::SelfTest(std::string& out) const
    {
        const float samples[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        bool ok = true;

        out = "t,value\n";
        for (float t : samples)
        {
            const float v = Evaluate(t);
            out += std::to_string(t) + "," + std::to_string(v) + "\n";

            // Verify against a freshly solved spline value for the mid samples.
            if (t > 0.0f && t < 1.0f)
            {
                const float ref = SplineValue(t);
                if (std::fabs(v - ref) > 1e-3f)
                {
                    ok = false;
                }
            }
        }

        // Endpoint guarantees.
        if (std::fabs(Evaluate(0.0f) - 0.0f) > 1e-6f)
        {
            ok = false;
        }
        if (std::fabs(Evaluate(1.0f) - 1.0f) > 1e-6f)
        {
            ok = false;
        }

        return ok;
    }
}
