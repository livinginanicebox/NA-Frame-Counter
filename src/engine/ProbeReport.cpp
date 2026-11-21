#include "ProbeReport.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace na_frame_counter::engine {

namespace {

std::string fmtFixed(double value, int decimals) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", std::max(0, decimals), value);
    return std::string(buf);
}

} // namespace

std::string summarizeSession(SessionStats const& stats, core::ModSettingsSnapshot const& cfg) {
    const int dec = std::clamp(cfg.subFrameDecimals, 0, 3);
    const double avgPhase = (stats.subFrameCount > 0)
        ? stats.phaseSum / static_cast<double>(stats.subFrameCount)
        : 0.0;
    return "inputs " + std::to_string(stats.totalMeasured) +
        " | 1F " + std::to_string(stats.framePerfects) +
        " | avg +" + fmtFixed(avgPhase, dec) + "F" +
        " | CBF " + std::to_string(stats.cbfCount) +
        " | pts " + fmtFixed(stats.totalPoints, std::clamp(cfg.pointsDecimals, 1, 4));
}

SessionReport buildSessionReport(
    SessionStats const& stats,
    std::vector<SubFrameSample> const& samples,
    core::ModSettingsSnapshot const& cfg
) {
    SessionReport out;
    out.title = "NA Frame Counter - Session Report";

    const int dec = std::clamp(cfg.subFrameDecimals, 0, 3);
    const double avgPhase = (stats.subFrameCount > 0)
        ? stats.phaseSum / static_cast<double>(stats.subFrameCount)
        : 0.0;

    std::size_t cbfNeeded = 0;
    std::size_t overLimit = 0;
    std::uint64_t totalTicks = 0;
    double worstPhase = 0.0;
    double bestScore = 0.0;
    double scoreSum = 0.0;
    std::size_t scored = 0;

    for (auto const& sample : samples) {
        if (sample.overLimit) {
            ++overLimit;
            continue;
        }
        totalTicks += sample.frames;
        worstPhase = std::max(worstPhase, sample.phase);
        scoreSum += sample.score;
        ++scored;
        if (sample.score > bestScore) bestScore = sample.score;
        if (sample.cbfRequired) ++cbfNeeded;
    }

    const double avgScore = scored > 0 ? scoreSum / static_cast<double>(scored) : 0.0;
    const double avgTicks = scored > 0
        ? static_cast<double>(totalTicks) / static_cast<double>(scored)
        : 0.0;

    std::string body;
    body += "Attempts: " + std::to_string(stats.attempts) + "\n";
    body += "Inputs measured: " + std::to_string(stats.totalMeasured) + "\n";
    body += "Frame perfects (1F): " + std::to_string(stats.framePerfects) +
        " (" + std::to_string(stats.attemptFramePerfects) + " this attempt)\n";
    body += "Average window: " + fmtFixed(avgTicks, 2) + "F\n";
    body += "Average sub-frame phase: +" + fmtFixed(avgPhase, dec) + "F ("
        + fmtFixed(core::framesToMillis(avgPhase), 2) + " ms)\n";
    body += "Worst phase: +" + fmtFixed(worstPhase, dec) + "F\n";
    body += "Sub-frame perfects (1F, <=0.25 phase): " + std::to_string(stats.subFramePerfects) + "\n";
    body += "Average precision score: " + fmtFixed(avgScore, 1) + "/100 (best "
        + fmtFixed(bestScore, 1) + ")\n";
    body += "Best 1F streak: " + std::to_string(stats.bestStreak) + "\n";
    body += "CBF readings: " + std::to_string(stats.cbfCount) +
        " (" + std::to_string(cbfNeeded) + " required)\n";
    body += "Windows over limit: " + std::to_string(overLimit) + "\n";
    body += "Total points: " + fmtFixed(stats.totalPoints, std::clamp(cfg.pointsDecimals, 1, 4)) + "\n";
    body += "Best L*: " + fmtFixed(stats.bestLStar, 2) + " s/s\n";
    out.body = body;

    std::string csv = "index,frames,phase,window_frames,score,cbf,cbf_required,over_limit,release\n";
    std::size_t index = 0;
    for (auto const& sample : samples) {
        ++index;
        csv += std::to_string(index) + ",";
        csv += std::to_string(sample.frames) + ",";
        csv += fmtFixed(sample.phase, 3) + ",";
        csv += fmtFixed(static_cast<double>(sample.frames) + sample.phase, 3) + ",";
        csv += fmtFixed(sample.score, 2) + ",";
        csv += sample.cbf ? "1," : "0,";
        csv += sample.cbfRequired ? "1," : "0,";
        csv += sample.overLimit ? "1," : "0,";
        csv += sample.isRelease ? "1\n" : "0\n";
    }
    out.csv = csv;

    return out;
}

}
