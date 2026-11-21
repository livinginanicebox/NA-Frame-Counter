#include "CbfDetector.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace na_frame_counter::math {

namespace {

std::string fmt2(double value) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f", value);
    std::string out(buf);
    while (!out.empty() && out.back() == '0') out.pop_back();
    if (!out.empty() && out.back() == '.') out.pop_back();
    return out.empty() ? "0" : out;
}

double boundaryOffset(double phase) {
    if (!std::isfinite(phase)) return 0.0;
    double p = std::fmod(std::abs(phase), 1.0);
    if (p > 1.0) p = 0.0;
    // Distance to the *nearest* boundary: 0 at .0, 0.5 at .5
    const double dist = std::min(p, 1.0 - p);
    return std::clamp(dist, 0.0, 0.5);
}

} // namespace

CbfAnalysis analyzeCbf(CbfSample const& sample, CbfConfig const& cfg) {
    CbfAnalysis out;
    if (!cfg.enabled) {
        out.verdict = CbfVerdict::Disabled;
        out.label = "CBF: off";
        return out;
    }

    const bool releaseIgnored = sample.isRelease && !cfg.includeReleases;
    const double guard = std::max(0.0, std::isfinite(cfg.guardFrames) ? cfg.guardFrames : 0.10);
    const double requireF = std::max(guard, std::isfinite(cfg.requireFrames) ? cfg.requireFrames : 0.25);

    const double phase = std::isfinite(sample.phase) ? sample.phase : 0.0;
    const double nearest = boundaryOffset(phase);
    out.offsetFrames = nearest;
    out.signedOffsetFrames = (phase < 0.5) ? -nearest : nearest;

    if (sample.frames == 0) {
        out.verdict = releaseIgnored ? CbfVerdict::Standard : CbfVerdict::CbfRequired;
        out.required = !releaseIgnored;
        out.confidence = 1.0;
        out.label = "CBF: frame 0";
        return out;
    }

    if (nearest <= 1e-6) {
        out.verdict = CbfVerdict::Boundary;
        out.confidence = 1.0;
        out.label = "on-frame";
        return out;
    }

    if (nearest <= guard) {
        out.verdict = CbfVerdict::NearBoundary;
        out.confidence = 1.0 - (nearest / guard);
        out.label = "near-frame (" + fmt2(nearest) + "F)";
        return out;
    }

    // Genuinely between frames.
    const double span = std::max(1e-6, 0.5 - guard);
    out.confidence = std::clamp((nearest - guard) / span, 0.0, 1.0);
    out.required = nearest >= requireF;
    out.verdict = out.required ? CbfVerdict::CbfRequired : CbfVerdict::Cbf;
    out.label = std::string("CBF (") + fmt2(nearest) + "F " +
        (out.signedOffsetFrames < 0.0 ? "early" : "late") + ")";
    if (releaseIgnored) {
        out.verdict = CbfVerdict::Standard;
        out.required = false;
        out.label = "release ignored";
    }
    return out;
}

CbfAnalysis analyzeAttempt(std::vector<CbfSample> const& samples, CbfConfig const& cfg) {
    CbfAnalysis out;
    if (!cfg.enabled) {
        out.verdict = CbfVerdict::Disabled;
        out.label = "CBF: off";
        return out;
    }
    if (samples.empty()) {
        out.verdict = CbfVerdict::Standard;
        out.label = "no inputs";
        return out;
    }

    std::size_t cbfCount = 0;
    std::size_t requiredCount = 0;
    double worstOffset = 0.0;
    double signedWorst = 0.0;
    double confidenceSum = 0.0;

    for (auto const& sample : samples) {
        const auto one = analyzeCbf(sample, cfg);
        if (one.verdict == CbfVerdict::Cbf || one.verdict == CbfVerdict::CbfRequired) {
            ++cbfCount;
            if (one.required) ++requiredCount;
            if (one.offsetFrames > worstOffset) {
                worstOffset = one.offsetFrames;
                signedWorst = one.signedOffsetFrames;
            }
        }
        confidenceSum += one.confidence;
    }

    const double total = static_cast<double>(samples.size());
    out.offsetFrames = worstOffset;
    out.signedOffsetFrames = signedWorst;
    out.confidence = confidenceSum / total;

    const double ratio = static_cast<double>(cbfCount) / total;
    if (cbfCount == 0) {
        out.verdict = CbfVerdict::Standard;
        out.label = "all on-frame";
    } else if (requiredCount > 0 || ratio >= 0.5) {
        out.verdict = CbfVerdict::CbfRequired;
        out.required = true;
        out.label = "CBF needed";
    } else {
        out.verdict = CbfVerdict::Cbf;
        out.label = "partly CBF";
    }
    return out;
}

char const* cbfTag(CbfVerdict verdict) {
    switch (verdict) {
        case CbfVerdict::Disabled: return "OFF";
        case CbfVerdict::Boundary: return "FRAME";
        case CbfVerdict::NearBoundary: return "NEAR";
        case CbfVerdict::Cbf: return "CBF";
        case CbfVerdict::CbfRequired: return "CBF!";
        case CbfVerdict::Standard:
        default: return "OK";
    }
}

std::string cbfDescription(CbfAnalysis const& analysis) {
    switch (analysis.verdict) {
        case CbfVerdict::Disabled:
            return "Click-between-frames detection is disabled.";
        case CbfVerdict::Boundary:
            return "Input landed exactly on a 240 Hz tick - no sub-frame placement needed.";
        case CbfVerdict::NearBoundary:
            return "Input is within the boundary guard band (" + fmt2(analysis.offsetFrames) + "F).";
        case CbfVerdict::Cbf:
            return "Input sits between frames by " + fmt2(analysis.offsetFrames) +
                   "F - sub-frame placement in play.";
        case CbfVerdict::CbfRequired:
            return "This window needs a click between frames (" + fmt2(analysis.offsetFrames) +
                   "F " + (analysis.signedOffsetFrames < 0.0 ? "early" : "late") +
                   ") - impossible on a plain 240 Hz install.";
        case CbfVerdict::Standard:
        default:
            return "Whole-frame input - normal 240 Hz behaviour.";
    }
}

}
