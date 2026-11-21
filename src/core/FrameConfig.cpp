#include "FrameConfig.hpp"
#include "../math/SubFrame.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace na_frame_counter::core {

int parseMaxWindowPreset(std::string_view value) {
    constexpr std::array<int, 7> kPresets = {10, 12, 17, 30, 60, 120, 240};
    for (int preset : kPresets) {
        if (value == std::to_string(preset)) return preset;
    }
    return 12;
}

LabelPlacement parseLabelPlacement(std::string_view value) {
    if (value == "Center" || value == "Inside") return LabelPlacement::Center;
    if (value == "Top" || value == "Above") return LabelPlacement::Top;
    if (value == "Right") return LabelPlacement::Right;
    return LabelPlacement::Left;
}

ColorPreset parseColorPreset(std::string_view value) {
    if (value == "NaN GD Fire") return ColorPreset::NangdFire;
    if (value == "Pastel") return ColorPreset::Pastel;
    if (value == "Monochrome") return ColorPreset::Monochrome;
    if (value == "Custom 1-5+" || value == "Custom") return ColorPreset::Custom;
    return ColorPreset::NangdClassic;
}

SoundStyle parseSoundStyle(std::string_view value) {
    if (value == "Soft Chime") return SoundStyle::SoftChime;
    if (value == "Crystal Ping") return SoundStyle::CrystalPing;
    if (value == "Retro Blip") return SoundStyle::RetroBlip;
    if (value == "Custom File") return SoundStyle::CustomFile;
    return SoundStyle::ClassicBell;
}

PitchCurve parsePitchCurve(std::string_view value) {
    if (value == "Linear") return PitchCurve::Linear;
    if (value == "Semitones") return PitchCurve::Semitones;
    return PitchCurve::Musical;
}

HudCorner parseHudCorner(std::string_view value) {
    if (value == "Top Right") return HudCorner::TopRight;
    if (value == "Bottom Right") return HudCorner::BottomRight;
    return HudCorner::TopLeft;
}

PointsCorner parsePointsCorner(std::string_view value) {
    if (value == "Middle Right") return PointsCorner::MiddleRight;
    if (value == "Bottom Right") return PointsCorner::BottomRight;
    return PointsCorner::TopRight;
}

PointsFormula parsePointsFormula(std::string_view value) {
    if (value == "60Hz FP (4/N)") return PointsFormula::FP60Equivalent;
    if (value == "120Hz FP (2/N)") return PointsFormula::FP120Equivalent;
    if (value == "Squared (1/N^2)") return PointsFormula::Squared240;
    if (value == "60Hz Align Odds") return PointsFormula::Align60Hz;
    if (value == "Sub-Frame Hard") return PointsFormula::SubFrameHard;
    return PointsFormula::FP240Inverse;
}

SubFrameMode parseSubFrameMode(std::string_view value) {
    if (value == "Whole Frames (1, 2, 3)") return SubFrameMode::WholeFrames;
    if (value == "Tick Offset (1+0.4)") return SubFrameMode::DecimalTick;
    return SubFrameMode::DecimalFrame;
}

int parseToastStyle(std::string_view value) {
    if (value == "Slide") return 0;
    if (value == "Pop") return 1;
    return 2;
}

CbfBadgeMode parseCbfBadgeMode(std::string_view value) {
    if (value == "Off") return CbfBadgeMode::Off;
    if (value == "Verbose") return CbfBadgeMode::Verbose;
    return CbfBadgeMode::Compact;
}

NandlDisplayMode parseNandlDisplayMode(std::string_view value) {
    if (value == "Base") return NandlDisplayMode::Base;
    if (value == "Nerve") return NandlDisplayMode::Nerve;
    if (value == "Fatigue") return NandlDisplayMode::Fatigue;
    if (value == "CPS") return NandlDisplayMode::CPS;
    if (value == "Nerve+Fatigue" || value == "N+F") return NandlDisplayMode::NerveFatigue;
    if (value == "Nerve+CPS" || value == "N+C") return NandlDisplayMode::NerveCPS;
    if (value == "Fatigue+CPS" || value == "F+C") return NandlDisplayMode::FatigueCPS;
    if (value == "All 8 Models" || value == "All Models") return NandlDisplayMode::AllModels;
    if (value == "Custom Toggles" || value == "Custom") return NandlDisplayMode::Custom;
    return NandlDisplayMode::NFC;
}

char const* soundAssetFilename(SoundStyle style) {
    switch (style) {
        case SoundStyle::SoftChime: return "ding_chime.wav";
        case SoundStyle::CrystalPing: return "ding_ping.wav";
        case SoundStyle::RetroBlip: return "ding_retro.wav";
        case SoundStyle::ClassicBell:
        case SoundStyle::CustomFile:
        default:
            return "ding_bell.wav";
    }
}

