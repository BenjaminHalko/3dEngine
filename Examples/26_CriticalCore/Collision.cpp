#include "Collision.h"

#include "GmHelpers.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace Engine::CriticalCore
{
namespace
{
// Wrap an angle to [0,360).
float Wrap360(float a)
{
    a = std::fmod(a, 360.0f);
    if (a < 0.0f)
        a += 360.0f;
    return a;
}

// The 8 outer arena oWall instances, transcribed from rooms/rGame/rGame.yy:38-45.
// (x, y, image_angle, image_xscale). Cardinals scaleX=12 (length 48); diagonals
// scaleX=28.25 (length 113). image_angle div 45 gives the distinct octant index.
struct RawWall
{
    float x, y, angle, scaleX;
};
constexpr RawWall kOuterRaw[8] = {
    {104.0f, 8.0f, 0.0f, 12.0f},     // top        (104,8)  -> (152,8)
    {152.0f, 216.0f, 180.0f, 12.0f}, // bottom     (152,216)-> (104,216)
    {232.0f, 88.0f, 270.0f, 12.0f},  // right      (232,88) -> (232,136)
    {24.0f, 136.0f, 90.0f, 12.0f},   // left       (24,136) -> (24,88)
    {24.0f, 88.0f, 45.0f, 28.25f},   // top-left   (24,88)  -> (104,8)
    {232.0f, 136.0f, 225.0f, 28.25f},// bottom-right(232,136)-> (152,216)
    {104.0f, 216.0f, 135.0f, 28.25f},// bottom-left(104,216)-> (24,136)
    {152.0f, 8.0f, 315.0f, 28.25f},  // top-right  (152,8)  -> (232,88)
};
} // namespace

//==================================================================================
// Canonical geometry
//==================================================================================
WallSegment MakeWall(float x, float y, float angle, float scaleX, bool flipped, bool bossWall)
{
    WallSegment w{};
    w.x       = x;
    w.y       = y;
    w.angle   = angle;
    w.length  = Arena::kWallSpriteThickness * scaleX; // sprite width (4) * image_xscale
    w.flipped = flipped;
    w.bossWall = bossWall;
    // GML: index = image_angle div 45 ; boss walls override to -1 (oCore:49).
    w.index = bossWall ? -1 : static_cast<int>(std::floor(Wrap360(angle) / 45.0f));

    // Collision centerline = mid-height (local y = +halfThickness) of the 4px-tall
    // sprite, from local x=0..length. GameMaker sprite transform (y-down):
    //   world = origin + lengthdir(localX, angle) + lengthdir(localY, angle-90)
    const float perpX = LengthDirX(Arena::kWallHalfThickness, angle - 90.0f);
    const float perpY = LengthDirY(Arena::kWallHalfThickness, angle - 90.0f);
    w.ax = x + LengthDirX(0.0f, angle) + perpX;
    w.ay = y + LengthDirY(0.0f, angle) + perpY;
    w.bx = x + LengthDirX(w.length, angle) + perpX;
    w.by = y + LengthDirY(w.length, angle) + perpY;
    w.cx = (w.ax + w.bx) * 0.5f;
    w.cy = (w.ay + w.by) * 0.5f;
    return w;
}

const std::array<WallSegment, 8>& OuterWalls()
{
    static const std::array<WallSegment, 8> kWalls = [] {
        std::array<WallSegment, 8> a{};
        for (int i = 0; i < 8; ++i)
        {
            const RawWall& r = kOuterRaw[i];
            a[i] = MakeWall(r.x, r.y, r.angle, r.scaleX, /*flipped*/ false, /*boss*/ false);
        }
        return a;
    }();
    return kWalls;
}

const std::array<WallSegment, 8>& BossWalls()
{
    // Boss walls spawn at the CORE center (oCore:44 instance_create_depth(x,y,...))
    // with image_xscale 0 (length 0), flipped=true, bossWall=true, and the SAME 8
    // angles as the outer walls (oCore:46 image_angle = walls[i].dir). Task 20/21
    // grows the scale at runtime; rebuild live segments via MakeWall(...).
    static const std::array<WallSegment, 8> kWalls = [] {
        std::array<WallSegment, 8> a{};
        for (int i = 0; i < 8; ++i)
        {
            a[i] = MakeWall(Arena::kCenterX, Arena::kCenterY, kOuterRaw[i].angle,
                            /*scaleX*/ 0.0f, /*flipped*/ true, /*boss*/ true);
        }
        return a;
    }();
    return kWalls;
}

//==================================================================================
// Collision tests
//==================================================================================
WallHit CircleVsWall(float cx, float cy, float radius, const WallSegment& w)
{
    WallHit h{};
    h.wallAngle = Wrap360(w.angle);
    h.flipped   = w.flipped;
    h.bossWall  = w.bossWall;
    h.wallIndex = w.index;

    // Closest point on segment [A,B] to the circle center (analytic projection).
    const float dx = w.bx - w.ax;
    const float dy = w.by - w.ay;
    const float len2 = dx * dx + dy * dy;
    float t = 0.0f;
    if (len2 > 0.0f)
        t = ((cx - w.ax) * dx + (cy - w.ay) * dy) / len2;
    t = std::clamp(t, 0.0f, 1.0f);
    const float px = w.ax + t * dx;
    const float py = w.ay + t * dy;

    const float dist  = PointDistance(px, py, cx, cy);
    const float reach = radius + Arena::kWallHalfThickness;
    h.hit         = dist <= reach;
    h.penetration = reach - dist;
    // Push direction = from the wall surface toward the circle center.
    h.pushDir = PointDirection(px, py, cx, cy);
    if (!h.hit)
        h.penetration = 0.0f;
    return h;
}

WallHit CircleVsOuterWalls(float cx, float cy, float radius)
{
    WallHit best{};
    best.hit       = false;
    best.wallIndex = -1;
    const auto& walls = OuterWalls();
    for (const WallSegment& w : walls)
    {
        const WallHit h = CircleVsWall(cx, cy, radius, w);
        if (h.hit && (!best.hit || h.penetration > best.penetration))
            best = h;
    }
    return best;
}

CoreHit CircleVsCore(float cx, float cy, float radius, float coreScale)
{
    CoreHit h{};
    const float coreR = Arena::kCoreSpriteRadius * coreScale;
    const float dist  = PointDistance(Arena::kCenterX, Arena::kCenterY, cx, cy);
    const float reach = radius + coreR;
    h.hit         = dist <= reach;
    h.penetration = h.hit ? (reach - dist) : 0.0f;
    h.outwardDir  = PointDirection(Arena::kCenterX, Arena::kCenterY, cx, cy);
    return h;
}

//==================================================================================
// Reflection
//==================================================================================
float ReflectDir(float incomingDir, float wallAngle)
{
    // pEntity:27 — _dir = 2 * _wallAngle - _dir. Wrap for tidiness (lengthdir is
    // periodic so the wrap is behaviour-preserving).
    return Wrap360(2.0f * wallAngle - incomingDir);
}

float WallAngleForReflection(float incomingDir, float wallImageAngle, bool flipped)
{
    const float flip = flipped ? 180.0f : 0.0f;
    // pEntity:8 — wall reference = image_angle + 90 + flipped*180.
    const float wallRef   = wallImageAngle + 90.0f + flip;
    // pEntity:8 — _angleDifference = angle_difference(_dir, wallRef).
    const float angleDiff = AngleDifference(incomingDir, wallRef);
    // pEntity:22-26 — clamp the incidence to +-85 then back out the wall angle.
    float wallAngle = incomingDir - std::clamp(angleDiff, -85.0f, 85.0f) - flip - 90.0f;
    return Wrap360(wallAngle);
}

bool ShouldReflect(float incomingDir, float wallImageAngle, bool flipped)
{
    const float flip    = flipped ? 180.0f : 0.0f;
    const float wallRef = wallImageAngle + 90.0f + flip;
    // pEntity:19 — if (_angleDifference <= 90).
    return AngleDifference(incomingDir, wallRef) <= 90.0f;
}

float ReflectOffWall(float incomingDir, float wallImageAngle, bool flipped)
{
    const float wallAngle = WallAngleForReflection(incomingDir, wallImageAngle, flipped);
    return ReflectDir(incomingDir, wallAngle);
}

//==================================================================================
// Push-out
//==================================================================================
int MarchIntoContact(float& px, float& py, float dir,
                     const std::function<bool(float, float)>& inWall, int maxSteps)
{
    // pEntity:13-16 / 34-37 — while NOT meeting a wall, step 1px along dir.
    const float oneX = LengthDirX(1.0f, dir);
    const float oneY = LengthDirY(1.0f, dir);
    int steps = 0;
    while (steps < maxSteps && !inWall(px, py))
    {
        px += oneX;
        py += oneY;
        ++steps;
    }
    return steps;
}

float CoreRedirectDir(float px, float py)
{
    // pEntity:50 — point_direction(oCore.x, oCore.y, x, y): fling radially outward.
    return PointDirection(Arena::kCenterX, Arena::kCenterY, px, py);
}

//==================================================================================
// Player death
//==================================================================================
bool PlayerWallDeath(bool wallHit, bool playerHasMoved, bool wallFlipped, float deathDelay)
{
    // pEntity:44-48 — death requires a wall touch + playerHasMoved, and either the
    // wall is non-flipped (outer) OR the death delay has run out.
    if (!wallHit || !playerHasMoved)
        return false;
    return (!wallFlipped) || (deathDelay <= 0.0f);
}

//==================================================================================
// Self-test
//==================================================================================
bool CollisionSelfTest(std::string& collideCsv, std::string& deathLog)
{
    bool pass = true;

    // ---- Reflection table: fire a non-player probe at 8 incidences into a wall ----
    std::ostringstream csv;
    csv << "incident,wallAngle,reflected,expectedMirror,mirrorDiff,reflProp,pass\n";

    // Use the top outer wall's surface direction (image_angle = 0) as the mirror
    // line. ReflectDir is the pure mirror the non-player probe undergoes.
    const float wallAngle = OuterWalls()[0].angle; // 0 deg
    const float incidents[8] = {0.0f, 45.0f, 90.0f, 135.0f, 180.0f, 225.0f, 270.0f, 315.0f};

    for (float inc : incidents)
    {
        const float reflected = ReflectDir(inc, wallAngle);

        // Task assertion: |reflected - (2*wallAngle - incident)| <= 2 (wrap-safe).
        const float mirrorTarget = 2.0f * wallAngle - inc;
        const float mirrorDiff   = std::fabs(AngleDifference(reflected, mirrorTarget));

        // Mirror invariant (non-tautological regression guard): reflecting across a
        // line NEGATES the angle to that line -> angle(reflected,line) ==
        // -angle(incident,line). Compared wrap-safe so the +-180 boundary (incident
        // parallel to the wall) does not spuriously read as 360.
        const float a = AngleDifference(inc, wallAngle);
        const float b = AngleDifference(reflected, wallAngle);
        const float reflProp = std::fabs(AngleDifference(b, -a));

        // Double-reflection round-trip must also recover the incident direction.
        const float roundTrip = std::fabs(AngleDifference(ReflectDir(reflected, wallAngle), inc));

        const bool ok = (mirrorDiff <= 2.0f) && (reflProp <= 2.0f) && (roundTrip <= 2.0f);
        pass = pass && ok;

        csv << inc << "," << wallAngle << "," << reflected << ","
            << Wrap360(mirrorTarget) << "," << mirrorDiff << "," << reflProp << ","
            << (ok ? "PASS" : "FAIL") << "\n";
    }

    // ---- Clamp demonstration: grazing incidence engages the +-85 clamp ----
    // image_angle 0, flipped=false: a near-grazing incidence (5 deg off the wall
    // line) has |angleDiff| ~= 85, so the clamp shifts wallAngle off image_angle.
    {
        const float grazing = 5.0f; // ~grazing along the wall
        const float wa = WallAngleForReflection(grazing, 0.0f, false);
        const bool clampOk = ShouldReflect(grazing, 0.0f, false);
        csv << "# clamp grazingIncident=" << grazing << " wallAngle=" << wa
            << " shouldReflect=" << (clampOk ? 1 : 0) << "\n";
    }

    collideCsv = csv.str();

    // ---- Death predicate ----
    std::ostringstream dlog;
    dlog << "case,wallHit,playerHasMoved,wallFlipped,deathDelay,death,expected,pass\n";

    auto deathCase = [&](const char* name, bool wallHit, bool moved, bool flipped,
                         float delay, bool expected) {
        const bool got = PlayerWallDeath(wallHit, moved, flipped, delay);
        const bool ok  = (got == expected);
        pass = pass && ok;
        dlog << name << "," << wallHit << "," << moved << "," << flipped << ","
             << delay << "," << got << "," << expected << "," << (ok ? "PASS" : "FAIL")
             << "\n";
    };

    // Player probe overlapping a NON-flipped outer wall, playerHasMoved -> DEATH.
    deathCase("outer_nonflipped_moved", true, true, false, 99.0f, true);
    // Same wall but the player has NOT moved yet -> survives.
    deathCase("outer_nonflipped_notmoved", true, false, false, 99.0f, false);
    // Flipped (boss) wall with deathDelay still positive -> survives.
    deathCase("boss_flipped_delayActive", true, true, true, 5.0f, false);
    // Flipped (boss) wall once deathDelay elapsed -> DEATH.
    deathCase("boss_flipped_delayElapsed", true, true, true, 0.0f, true);
    // No wall touched -> survives regardless.
    deathCase("no_wall_touch", false, true, false, 0.0f, false);

    deathLog = dlog.str();

    return pass;
}

} // namespace Engine::CriticalCore
