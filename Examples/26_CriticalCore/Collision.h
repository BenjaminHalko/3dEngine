#pragma once

#include <array>
#include <functional>
#include <string>

// Octagon-arena collision subsystem for "Critical Core 2" (task 17).
//
// THIS FILE OWNS THE CANONICAL OCTAGON WALL GEOMETRY. Tasks 25 (Wall component)
// and 33 (level author) MUST consume OuterWalls()/BossWalls() here so all three
// share IDENTICAL geometry. Derived from oCore/Create_0.gml:35-52 (walls[] build)
// and rooms/rGame/rGame.yy (the 8 outer oWall instance placements).
//
// Conventions (locked by task 8 GmHelpers): DEGREE-based trig, y-DOWN screen
// space (LengthDirY = -len*sin), AngleDifference returns signed [-180,180].
// All angle math goes through GmHelpers so conventions match the source game.
//
// 2D ANALYTIC ONLY. No Bullet / PhysicsService (3D overkill). The Core's VISUAL
// octagon (oCore.polygonPoints) is DISTINCT from these collision walls[]: the
// visual is a filled polygon, collision uses the 8 outer + 8 boss wall segments.
namespace Engine::CriticalCore
{
//==================================================================================
// Arena constants (room 256x224, center = room_width/2, room_height/2)
//==================================================================================
namespace Arena
{
    constexpr float kCenterX = 128.0f; // room_width/2  (oCore/Create_0.gml:10)
    constexpr float kCenterY = 112.0f; // room_height/2 (oCore/Create_0.gml:11)

    // Octagon vertices sit at (+-24,+-104) & (+-104,+-24) from center.
    constexpr float kVertexRadius = 106.7708f; // sqrt(24^2 + 104^2): center -> vertex
    constexpr float kFlatApothem  = 104.0f;    // center -> axis edge inner face (top/bottom/left/right)
    constexpr float kDiagApothem  = 90.5097f;  // center -> diagonal edge inner face (sqrt(64^2+64^2))

    constexpr float kWallSpriteThickness = 4.0f; // sWall is 4px tall (sprites/sWall: height 4)
    constexpr float kWallHalfThickness   = 2.0f; // collision centerline +- 2px
    constexpr float kCoreSpriteRadius    = 104.0f; // sCore 208x208, origin (104,104) -> radius 104 @ scale 1
}

//==================================================================================
// Canonical wall segment (single source of truth)
//==================================================================================
struct WallSegment
{
    // Raw GameMaker instance data (what tasks 25/33 render/author):
    float x;        // instance origin (pivot) x — sWall origin is top-left (0,0)
    float y;        // instance origin (pivot) y
    float angle;    // image_angle (deg): the wall length-axis direction
    float length;   // 4 * image_xscale (sprite width 4 px * scale)
    bool  flipped;  // false = outer arena wall ; true = boss wall (spawned flipped)
    bool  bossWall; // true = boss wall (center-spawned, runtime-scaled)
    int   index;    // image_angle div 45 (octant id); -1 for boss walls (per GML)