std::vector<WindowBand> bandsForMax(int maxFrames, bool oneAtBottom) {
    std::vector<WindowBand> bands;
    if (maxFrames <= 10) {
        bands = {
            {9, 10, "9-10"},
            {7, 8, "7-8"},
            {5, 6, "5-6"},
            {4, 4, "4"},
            {3, 3, "3"},
            {2, 2, "2"},
            {1, 1, "1"},
        };
    } else if (maxFrames <= 12) {
        bands = {
            {9, 12, "9-12"},
            {7, 8, "7-8"},
            {5, 6, "5-6"},
            {4, 4, "4"},
            {3, 3, "3"},
            {2, 2, "2"},
            {1, 1, "1"},
        };
    } else if (maxFrames <= 17) {
        bands = {
            {17, 17, "17"},
            {13, 16, "13-16"},
            {9, 12, "9-12"},
            {7, 8, "7-8"},
            {5, 6, "5-6"},
            {4, 4, "4"},
            {3, 3, "3"},
            {2, 2, "2"},
            {1, 1, "1"},
        };
    } else if (maxFrames <= 30) {
        bands = {
            {21, 30, "21-30"},
            {13, 20, "13-20"},
            {9, 12, "9-12"},
            {5, 8, "5-8"},
            {4, 4, "4"},
            {3, 3, "3"},
            {2, 2, "2"},
            {1, 1, "1"},
        };
    } else if (maxFrames <= 60) {
        bands = {
            {41, 60, "41-60"},
            {25, 40, "25-40"},
            {13, 24, "13-24"},
            {7, 12, "7-12"},
            {4, 6, "4-6"},
            {3, 3, "3"},
            {2, 2, "2"},
            {1, 1, "1"},
        };
    } else if (maxFrames <= 120) {
        bands = {
            {81, 120, "81-120"},
            {41, 80, "41-80"},
            {21, 40, "21-40"},
            {9, 20, "9-20"},
            {4, 8, "4-8"},
            {3, 3, "3"},
            {2, 2, "2"},
            {1, 1, "1"},
        };
    } else {
        bands = {
            {161, 240, "161-240"},
            {81, 160, "81-160"},
            {41, 80, "41-80"},
            {17, 40, "17-40"},
            {5, 16, "5-16"},
            {3, 4, "3-4"},
            {2, 2, "2"},
            {1, 1, "1"},
        };
    }

    if (!oneAtBottom) {
        std::reverse(bands.begin(), bands.end());
    }
    return bands;
}

RGBColor colorForWindow(std::uint64_t frames, ModSettingsSnapshot const& cfg) {
    if (frames == 0) {
        return {140, 140, 140};
    }

    switch (cfg.colorPreset) {
        case ColorPreset::Custom: {
            if (frames == 1) return cfg.custom1F;
            if (frames == 2) return cfg.custom2F;
            if (frames == 3) return cfg.custom3F;
            if (frames == 4) return cfg.custom4F;
            return cfg.custom5P;
        }
        case ColorPreset::NangdFire: {
            static constexpr std::array<RGBColor, 14> kFire = {{
                {140, 140, 140}, // 0
                {255, 38, 0},    // 1 - fiery crimson orange
                {255, 136, 0},   // 2 - vivid orange
                {255, 224, 24},  // 3 - gold yellow
                {255, 255, 180}, // 4 - warm ivory
                {128, 255, 64},  // 5 - lime
                {0, 245, 160},   // 6 - mint
                {0, 235, 255},   // 7 - aqua
                {40, 170, 255},  // 8 - sky
                {85, 115, 255},  // 9 - cobalt
                {145, 85, 255},  // 10 - violet
                {205, 85, 255},  // 11 - orchid
                {255, 110, 240}, // 12 - magenta
                {255, 165, 255}, // 13+ - bright pink
            }};
            return kFire[std::min<std::uint64_t>(frames, kFire.size() - 1)];
        }
        case ColorPreset::Pastel: {
            static constexpr std::array<RGBColor, 14> kPastel = {{
                {160, 160, 160},
                {255, 112, 112}, // 1
                {255, 176, 108}, // 2
                {255, 242, 128}, // 3
                {248, 250, 252}, // 4
                {138, 255, 160}, // 5
                {128, 255, 212}, // 6
                {138, 244, 255}, // 7
                {148, 208, 255}, // 8
                {166, 178, 255}, // 9
                {194, 158, 255}, // 10
                {226, 152, 255}, // 11
                {255, 158, 232}, // 12
                {255, 198, 245}, // 13+
            }};
            return kPastel[std::min<std::uint64_t>(frames, kPastel.size() - 1)];
        }
        case ColorPreset::Monochrome: {
            if (frames == 1) return {255, 255, 255};
            if (frames == 2) return {225, 225, 225};
            if (frames <= 4) return {195, 195, 195};
            if (frames <= 8) return {165, 165, 165};
            return {135, 135, 135};
        }
        case ColorPreset::NangdClassic:
        default: {
            static constexpr std::array<RGBColor, 20> kClassic = {{
                {0x80, 0x80, 0x80}, // 0
                {0xFF, 0x00, 0x00}, // 1 - red
                {0xFF, 0x7F, 0x00}, // 2 - orange
                {0xF6, 0xFF, 0x00}, // 3 - yellow
                {0xFF, 0xFF, 0xFF}, // 4 - white
                {0x00, 0xFF, 0x00}, // 5 - green
                {0x00, 0xFD, 0x81}, // 6
                {0x00, 0xFF, 0xF9}, // 7 - cyan
                {0x00, 0xC4, 0xFF}, // 8
                {0x08, 0x7B, 0xFF}, // 9
                {0x06, 0x3A, 0xFF}, // 10 - blue
                {0x0A, 0x00, 0xFF}, // 11
                {0x3F, 0x01, 0xFF}, // 12 - indigo
                {0x7F, 0x08, 0xFF}, // 13
                {0xBF, 0x10, 0xFF}, // 14
                {0xFF, 0x18, 0xFF}, // 15
                {0xFF, 0x1B, 0xFF}, // 16
                {0xFF, 0x3C, 0xFF}, // 17
                {0xFF, 0x5D, 0xFF}, // 18
                {0xFF, 0x7C, 0xFF}, // 19+
            }};
            return kClassic[std::min<std::uint64_t>(frames, kClassic.size() - 1)];
        }
    }
}

