#pragma once

#include "ProbeTypes.hpp"
#include "../core/FrameConfig.hpp"

#include <string>
#include <vector>

namespace na_frame_counter::engine {

// ---------------------------------------------------------------------------
// Session report
//
// A plain-text summary that can be copied to the clipboard from the mod menu or
// auto-shown on level completion. Deliberately pure (no cocos2d calls) so the
// formatting is unit-testable.
// ---------------------------------------------------------------------------
struct SessionReport {
    std::string title;
    std::string body;
    std::string csv;
};

// Builds the human-readable report + CSV dump from a session's stats and the
// sub-frame sample stream.
SessionReport buildSessionReport(
    SessionStats const& stats,
    std::vector<SubFrameSample> const& samples,
    core::ModSettingsSnapshot const& cfg
);

// One-line summary ("12 inputs | avg 0.31F | 4 CBF | 68.4 pts").
std::string summarizeSession(
    SessionStats const& stats,
    core::ModSettingsSnapshot const& cfg
);

}