    // Derived collision centerline (world space, y-down). Centerline is the
    // mid-height (local y=2) line of the 4px-tall sprite, from local x=0..length.
    float ax, ay;   // segment endpoint A
    float bx, by;   // segment endpoint B
    float cx, cy;   // centerline midpoint
};

// Builds a WallSegment from raw GameMaker instance params (origin, image_angle,
// image_xscale, flags). Computes the derived collision centerline.
WallSegment MakeWall(float x, float y, float angle, float scaleX, bool flipped, bool bossWall);

// The 8 OUTER arena walls (from rGame.yy oWall instances). CANONICAL.
const std::array<WallSegment, 8>& OuterWalls();

// The 8 BOSS walls at SPAWN (center-spawned, image_xscale 0 => length 0). They
// share the outer walls' 8 angles; task 20/21 grows image_xscale at runtime, so
// call MakeWall(centerX, centerY, angle, runtimeScale, true, true) per-frame to
// rebuild the live boss segment. CANONICAL angle set.
const std::array<WallSegment, 8>& BossWalls();

//==================================================================================
// Collision tests (plain data in / out — NO entity-class coupling)
//==================================================================================
struct WallHit
{
    bool  hit;
    float penetration; // (radius + halfThickness) - distToCenterline ; > 0 when hit
    float pushDir;     // deg: direction from wall toward circle (push-out normal)
    float wallAngle;   // wall.angle (image_angle) — feed WallAngleForReflection
    bool  flipped;
    bool  bossWall;
    int   wallIndex;   // index into the queried wall array (-1 if none)
};

// Circle vs ONE wall segment, thickened by Arena::kWallHalfThickness.
// Returns hit + the wall's angle + penetration depth.
WallHit CircleVsWall(float cx, float cy, float radius, const WallSegment& w);

// Circle vs ALL OuterWalls() — returns the DEEPEST penetration hit (or hit=false).
WallHit CircleVsOuterWalls(float cx, float cy, float radius);

struct CoreHit
{
    bool  hit;
    float penetration; // (radius + coreRadius) - dist ; > 0 when hit
    float outwardDir;  // deg: direction core-center -> circle (radial push-out)
};

// Circle vs the Core (a circle at arena center, radius = kCoreSpriteRadius*coreScale).
CoreHit CircleVsCore(float cx, float cy, float radius, float coreScale);

//==================================================================================
// Reflection (pEntity/Step_0.gml:19-30)
//==================================================================================
// PURE mirror: reflected = 2*wallAngle - incomingDir, wrapped to [0,360).
// (pEntity:27 `_dir = 2 * _wallAngle - _dir`.)
float ReflectDir(float incomingDir, float wallAngle);

// The wall surface angle WITH the +-85 clamp (pEntity:22-26):
//   wallRef   = image_angle + 90 + flipped*180
//   angleDiff = angle_difference(incomingDir, wallRef)            // [-180,180]
//   wallAngle = incomingDir - clamp(angleDiff,-85,85) - flipped*180 - 90, wrapped [0,360)
// When the clamp is inactive this collapses to image_angle; the clamp forces a
// minimum bounce at grazing angles.
float WallAngleForReflection(float incomingDir, float wallImageAngle, bool flipped);

// Reflection gate (pEntity:19): reflect only when angle_difference <= 90.
bool ShouldReflect(float incomingDir, float wallImageAngle, bool flipped);

// Full non-player reflection off a wall (clamp + mirror): pEntity:22-27.
// Returns the reflected direction; caller re-derives xSpd/ySpd via LengthDir.
float ReflectOffWall(float incomingDir, float wallImageAngle, bool flipped);

//==================================================================================
// Push-out (pixel-step march) — pEntity:9-17 (boss) / :31-41 (player) / :49-54 (core)
//==================================================================================
// Marches (px,py) by 1px steps along `dir` until inWall(px,py) is TRUE (i.e.
// while NOT meeting a wall, keep stepping). Mirrors the GML
//   while(!place_meeting(x,y,oWall)) { x += lengthdir_x(1,dir); y += lengthdir_y(1,dir); }
// used by boss walls (:13-16) and the player (:34-37). Returns steps taken.
int MarchIntoContact(float& px, float& py, float dir,
                     const std::function<bool(float, float)>& inWall, int maxSteps = 512);

// Core radial redirect (pEntity:49-54): new velocity direction = core-center ->
// entity, so a non-player overlapping the core is flung straight outward.
float CoreRedirectDir(float px, float py);

//==================================================================================
// Player-vs-wall death (pEntity/Step_0.gml:44-48)
//==================================================================================
//   if (object_index == oPlayer and oCore.playerHasMoved)
//       if (!_wall.flipped or deathDelay <= 0) GameOver();
// i.e. the player dies on touching a NON-flipped (outer) wall once it has moved,
// OR any wall once deathDelay has elapsed. wallHit = a wall was actually touched.
bool PlayerWallDeath(bool wallHit, bool playerHasMoved, bool wallFlipped, float deathDelay);

//==================================================================================
// Deterministic self-test (task 35 writes task-17-collide.csv + task-17-death.txt)
//==================================================================================
bool CollisionSelfTest(std::string& collideCsv, std::string& deathLog);

} // namespace Engine::CriticalCore