float pitchForWindow(std::uint64_t frames, ModSettingsSnapshot const& cfg) {
    const double lo = std::clamp(static_cast<double>(cfg.minPitch), 0.20, 2.50);
    const double hi = std::clamp(std::max(static_cast<double>(cfg.maxPitch), lo), 0.20, 3.00);
    if (frames <= 1) {
        return static_cast<float>(lo);
    }

    const int cappedMax = std::clamp(cfg.soundMaxFrames, 2, 60);
    const double refMax = static_cast<double>(cappedMax);
    const double f = static_cast<double>(frames);

    double pitch = lo;
    switch (cfg.pitchCurve) {
        case PitchCurve::Linear: {
            const double u = std::clamp((f - 1.0) / (refMax - 1.0), 0.0, 1.0);
            pitch = lo + u * (hi - lo);
            break;
        }
        case PitchCurve::Semitones: {
            const double u = std::clamp((f - 1.0) / (refMax - 1.0), 0.0, 1.0);
            pitch = lo * std::pow(hi / lo, u);
            break;
        }
        case PitchCurve::Musical:
        default: {
            const double u = std::clamp(std::log(f) / std::log(refMax), 0.0, 1.0);
            pitch = lo * std::pow(hi / lo, u);
            break;
        }
    }

    return static_cast<float>(std::clamp(pitch, lo, hi));
}

double pointsForWindow(std::uint64_t frames, ModSettingsSnapshot const& cfg) {
    if (frames == 0) return 0.0;
    if (cfg.maxWindowFrames > 0 && frames > static_cast<std::uint64_t>(cfg.maxWindowFrames)) {
        return 0.0;
    }
    const double n = static_cast<double>(frames);
    switch (cfg.pointsFormula) {
        case PointsFormula::FP60Equivalent:
            return 4.0 / n;
        case PointsFormula::FP120Equivalent:
            return 2.0 / n;
        case PointsFormula::Squared240:
            return 1.0 / (n * n);
        case PointsFormula::Align60Hz:
            if (frames <= 4) return 1.0;
            if (frames < 8) return (8.0 - n) / 4.0;
            return 0.0;
        case PointsFormula::SubFrameHard:
            // Extra harsh curve for sub-frame hunting: 1F=1.00, 2F=0.125, 3F=0.037
            return 1.0 / std::pow(n, 3.0);
        case PointsFormula::FP240Inverse:
        default:
            return 1.0 / n;
    }
}

RGBColor colorForWindow(std::uint64_t frames, double phase, ModSettingsSnapshot const& cfg) {
    RGBColor base = colorForWindow(frames, cfg);
    if (frames == 0) return base;

    const double p = std::clamp(std::isfinite(phase) ? phase : 0.0, 0.0, 1.0);
    // Blend toward the "worse" neighbour so 1.9F looks closer to 2F than 1.0F does.
    // Weights are intentionally subtle: the tick bucket still dominates the hue.
    const double shade = 1.0 - 0.28 * p;
    auto scale = [&](std::uint8_t c) {
        const double v = static_cast<double>(c) * shade;
        return static_cast<std::uint8_t>(std::clamp(v, 0.0, 255.0));
    };
    return {scale(base.r), scale(base.g), scale(base.b)};
}

