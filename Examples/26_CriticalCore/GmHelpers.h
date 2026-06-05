#pragma once

#include <Graphics/Inc/Color.h>
#include <Math/Inc/DWMath.h>

#include <cstdint>
#include <initializer_list>
#include <string>

// GameMaker helper math/easing/color/RNG utilities ported from "Critical Core 2"
// (scripts/HelperFunctions/HelperFunctions.gml). These reproduce the GML built-ins
// and helpers the gameplay depends on.
//
// Conventions (documented for downstream tasks 9/17/20/21/22/23):
//  - Trig is DEGREE-based, GameMaker convention. y axis points DOWN, so
//    LengthDirY = -len*sin(deg) (the sign is negated vs. standard math).
//  - HSV uses GameMaker's 0-255 range for hue, saturation AND value (matching the
//    source's make_color_hsv / color_get_* usage, e.g. make_color_hsv(0,0,255*0.9)).
//    RGB is stored in the engine Color (Vector4, 0-1 per channel, alpha forced 1).
//  - AngleDifference(a,b) returns the signed shortest delta a-b wrapped to
//    [-180,180] (GameMaker angle_difference order). AngleDifference(10,350) == +20.
//  - RNG uses a seedable std::mt19937; call SeedRng() for reproducible --selftest.
//
// REUSES engine primitives (Engine::Math::Lerp/Clamp/Min/Max, Constants::DegToRad,
// Engine::Graphics::Color). Does NOT reimplement Lerp/Clamp/Distance.
namespace Engine::CriticalCore
{
using Color = Engine::Graphics::Color;

//==================================================================================
// Easing / approach
//==================================================================================

// Moves 'a' toward 'b' by 'amt', clamped so it never overshoots.
// Approach(0,10,3)==3 ; Approach(10,0,3)==7. (HelperFunctions.gml:6-21)
float Approach(float a, float b, float amt);

// Eased approach: value += median(-maxSpd, maxSpd, (1-ease)*(target-value)).
// (HelperFunctions.gml:29-33)
float ApproachFade(float v, float target, float maxSpd, float ease);

// Eased approach over the angular shortest path (uses AngleDifference).
// (HelperFunctions.gml:41-45)
float ApproachCircleEase(float v, float target, float maxSpd, float ease);

// Turns x into a normalized percent in [a,b]: (x-a)/(b-a). (HelperFunctions.gml:52-54)
float ValuePercent(float x, float a, float b);

//==================================================================================
// Wave / wrap
//==================================================================================

// Sine wave oscillating between [from,to] over 'duration' seconds, phase-shifted by
// 'offset' (0..1). Driven by the GAME-CLOCK time 'clockTime' (seconds) passed in,
// NEVER wall-clock. (HelperFunctions.gml:62-65; source current_time*0.001 -> clockTime)
float Wave(float from, float to, float duration, float offset, float clockTime);

// Wraps v into [minV,maxV], values over/under wrap around.
// (HelperFunctions.gml:72-98, continuous branch.)
float Wrap(float v, float minV, float maxV);

//==================================================================================
// Vector / direction (degree-based, GameMaker lengthdir sign convention)
//==================================================================================

// lengthdir_x: len*cos(deg). LengthDirX(1,90) ~= 0.
float LengthDirX(float len, float deg);

// lengthdir_y: -len*sin(deg) (GameMaker y-down convention). LengthDirY(1,90) ~= -1.
float LengthDirY(float len, float deg);

// Rotates the vector (x,y) by 'deg' degrees about the origin (lengthdir-based).
// RotateVector(1,0,90) ~= (0,-1). (HelperFunctions.gml:104-111)
void RotateVector(float x, float y, float deg, float& outX, float& outY);

// Euclidean distance between (x1,y1) and (x2,y2).
float PointDistance(float x1, float y1, float x2, float y2);

// Direction in degrees [0,360) from (x1,y1) to (x2,y2), GameMaker convention
// (0=right, 90=up because y points down).
float PointDirection(float x1, float y1, float x2, float y2);

// Signed shortest angular delta a-b in degrees, wrapped to [-180,180].
float AngleDifference(float a, float b);

// Median of three values (GML median(a,b,c)).
float Median3(float a, float b, float c);

//==================================================================================
// Color (GameMaker HSV: h,s,v in 0-255; RGB stored as engine Color 0-1, alpha 1)
//==================================================================================

// make_color_hsv(h,s,v) with h,s,v in [0,255].
Color MakeColorHSV(float h, float s, float v);

// merge_color(a,b,t): per-channel lerp of RGBA by t in [0,1].
Color MergeColor(Color a, Color b, float t);

// color_get_hue: hue in [0,255].
float ColorGetHue(Color c);

// color_get_saturation: saturation in [0,255].
float ColorGetSat(Color c);

// color_get_value: value (brightness) in [0,255].
float ColorGetValue(Color c);

//==================================================================================
// RNG (seedable for reproducible --selftest)
//==================================================================================

// Seeds the shared engine. Call before deterministic runs (e.g. --selftest).
void SeedRng(uint32_t seed);

// random_range(lo,hi): uniform real in [lo,hi).
float RandomRange(float lo, float hi);

// irandom(n): uniform integer in [0,n] INCLUSIVE.
int IRandom(int n);

// choose(...): picks one of the listed values uniformly at random.
template <typename T> T Choose(std::initializer_list<T> options)
{
    const int idx = IRandom(static_cast<int>(options.size()) - 1);
    return *(options.begin() + idx);
}

//==================================================================================
// Self test (emits helpers.csv rows; asserts hand-computed values). Task 35's
// --selftest writes the result to .omo/evidence/task-8-helpers.csv.
//==================================================================================
bool SelfTest(std::string& csvOut);
} // namespace Engine::CriticalCore
