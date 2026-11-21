#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace na_frame_counter::math {

// ---------------------------------------------------------------------------
// CBF / "click between frames" classification.
//
// A CBF pattern means the player's input is documented as landing *between* two
// 240 Hz physics ticks. On a standard GD install both ticks run the same step,
// so an input strictly between them is applied at the *next* tick and behaves
// exactly like a whole-frame input. On framerate-unlocked / "CBF" setups the
// physics step and the render step disagree, so the sub-tick phase becomes
// behaviourally significant.
//
// We therefore cannot "just" declare CBF from a fractional number: we need to
// compare the measured placement phase against a model of when real clicks are
// known to land, and only differ from the tick boundary when the evidence
// actually supports it.
// ---------------------------------------------------------------------------

enum class CbfVerdict : int {
    Disabled = 0,   // Feature turned off in settings.
    Standard = 1,   // Whole-frame input; no sub-tick evidence.
    Boundary = 2,   // Input sits on the tick boundary (phase ~0).
    NearBoundary = 3, // Within the configured guard band; reported but not flagged.
    Cbf = 4,        // Input genuinely sits between frames.
    CbfRequired = 5, // Input would be impossible without a sub-tick placement.
};

struct CbfConfig {
    bool enabled = true;
    // How far (in frames) from a tick boundary an input must be before it is
    // considered "between frames". 0.10F = ~0.42 ms.
    double guardFrames = 0.10;
    // How far the very first / very last input of an attempt may drift before
    // we call it "required" (no legal whole-frame placement exists).
    double requireFrames = 0.25;
    // Treat releases and presses the same way (CBF matters for both).
    bool includeReleases = true;
};

struct CbfSample {
    bool isRelease = false;
    // Measured whole-tick window and the phase within that tick.
    std::uint64_t frames = 0;
    double phase = 0.0;
    // Signed distance of the placement from the nearest tick boundary.
    double offsetFrames = 0.0;
};

struct CbfAnalysis {
    CbfVerdict verdict = CbfVerdict::Standard;
    // Fraction of *whole* frames the input was away from a boundary (always >= 0).
    double offsetFrames = 0.0;
    // Signed: negative = early of the boundary, positive = late.
    double signedOffsetFrames = 0.0;
    // Human readable flavour text for the toast ("CBF (0.42F between)").
    std::string label;
    // True when the placement is only possible with sub-tick input.
    bool required = false;
    // 0..1 confidence that this reading reflects CBF rather than measurement noise.
    double confidence = 0.0;
};

// Classify a single sample.
CbfAnalysis analyzeCbf(CbfSample const& sample, CbfConfig const& cfg);

// Classify a whole attempt's samples. When at least half the samples are CBF,
// the attempt itself is reported as "CBF required" which is what a toast wants
// to say when a run simply cannot be done on a normal 240 Hz install.
CbfAnalysis analyzeAttempt(std::vector<CbfSample> const& samples, CbfConfig const& cfg);

// Short tag for a verdict: "OK", "CBF", "CBF!", "OFF".
char const* cbfTag(CbfVerdict verdict);

// Full sentence for the toast body.
std::string cbfDescription(CbfAnalysis const& analysis);

}