RGBColor subFrameRampColor(double t, ModSettingsSnapshot const& cfg) {
    const double u = std::clamp(std::isfinite(t) ? t : 0.0, 0.0, 1.0);
    struct Stop { double pos; RGBColor col; };
    // Consistent with the NangdClassic 1F->5+ ramp so the heat map reads the same.
    static constexpr Stop kStops[] = {
        {0.00, {255, 0, 0}},
        {0.20, {255, 127, 0}},
        {0.40, {246, 255, 0}},
        {0.60, {0, 255, 0}},
        {0.80, {0, 235, 255}},
        {1.00, {145, 85, 255}},
    };

    if (cfg.colorPreset == ColorPreset::Monochrome) {
        const auto v = static_cast<std::uint8_t>(std::clamp(255.0 * (1.0 - u) + 120.0 * u, 0.0, 255.0));
        return {v, v, v};
    }

    for (std::size_t i = 1; i < std::size(kStops); ++i) {
        if (u > kStops[i].pos) continue;
        const auto& a = kStops[i - 1];
        const auto& b = kStops[i];
        const double span = std::max(1e-9, b.pos - a.pos);
        const double f = std::clamp((u - a.pos) / span, 0.0, 1.0);
        auto mix = [&](std::uint8_t ca, std::uint8_t cb) {
            return static_cast<std::uint8_t>(
                std::clamp(static_cast<double>(ca) + (static_cast<double>(cb) - ca) * f, 0.0, 255.0)
            );
        };
        return {mix(a.col.r, b.col.r), mix(a.col.g, b.col.g), mix(a.col.b, b.col.b)};
    }
    return kStops[std::size(kStops) - 1].col;
}

double framesToMillis(double frames) {
    if (!std::isfinite(frames)) return 0.0;
    return frames * math::kTickMillis;
}

std::string formatWindowLabel(
    std::uint64_t frames,
    double phase,
    bool overLimit,
    ModSettingsSnapshot const& cfg
) {
    if (overLimit) {
        return ">" + std::to_string(std::max(0, cfg.maxWindowFrames)) + "F";
    }
    const double p = std::clamp(std::isfinite(phase) ? phase : 0.0, 0.0, 0.9999);
    const int dec = std::clamp(cfg.subFrameDecimals, 0, 3);

    switch (cfg.subFrameMode) {
        case SubFrameMode::WholeFrames:
            return std::to_string(frames);
        case SubFrameMode::DecimalTick: {
            const double whole = static_cast<double>(frames);
            const double sub = p;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.0f+%.*f", whole, std::max(1, dec), sub);
            return std::string(buf);
        }
        case SubFrameMode::DecimalFrame:
        default:
            return math::formatFrames(frames, p, false, cfg.maxWindowFrames, dec);
    }
}

double windowScore(std::uint64_t frames, double phase, ModSettingsSnapshot const& cfg) {
    if (frames == 0) return 0.0;
    const double jitter = std::clamp(static_cast<double>(cfg.subFrameJitterFrames), 0.25, 8.0);
    return math::subFramePrecisionScore(frames, phase, jitter);
}

std::string formatMarkerText(
    std::uint64_t frames,
    double phase,
    bool isRelease,
    bool overLimit,
    int maxWindow,
    bool distinguishRelease,
    ModSettingsSnapshot const& cfg
) {
    std::string body;
    if (overLimit) {
        body = ">" + std::to_string(maxWindow);
    } else if (frames == 0) {
        body = "X";
    } else {
        body = formatWindowLabel(frames, phase, false, cfg);
    }
    if (isRelease && distinguishRelease) {
        return "R" + body;
    }
    return body;
}

double pointsForWindow(std::uint64_t frames, double phase, ModSettingsSnapshot const& cfg) {
    const double base = pointsForWindow(frames, cfg);
    if (!cfg.subFrameScoring || base <= 0.0) return base;
    const double hardness = std::clamp(static_cast<double>(cfg.subFrameJitterFrames), 0.25, 8.0);
    return math::subFramePoints(base, frames, phase, hardness);
}

std::string formatMarkerText(
    std::uint64_t frames,
    bool isRelease,
    bool overLimit,
    int maxWindow,
    bool distinguishRelease
) {
    std::string body;
    if (overLimit) {
        body = ">" + std::to_string(maxWindow);
    } else if (frames == 0) {
        body = "X";
    } else {
        body = std::to_string(frames);
    }

    if (isRelease && distinguishRelease) {
        return "R" + body;
    }
    return body;
}

}
