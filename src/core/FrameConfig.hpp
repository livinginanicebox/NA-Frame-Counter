#pragma once

#include "../math/NandlCalculator.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace na_frame_counter::core {

struct RGBColor {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;

    bool operator==(RGBColor const& other) const = default;
};

struct WindowBand {
    std::int64_t minFrames = 0;
    std::int64_t maxFrames = 0;
    std::string label;
};

enum class LabelPlacement : int {
    Left = 0,
    Center = 1,
    Top = 2,
    Right = 3,
};

enum class ColorPreset : int {
    NangdClassic = 0,
    NangdFire = 1,
    Pastel = 2,
    Monochrome = 3,
    Custom = 4,
};

enum class SoundStyle : int {
    ClassicBell = 0,
    SoftChime = 1,
    CrystalPing = 2,
    RetroBlip = 3,
    CustomFile = 4,
};

enum class PitchCurve : int {
    Musical = 0,
    Linear = 1,
    Semitones = 2,
};

enum class HudCorner : int {
    TopLeft = 0,
    TopRight = 1,
    BottomRight = 2,
};

enum class PointsCorner : int {
    TopRight = 0,
    MiddleRight = 1,
    BottomRight = 2,
};

enum class PointsFormula : int {
    FP240Inverse = 0,
    FP60Equivalent = 1,
    FP120Equivalent = 2,
    Squared240 = 3,
    Align60Hz = 4,
    SubFrameHard = 5,
};

enum class SubFrameMode : int {
    WholeFrames = 0,
    DecimalFrame = 1,
    DecimalTick = 2,
};

enum class CbfBadgeMode : int {
    Off = 0,
    Compact = 1,
    Verbose = 2,
};

enum class NandlDisplayMode : int {
    NFC = 0,
    Base = 1,
    Nerve = 2,
    Fatigue = 3,
    CPS = 4,
    NerveFatigue = 5,
    NerveCPS = 6,
    FatigueCPS = 7,
    AllModels = 8,
    Custom = 9,
};

struct NandlVisibility {
    bool base = false;
    bool nerve = false;
    bool fatigue = false;
    bool cps = false;
    bool nerveFatigue = false;
    bool nerveCps = false;
    bool fatigueCps = false;
    bool nfc = true;
};

struct ModSettingsSnapshot {
    // Measurement & Engine
    bool enabled = true;
    bool measureClicks = true;
    bool measureReleases = true;
    int maxWindowFrames = 12;
    bool ignoreHarmlessClicks = true;
    bool showOverLimit = false;

    // Visuals & Circle Markers
    bool showRings = true;
    LabelPlacement labelPlacement = LabelPlacement::Left;
    ColorPreset colorPreset = ColorPreset::NangdClassic;
    RGBColor custom1F{255, 0, 0};
    RGBColor custom2F{255, 127, 0};
    RGBColor custom3F{246, 255, 0};
    RGBColor custom4F{255, 255, 255};
    RGBColor custom5P{0, 255, 0};
    float circleRadius = 10.0f;
    float circleThickness = 2.0f;
    float markerScale = 1.0f;
    float markerFadeSeconds = 0.0f;
    bool scaleWithZoom = false;
    bool distinguishRelease = true;

    // Sub-Frame Precision (fractional frame windows, 0.4F / 3.2F / 1.3F)
    SubFrameMode subFrameMode = SubFrameMode::DecimalFrame;
    int subFrameDecimals = 1;
    bool subFramePrecisionHud = true;
    bool subFrameToast = true;
    CbfBadgeMode cbfBadge = CbfBadgeMode::Compact;
    bool cbfIncludeReleases = true;
    float cbfGuardFrames = 0.10f;
    float subFrameJitterFrames = 1.0f;
    bool subFrameScoring = true;
    bool markerHeatmap = true;
    int toastStyle = 2;      // 0 slide, 1 pop, 2 slide+pop
    float toastDuration = 1.9f;

    // Audio & Pitch
    bool dingSounds = true;
    SoundStyle soundStyle = SoundStyle::ClassicBell;
    std::string customSoundFile;
    float soundVolume = 0.70f;
    bool syncGdSfx = false;
    float minPitch = 0.55f;
    float maxPitch = 1.65f;
    PitchCurve pitchCurve = PitchCurve::Musical;
    int soundMaxFrames = 12;

    // HUD & NaNDL
    bool showCounters = true;
    HudCorner hudCorner = HudCorner::TopLeft;
    bool oneAtBottom = true;
    bool showLastBadge = true;
    bool showPointsHud = true;
    PointsCorner pointsCorner = PointsCorner::TopRight;
    PointsFormula pointsFormula = PointsFormula::FP240Inverse;
    int pointsDecimals = 2;
    bool showPrecisionHud = true;
    NandlDisplayMode nandlDisplay = NandlDisplayMode::NFC;
    NandlVisibility nandlCustom;
    bool showPassProb = false;
    bool showExpectedTime = false;
    math::NandlParams nandlParams;
};

int parseMaxWindowPreset(std::string_view value);
LabelPlacement parseLabelPlacement(std::string_view value);
ColorPreset parseColorPreset(std::string_view value);
SoundStyle parseSoundStyle(std::string_view value);
PitchCurve parsePitchCurve(std::string_view value);
HudCorner parseHudCorner(std::string_view value);
PointsCorner parsePointsCorner(std::string_view value);
PointsFormula parsePointsFormula(std::string_view value);
SubFrameMode parseSubFrameMode(std::string_view value);
int parseToastStyle(std::string_view value);
CbfBadgeMode parseCbfBadgeMode(std::string_view value);
NandlDisplayMode parseNandlDisplayMode(std::string_view value);

char const* soundAssetFilename(SoundStyle style);

std::vector<WindowBand> bandsForMax(int maxFrames, bool oneAtBottom = true);
RGBColor colorForWindow(std::uint64_t frames, ModSettingsSnapshot const& cfg);
float pitchForWindow(std::uint64_t frames, ModSettingsSnapshot const& cfg);
double pointsForWindow(std::uint64_t frames, ModSettingsSnapshot const& cfg);
std::string formatMarkerText(std::uint64_t frames, bool isRelease, bool overLimit, int maxWindow, bool distinguishRelease);
std::string formatMarkerText(
    std::uint64_t frames,
    double phase,
    bool isRelease,
    bool overLimit,
    int maxWindow,
    bool distinguishRelease,
    ModSettingsSnapshot const& cfg
);

// Color lookup that additionally varies with the sub-frame phase, so a 1.0F
// input and a 1.9F input inside the same 2F bucket are visually distinct.
RGBColor colorForWindow(
    std::uint64_t frames,
    double phase,
    ModSettingsSnapshot const& cfg
);

// Continuous gradient for the sub-frame heat map ramp (0 = best, 1 = worst).
RGBColor subFrameRampColor(double t, ModSettingsSnapshot const& cfg);

// Human label for a measured window using the user's sub-frame display mode.
std::string formatWindowLabel(
    std::uint64_t frames,
    double phase,
    bool overLimit,
    ModSettingsSnapshot const& cfg
);

// Milliseconds for a fractional frame count (1F = 4.1667 ms).
double framesToMillis(double frames);

// 0..100 combined score for the precision HUD.
double windowScore(std::uint64_t frames, double phase, ModSettingsSnapshot const& cfg);

// Sub-frame aware points: the whole-frame formula, additionally scaled by the
// phase penalty when "Sub-Frame Scoring" is enabled.
double pointsForWindow(std::uint64_t frames, double phase, ModSettingsSnapshot const& cfg);

}
