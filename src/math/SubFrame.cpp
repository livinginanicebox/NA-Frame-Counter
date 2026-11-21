#include "SubFrame.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace na_frame_counter::math {

namespace {

constexpr double kMaxSubFrame = 1.0 - 1e-6;

double clampPhase(double phase) {
    if (!std::isfinite(phase)) return 0.0;
    return std::clamp(phase, 0.0, kMaxSubFrame);
}

std::string trimFixed(std::string text) {
    while (!text.empty() && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text.empty() ? "0" : text;
}

} // namespace

FrameWindow splitFrameCount(double fractionalFrames) {
    FrameWindow out;
    if (!std::isfinite(fractionalFrames) || fractionalFrames <= 0.0) {
        return out;
    }
    const double whole = std::floor(fractionalFrames);
    const double frac = fractionalFrames - whole;
    out.frames = static_cast<std::uint64_t>(whole);
    out.phase = clampPhase(frac);
    return out;
}

double frameDelta(std::uint64_t measuredFrames, double targetFrames) {
    if (!std::isfinite(targetFrames)) return 0.0;
    return static_cast<double>(measuredFrames) - targetFrames;
}

double subFrameResidual(std::uint64_t measuredFrames, double targetFrames) {
    if (!std::isfinite(targetFrames) || targetFrames <= 0.0) return 0.0;
    // Distance to the nearest tick boundary the player was aiming at.
    const double measured = static_cast<double>(measuredFrames);
    const double delta = measured - targetFrames;
    if (delta <= 0.0) return 0.0;

    // Anything beyond half a tick of drift cannot be recovered by sub-frame
    // placement: the input would have to have happened before the tick.
    if (delta >= 0.5) return 1.0;
    return std::clamp(delta / 0.5, 0.0, 1.0);
}

double subFrameHitChance(double windowFrames, double jitterFrames) {
    if (!std::isfinite(windowFrames) || windowFrames <= 0.0) return 0.0;
    const double jitter = (std::isfinite(jitterFrames) && jitterFrames > 0.0) ? jitterFrames : 1.0;
    const double chance = windowFrames / jitter;
    return std::clamp(chance, 0.0, 1.0);
}

double subFrameAlignScore(std::uint64_t frames, double phase) {
    const double f = std::max(1.0, static_cast<double>(frames));
    const double p = clampPhase(phase);
    const double windowPenalty = std::pow(0.55, f - 1.0);
    const double phasePenalty = 1.0 - 0.6 * p * p;
    const double score = 100.0 * windowPenalty * phasePenalty;
    return std::clamp(score, 0.0, 100.0);
}

double subFramePrecisionScore(std::uint64_t frames, double phase, double jitterFrames) {
    const double f = std::max(1.0, static_cast<double>(frames));
    const double p = clampPhase(phase);
    const double jitter = (std::isfinite(jitterFrames) && jitterFrames > 0.0) ? jitterFrames : 1.0;

    // Base: how tight the tick window was (1F is the gold standard).
    const double windowScore = std::pow(0.50, f - 1.0);
    // Phase: how well the (only possible) tick was centred inside the window.
    const double phaseScore = subFrameAlignScore(frames, p) / 100.0;
    // Consistency: probability of landing it again by chance.
    const double luckScore = std::sqrt(subFrameHitChance(f, jitter));

    const double mixed = 0.45 * windowScore + 0.35 * phaseScore + 0.20 * luckScore;
    return std::clamp(100.0 * mixed, 0.0, 100.0);
}

double subFramePoints(double basePoints, std::uint64_t frames, double phase, double hardness) {
    if (!std::isfinite(basePoints) || basePoints <= 0.0) return 0.0;
    if (frames == 0) return 0.0;

    const double p = clampPhase(phase);
    const double h = (std::isfinite(hardness) && hardness >= 0.0) ? hardness : 1.0;

    // Phase penalty: worst case (phase 0.5) removes up to 15% of a tick's worth
    // of credit at hardness 1 (the value the UI calls "Sub-Frame Hardness").
    // Applying it multiplicatively keeps 1F >= 2F >= 3F ordering intact for
    // every phase combination, and keeps frame-perfects strictly dominant.
    // 4*p*(1-p) peaks at 0.5 (the worst possible placement, dead centre between
    // two ticks) and is exactly 0 at a tick boundary (a perfectly timed input).
    const double phaseGain = 1.0 - 0.15 * h * (4.0 * p * (1.0 - p));
    const double rounded = std::clamp(phaseGain, 0.25, 1.0);
    return basePoints * rounded;
}

std::string formatFrames(
    std::uint64_t frames,
    double phase,
    bool overLimit,
    int maxWindow,
    int decimals
) {
    if (overLimit) {
        return ">" + std::to_string(std::max(0, maxWindow));
    }
    if (frames == 0 && clampPhase(phase) <= 0.0) {
        return "0.0";
    }

    const int dec = std::clamp(decimals, 0, 3);
    if (dec == 0) {
        return std::to_string(frames);
    }

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", dec, static_cast<double>(frames) + clampPhase(phase));
    return trimFixed(std::string(buf));
}

InterpolatedWindow interpolateWindow(
    std::uint64_t earlyTicks,
    std::uint64_t lateTicks,
    double boundaryFraction,
    bool earlyHitLimit,
    bool lateHitLimit
) {
    InterpolatedWindow out;

    // Whole-tick window: the input itself plus every tick that still works on
    // each side. Clamped to 1 so a single-tick window never reports < 1F.
    const double whole = 1.0 + static_cast<double>(earlyTicks) + static_cast<double>(lateTicks);
    out.windowFrames = std::max(1.0, whole);
    out.fraction = 0.0;
    out.phase = (whole > 1.0) ? static_cast<double>(earlyTicks) / whole : 0.5;

    if (earlyHitLimit || lateHitLimit) {
        out.tightSide = earlyHitLimit && lateHitLimit ? ClipSide::Both
            : (earlyHitLimit ? ClipSide::Early : ClipSide::Late);
        out.earlyFrames = static_cast<double>(earlyTicks);
        out.lateFrames = static_cast<double>(lateTicks);
        return out;
    }

    if (whole <= 1.0) {
        // Exactly one tick: the geometric fraction is the *only* sub-frame
        // information available, and it is a real 1.x window.
        const double frac = std::clamp(boundaryFraction, 0.0, 0.9999);
        out.windowFrames = 1.0 + frac;
        out.fraction = frac;
        out.phase = 0.5;
        out.earlyFrames = 0.5;
        out.lateFrames = 0.5;
        return out;
    }

    // One of the two boundaries (nearly always the late/easier side) is the
    // binding one. Scale its partial tick by how far the hitbox had penetrated
    // into the failing object: full penetration = the whole tick still works.
    const double frac = std::clamp(boundaryFraction, 0.0, 0.9999);
    const double partial = frac;

    if (earlyTicks <= lateTicks) {
        out.earlyFrames = static_cast<double>(earlyTicks) + partial;
        out.lateFrames = static_cast<double>(lateTicks);
        out.tightSide = ClipSide::Early;
    } else {
        out.earlyFrames = static_cast<double>(earlyTicks);
        out.lateFrames = static_cast<double>(lateTicks) + partial;
        out.tightSide = ClipSide::Late;
    }

    out.windowFrames = 1.0 + out.earlyFrames + out.lateFrames;
    out.fraction = std::clamp(out.windowFrames - std::floor(out.windowFrames), 0.0, 0.9999);
    out.phase = std::clamp(out.earlyFrames / std::max(1e-9, out.earlyFrames + out.lateFrames), 0.0, 1.0);
    return out;
}

ClipSide classifyClip(std::uint64_t frames, double phase, double targetFrames) {
    if (!std::isfinite(targetFrames) || targetFrames <= 0.0) return ClipSide::None;
    if (frames == 0) return ClipSide::Both;

    const double measured = static_cast<double>(frames) + clampPhase(phase);
    const double delta = measured - targetFrames;
    if (std::abs(delta) <= kTimeEps) return ClipSide::None;

    // Inside the same tick: the player's position within that tick is the tell.
    if (frames == static_cast<std::uint64_t>(std::floor(targetFrames))) {
        const double p = clampPhase(phase);
        if (p <= 0.25) return ClipSide::Early;
        if (p >= 0.75) return ClipSide::Late;
        return ClipSide::None;
    }
    if (delta < 0.0) return ClipSide::Early;
    return ClipSide::Late;
}

char const* clipSideName(ClipSide side) {
    switch (side) {
        case ClipSide::Early: return "early";
        case ClipSide::Late: return "late";
        case ClipSide::Both: return "both";
        case ClipSide::None:
        default: return "none";
    }
}

}
