#pragma once

#include <Geode/Geode.hpp>
#include <cstdint>

namespace na_frame_counter::engine {

// ---------------------------------------------------------------------------
// Sub-frame geometry
//
// GD's physics is a fixed 240 Hz tick, but *when inside a tick* a player presses
// still matters for reachability: the player's collision box can already be
// overlapping (or already clear of) the target object. By measuring how far the
// player's hitbox travelled past the edge of the object it was aiming at, we can
// express the input as a fractional frame, e.g. "0.4F late" or "3.2F".
//
// The result is deliberately conservative: we only report a non-zero phase when
// the geometry gives an unambiguous answer, otherwise 0 (whole frame).
// ---------------------------------------------------------------------------
struct SubFrameGeometry {
    // Fractional progress of the player's hitbox across the object, 0..1.
    double progress = 0.0;
    // Resulting phase (0 = exactly on the tick boundary).
    double phase = 0.0;
    // Signed offset in frames: negative = early, positive = late.
    double offsetFrames = 0.0;
    bool valid = false;
};

// Measures how deep an input was inside its tick for a *grounded cube press*
// against `target` (the object the click was aimed at). `fallback` is used when
// there is no usable target (rings, pads, orbs, aerial presses): the vertical
// velocity tells us how far into the jump arc the tick landed.
SubFrameGeometry measureSubFrame(
    cocos2d::CCPoint const& playerPos,
    cocos2d::CCPoint const& previousPos,
    cocos2d::CCRect const& targetRect,
    bool grounded,
    double yVelocity,
    double fallbackPhase
);

// Converts an arbitrary 0..1 progress to a phase in [0, 0.999].
double progressToPhase(double progress);

// Clamps/normalises any candidate phase; never returns NaN.
double sanitizePhase(double phase);

}
