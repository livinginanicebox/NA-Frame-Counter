#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace na_frame_counter::math {

// Geometry / timing helpers for sub-frame (fractional physics frame) precision.
//
// GD physics ticks at 240 Hz. A *frame* is 1/240 s. A *sub-frame* is any real
// offset inside that grid, e.g. 0.4F = 1.6667 ms, 3.2F = 13.3333 ms, 1.3F = 5.4167 ms.
// The engine can only resolve a click to a whole 240 Hz tick, but we can still
// report (and verify) the *geometric* depth of an input inside its tick, which is
// what makes a click feel like "0.4 frames late" instead of "1 frame".

constexpr double kPhysicsHz = 240.0;
constexpr double kTickMillis = 1000.0 / kPhysicsHz; // 4.166666...
constexpr double kTimeEps = 1e-9;

struct FrameWindow {
    // Whole-tick window measured by the probe (1, 2, 3, ... frames).
    std::uint64_t frames = 0;
    // Geometry-derived fraction of a tick at which the input actually landed.
    // 0.0 = exactly on the tick, 0.5 = half a tick late, etc.
    double phase = 0.0;
    bool overLimit = false;
};

// Wraps a fractional frame count into a whole count + [0,1) phase.
// 3.2 -> {3, 0.2}; 0.4 -> {0, 0.4}; -0.4 -> {0, 0.6} is *not* used (negative
// inputs are clamped to 0 for display, see clampFrameCount).
FrameWindow splitFrameCount(double fractionalFrames);

// Signed fractional frame distance between a measured tick count and a target.
// Positive means the measured window is wider (easier) than the target.
double frameDelta(std::uint64_t measuredFrames, double targetFrames);

// Best possible sub-frame position inside a tick, in frames:
// a player physically cannot place an input at exactly 0.0 of a tick, the
// earliest realistic placement is roughly half a tick of reaction jitter.
// Returns the *residual* difficulty, 0 = perfectly placed.
double subFrameResidual(std::uint64_t measuredFrames, double targetFrames);

// Chance that a uniformly random input lands inside the given window, in
// frames. Used for the "(CBF)" probability badge: 1 frame out of N frames of
// "human jitter budget" is 1/N.
double subFrameHitChance(double windowFrames, double jitterFrames);

// ---------------------------------------------------------------------------
// Sub-frame scoring. These are pure functions so they are unit-testable.
// ---------------------------------------------------------------------------

// "Align score" 0..100: how close an input was to the exact tick boundary.
// 1.00F window at phase 0 => 100, at phase 0.5 => ~70 (still frame perfect),
// wide windows decay quickly.
double subFrameAlignScore(std::uint64_t frames, double phase);

// "Precision score" 0..100 combining the tick window and the phase residual.
double subFramePrecisionScore(std::uint64_t frames, double phase, double jitterFrames);

// Points for a measured window, with a continuous sub-frame penalty applied on
// top of the base (whole-frame) formula. A 1F window at phase 0.5 is worth
// slightly less than a 1F at phase 0.0, and a 4.4F window is worth strictly
// less than a 4.0F window.
double subFramePoints(double basePoints, std::uint64_t frames, double phase, double hardness);

// Round-trip helper used by the HUD and the toast: "1.0", "3.2", "0.4", ">12".
std::string formatFrames(std::uint64_t frames, double phase, bool overLimit, int maxWindow, int decimals);

// ---------------------------------------------------------------------------
// Window interpolation.
//
// The probe resolves *whole* ticks: it finds that a press may be N-1 ticks early
// and M-1 ticks late (N+M-1 total). The real boundary between "works" and
// "dies" lies inside the first failing tick, at a fraction determined by how far
// the player's hitbox had already travelled into (or out of) the object.
// `boundaryFraction` in [0,1) is that geometric penetration fraction.
//
// interpolateWindow returns the *continuous* window width, e.g. 3.2F, plus the
// position of the original input inside it (phase).
// ---------------------------------------------------------------------------

enum class ClipSide : int {
    None = 0,
    Early = 1,
    Late = 2,
    Both = 3,
};

struct InterpolatedWindow {
    // Continuous window width in frames (>= 1.0 for a valid 1F+ window).
    double windowFrames = 1.0;
    // Fractional part of windowFrames, in [0,1). This is what the HUD prints.
    double fraction = 0.0;
    // Position of the original input inside the window, 0..1 (0 = earliest).
    double phase = 0.0;
    // Slack before / after the input, in continuous frames.
    double earlyFrames = 0.0;
    double lateFrames = 0.0;
    // Which side is the binding constraint.
    ClipSide tightSide = ClipSide::None;
};

InterpolatedWindow interpolateWindow(
    std::uint64_t earlyTicks,
    std::uint64_t lateTicks,
    double boundaryFraction,
    bool earlyHitLimit,
    bool lateHitLimit
);

// Produces a compact human description of why an input was clipped, used by the
// toast notification: which side failed.

// Classifies a measured sub-frame placement relative to the *ideal* tick.
// Inputs that land in the first half of a tick are "early", the second half
// "late", anything landing on another tick is fine.
ClipSide classifyClip(std::uint64_t frames, double phase, double targetFrames);

char const* clipSideName(ClipSide side);

}
