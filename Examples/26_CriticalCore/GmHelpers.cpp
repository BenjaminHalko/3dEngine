#include "GmHelpers.h"

#include <cmath>
#include <random>
#include <sstream>

namespace Engine::CriticalCore
{
namespace
{
namespace MC = Engine::Math::Constants;

// Shared, seedable RNG so --selftest is reproducible.
std::mt19937& Rng()
{
    static std::mt19937 engine{0xC0FFEEu};
    return engine;
}
} // namespace

//==================================================================================
// Easing / approach
//==================================================================================

float Approach(float a, float b, float amt)
{
    if (a < b)
    {
        a += amt;
        if (a > b)
            return b;
    }
    else
    {
        a -= amt;
        if (a < b)
            return b;
    }
    return a;
}

float ApproachFade(float v, float target, float maxSpd, float ease)
{
    return v + Median3(-maxSpd, maxSpd, (1.0f - ease) * (target - v));
}

float ApproachCircleEase(float v, float target, float maxSpd, float ease)
{
    return v + Median3(-maxSpd, maxSpd, (1.0f - ease) * AngleDifference(target, v));
}

float ValuePercent(float x, float a, float b)
{
    return (x - a) / (b - a);
}

//==================================================================================
// Wave / wrap
//==================================================================================

float Wave(float from, float to, float duration, float offset, float clockTime)
{
    const float amp = (to - from) * 0.5f;
    return from + amp +
           std::sin(((clockTime + duration * offset) / duration) * MC::TwoPi) * amp;
}

float Wrap(float v, float minV, float maxV)
{
    // Continuous wrap branch (HelperFunctions.gml:84-95). GM also has an integer
    // special-case branch with inclusive off-by-one semantics; we deliberately use
    // the continuous branch for all inputs (Wrap(370,0,360) == 10, the hand-computed
    // gate; the integer branch would yield 9). Gameplay wrap usage is angular.
    float valueOld = v + 1.0f;
    while (v != valueOld)
    {
        valueOld = v;
        if (v < minV)
            v = maxV - (minV - v);
        else if (v > maxV)
            v = minV + (v - maxV);
    }
    return v;
}

//==================================================================================
// Vector / direction (degree-based, GameMaker lengthdir sign convention)
//==================================================================================

float LengthDirX(float len, float deg)
{
    return len * std::cos(deg * MC::DegToRad);
}

float LengthDirY(float len, float deg)
{
    // GameMaker y points DOWN -> negated sine.
    return -len * std::sin(deg * MC::DegToRad);
}

void RotateVector(float x, float y, float deg, float& outX, float& outY)
{
    const float dist = PointDistance(0.0f, 0.0f, x, y);
    const float dir = PointDirection(0.0f, 0.0f, x, y);
    outX = LengthDirX(dist, dir + deg);
    outY = LengthDirY(dist, dir + deg);
}

float PointDistance(float x1, float y1, float x2, float y2)
{
    return std::hypot(x2 - x1, y2 - y1);
}

float PointDirection(float x1, float y1, float x2, float y2)
{
    // GameMaker convention: 0=right, 90=up (y is down so dy is negated).
    float deg = std::atan2(y1 - y2, x2 - x1) * MC::RadToDeg;
    if (deg < 0.0f)
        deg += 360.0f;
    return deg;
}

float AngleDifference(float a, float b)
{
    // Signed shortest delta a-b wrapped to [-180,180].
    float d = std::fmod(a - b + 180.0f, 360.0f);
    if (d < 0.0f)
        d += 360.0f;
    return d - 180.0f;
}

float Median3(float a, float b, float c)
{
    return a + b + c - Engine::Math::Min(a, Engine::Math::Min(b, c)) -
           Engine::Math::Max(a, Engine::Math::Max(b, c));
}

//==================================================================================
// Color (GameMaker HSV: h,s,v in 0-255; RGB stored as engine Color 0-1, alpha 1)
//==================================================================================

Color MakeColorHSV(float h, float s, float v)
{
    // GM ranges 0-255 -> normalized. Hue spans the full circle over 0-255.
    const float hue = (h / 255.0f) * 360.0f;
    const float sat = s / 255.0f;
    const float val = v / 255.0f;

    if (sat <= 0.0f)
        return Color{val, val, val, 1.0f};

    float hh = hue / 60.0f;
    if (hh >= 6.0f)
        hh = std::fmod(hh, 6.0f);
    const int sector = static_cast<int>(hh);
    const float ff = hh - static_cast<float>(sector);
    const float p = val * (1.0f - sat);
    const float q = val * (1.0f - sat * ff);
    const float t = val * (1.0f - sat * (1.0f - ff));

    switch (sector)
    {
    case 0:
        return Color{val, t, p, 1.0f};
    case 1:
        return Color{q, val, p, 1.0f};
    case 2:
        return Color{p, val, t, 1.0f};
    case 3:
        return Color{p, q, val, 1.0f};
    case 4:
        return Color{t, p, val, 1.0f};
    default:
        return Color{val, p, q, 1.0f};
    }
}

Color MergeColor(Color a, Color b, float t)
{
    return Color{Engine::Math::Lerp(a.r, b.r, t),
                 Engine::Math::Lerp(a.g, b.g, t),
                 Engine::Math::Lerp(a.b, b.b, t),
                 Engine::Math::Lerp(a.a, b.a, t)};
}

float ColorGetHue(Color c)
{
    const float maxC = Engine::Math::Max(c.r, Engine::Math::Max(c.g, c.b));
    const float minC = Engine::Math::Min(c.r, Engine::Math::Min(c.g, c.b));
    const float delta = maxC - minC;
    if (delta <= 0.0f)
        return 0.0f;

    float hue = 0.0f;
    if (maxC == c.r)
        hue = std::fmod((c.g - c.b) / delta, 6.0f);
    else if (maxC == c.g)
        hue = (c.b - c.r) / delta + 2.0f;
    else
        hue = (c.r - c.g) / delta + 4.0f;

    hue *= 60.0f; // degrees
    if (hue < 0.0f)
        hue += 360.0f;
    return (hue / 360.0f) * 255.0f; // GM 0-255
}

float ColorGetSat(Color c)
{
    const float maxC = Engine::Math::Max(c.r, Engine::Math::Max(c.g, c.b));
    const float minC = Engine::Math::Min(c.r, Engine::Math::Min(c.g, c.b));
    if (maxC <= 0.0f)
        return 0.0f;
    return ((maxC - minC) / maxC) * 255.0f;
}

float ColorGetValue(Color c)
{
    return Engine::Math::Max(c.r, Engine::Math::Max(c.g, c.b)) * 255.0f;
}

//==================================================================================
// RNG
//==================================================================================

void SeedRng(uint32_t seed)
{
    Rng().seed(seed);
}

float RandomRange(float lo, float hi)
{
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(Rng());
}

int IRandom(int n)
{
    if (n < 0)
        return 0;
    std::uniform_int_distribution<int> dist(0, n);
    return dist(Rng());
}

//==================================================================================
// Self test
//==================================================================================

bool HelpersSelfTest(std::string& csvOut)
{
    bool ok = true;
    std::ostringstream csv;
    csv << "name,expected,actual,pass\n";

    auto checkEq = [&](const char* name, float expected, float actual, float eps) {
        const bool pass = std::fabs(actual - expected) <= eps;
        csv << name << ',' << expected << ',' << actual << ',' << (pass ? 1 : 0) << '\n';
        ok = ok && pass;
    };

    // Approach
    checkEq("Approach(0,10,3)", 3.0f, Approach(0.0f, 10.0f, 3.0f), 1e-6f);
    checkEq("Approach(10,0,3)", 7.0f, Approach(10.0f, 0.0f, 3.0f), 1e-6f);

    // lengthdir (GameMaker y-down sign convention)
    checkEq("LengthDirX(1,90)", 0.0f, LengthDirX(1.0f, 90.0f), 1e-5f);
    checkEq("LengthDirY(1,90)", -1.0f, LengthDirY(1.0f, 90.0f), 1e-5f);

    // RotateVector(1,0,90) ~= (0,-1)
    float rx = 0.0f;
    float ry = 0.0f;
    RotateVector(1.0f, 0.0f, 90.0f, rx, ry);
    checkEq("RotateVector(1,0,90).x", 0.0f, rx, 1e-5f);
    checkEq("RotateVector(1,0,90).y", -1.0f, ry, 1e-5f);

    // Wrap (continuous branch): 370 in [0,360] -> 10
    checkEq("Wrap(370,0,360)", 10.0f, Wrap(370.0f, 0.0f, 360.0f), 1e-5f);

    // AngleDifference signed shortest delta, a-b in [-180,180]: 10-350 -> +20
    checkEq("AngleDifference(10,350)", 20.0f, AngleDifference(10.0f, 350.0f), 1e-5f);

    // Median3 sanity
    checkEq("Median3(-5,5,2)", 2.0f, Median3(-5.0f, 5.0f, 2.0f), 1e-6f);

    // ValuePercent sanity
    checkEq("ValuePercent(5,0,10)", 0.5f, ValuePercent(5.0f, 0.0f, 10.0f), 1e-6f);

    // Wave at clockTime=0, offset=0: from + amp + sin(0)*amp == midpoint.
    checkEq("Wave(0,10,1,0,0)", 5.0f, Wave(0.0f, 10.0f, 1.0f, 0.0f, 0.0f), 1e-5f);

    // Color round-trip: pure red has hue 0, full sat/value (GM 0-255).
    const Color red = MakeColorHSV(0.0f, 255.0f, 255.0f);
    checkEq("MakeColorHSV(0,255,255).r", 1.0f, red.r, 1e-5f);
    checkEq("MakeColorHSV(0,255,255).g", 0.0f, red.g, 1e-5f);
    checkEq("ColorGetValue(red)", 255.0f, ColorGetValue(red), 1e-3f);
    checkEq("ColorGetSat(red)", 255.0f, ColorGetSat(red), 1e-3f);

    // MergeColor midpoint of black->white = gray.
    const Color gray = MergeColor(Color{0, 0, 0, 1}, Color{1, 1, 1, 1}, 0.5f);
    checkEq("MergeColor(black,white,.5).r", 0.5f, gray.r, 1e-6f);

    // RNG reproducibility: same seed -> same sequence.
    SeedRng(12345u);
    const float r0 = RandomRange(0.0f, 1.0f);
    SeedRng(12345u);
    const float r1 = RandomRange(0.0f, 1.0f);
    checkEq("RandomRange reproducible", r0, r1, 0.0f);

    csvOut = csv.str();
    return ok;
}
} // namespace Engine::CriticalCore
