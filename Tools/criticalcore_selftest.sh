#!/usr/bin/env bash
#
# criticalcore_selftest.sh
# -------------------------
# CI/QA runner for Example 26 (Critical Core 2) headless self-tests.
#
#   1. runs `./26_CriticalCore --selftest` from build/bin
#   2. asserts it exited 0 (each subsystem's internal assertion drives this)
#   3. asserts every expected artifact exists in .omo/evidence/
#   4. greps each artifact for its PASS threshold token (and that no row FAILed)
#   5. prints "ALL PASS" on success, or the first failing check + exits non-zero.
#
# Runs from anywhere: paths are resolved relative to this script's location.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN_DIR="${REPO_ROOT}/build/bin"
EVIDENCE_DIR="${REPO_ROOT}/.omo/evidence"
EXE="26_CriticalCore"

fail() {
    echo "FAIL: $*"
    exit 1
}

# ---- 1. binary present ----
[ -x "${BIN_DIR}/${EXE}" ] || fail "binary not found/executable: ${BIN_DIR}/${EXE} (build first: cmake --build build)"

# ---- 2. run --selftest from build/bin, assert exit 0 ----
echo "==> running ${EXE} --selftest (cwd=${BIN_DIR})"
( cd "${BIN_DIR}" && "./${EXE}" --selftest )
rc=$?
echo "==> exit code: ${rc}"
[ "${rc}" -eq 0 ] || fail "--selftest exited ${rc} (a subsystem assertion failed)"

# ---- 3+4. each artifact: exists, non-empty, threshold token present, no FAIL ----
# checkArtifact <file> <must-contain-regex> <human-description>
checkArtifact() {
    local file="${EVIDENCE_DIR}/$1"
    local needle="$2"
    local desc="$3"
    [ -f "${file}" ]  || fail "artifact missing: $1"
    [ -s "${file}" ]  || fail "artifact empty: $1"
    if [ -n "${needle}" ]; then
        grep -Eq "${needle}" "${file}" || fail "$1: threshold check failed (expected /${needle}/) -- ${desc}"
    fi
    echo "  ok: $1 (${desc})"
}

# refuteToken <file> <regex> <description>: fails if the regex IS present.
refuteToken() {
    local file="${EVIDENCE_DIR}/$1"
    local needle="$2"
    local desc="$3"
    if grep -Eq "${needle}" "${file}"; then
        fail "$1: found a failing marker /${needle}/ -- ${desc}"
    fi
}

echo "==> checking artifacts in ${EVIDENCE_DIR}"

# GameClock: final result row must read PASS.
checkArtifact "task-14-clock.csv"      "result,PASS"          "GameClock fixed-step result row PASS"

# GmHelpers: per-row pass column; a failing assertion ends a row with ',0'.
checkArtifact "task-8-helpers.csv"     "name,expected,actual,pass" "GmHelpers header present"
refuteToken   "task-8-helpers.csv"     ",0$"                  "GmHelpers has a failing (pass=0) row"

# RoundConfig: final RESULT row PASS.
checkArtifact "task-9-rounds.csv"      "RESULT,PASS"          "RoundConfig formulas/patterns RESULT PASS"

# AnimCurve: no PASS token (it is a t,value LUT dump) -- assert endpoints pinned.
checkArtifact "task-5-animcurve.csv"   "^0.000000,0.000000$"  "AnimCurve Evaluate(0)==0"
checkArtifact "task-5-animcurve.csv"   "^1.000000,1.000000$"  "AnimCurve Evaluate(1)==1"

# Collision: every reflection row must say PASS; none may say FAIL.
checkArtifact "task-17-collide.csv"    "PASS"                 "Collision reflection rows present"
refuteToken   "task-17-collide.csv"    "FAIL"                 "Collision reflection row FAILED"

# Collision death log: must exercise the death path; none may FAIL.
# Header has a 'death' column (0/1); the moved-into-nonflipped-wall case must die
# (death=1, expected=1) -> row ends ',1,1,PASS'.
checkArtifact "task-17-death.txt"      "death,expected,pass"  "Collision death-predicate log header"
checkArtifact "task-17-death.txt"      ",1,1,PASS"            "Collision death case (death=1, expected=1)"
refuteToken   "task-17-death.txt"      "FAIL"                 "Collision death-predicate row FAILED"

# CameraShake: RESULT row carries pass=1.
checkArtifact "task-19-shake.csv"      "pass=1"               "CameraShake envelope RESULT pass=1"

# MusicController beat clock: result row PASS.
checkArtifact "task-18-beatlog.csv"    "result,PASS"          "MusicController beat result PASS"

# Leaderboard: final RESULT PASS; no per-row FAIL.
checkArtifact "task-30-leaderboard.txt" "RESULT PASS"         "Leaderboard RESULT PASS"
refuteToken   "task-30-leaderboard.txt" "FAIL"                "Leaderboard row FAILED"

echo
echo "ALL PASS"
exit 0
