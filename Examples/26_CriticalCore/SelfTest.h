#pragma once

namespace Engine::CriticalCore
{
// Headless self-test aggregator for Example 26 (Critical Core 2).
//
// Runs EVERY deterministic, logic-only subsystem self-test with NO window, NO
// GraphicsSystem and NO GameWorld. Each test's CSV/text output is captured and
// written to .omo/evidence/<name>.csv|txt, a summary table is printed, and the
// function returns 0 iff every EXECUTED test passed (non-zero if any internal
// assertion failed).
//
// Subsystems whose self-test requires a live render / GameWorld (entity,
// coreShoot, player, bubble, projectiles, particles) are reported honestly as
// "N/A (needs live world)" and never counted as a pass.
//
// Invoked from WinMain.cpp when "--selftest" is present in argv; the normal
// App/window path is bypassed entirely.
int RunSelfTests();
} // namespace Engine::CriticalCore
