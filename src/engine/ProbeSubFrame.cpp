#include "ProbeController.hpp"
#include "ProbeReport.hpp"
#include "SubFrameGeometry.hpp"
#include "../ui/Toast.hpp"

#include <Geode/ui/NineSlice.hpp>
#include <fmt/format.h>
#include <algorithm>
#include <cmath>

using namespace geode::prelude;
using namespace cocos2d;

namespace na_frame_counter::engine {

namespace {

bool hasChild(CCNode* parent, CCNode* child) {
    if (!parent || !child) return false;
    auto* children = parent->getChildren();
    return children && children->containsObject(child);
}

ccColor3B toCCColor(core::RGBColor c) {
    return ccc3(c.r, c.g, c.b);
}

core::RGBColor rampColor(double t, core::ModSettingsSnapshot const& cfg) {
    return core::subFrameRampColor(t, cfg);
}

std::string formatSigned(double value, int decimals) {
    if (value <= 0.0001) return fmt::format("{:.{}f}", 0.0, decimals);
    return fmt::format("+{:.{}f}", value, decimals);
}

} // namespace

namespace {

// Fraction of a tick the player's hitbox already crossed into (or out of) the
// target object, expressed as 0..0.999. Used to interpolate the failing tick.
double penetrationFraction(GameObject* target, CCPoint const& atPos) {
    if (!target) return 0.5;
    const auto rect = target->getObjectRect();
    const double width = static_cast<double>(rect.size.width);
    if (width <= 1e-6) return 0.5;

    const double relative = static_cast<double>(atPos.x - rect.getMinX());
    const double normalised = relative / width;
    if (!std::isfinite(normalised)) return 0.5;
    return std::clamp(normalised, 0.0, 0.999);
}

} // namespace

double ProbeController::pressPhaseFor(PlayLayer* layer, ClickJob const& job) const {
    if (!layer) return 0.5;
    if (job.pressPhaseKnown) {
        return std::clamp(static_cast<double>(job.pressPhase), 0.0, 0.999);
    }
    if (job.activationRing) {
        return penetrationFraction(static_cast<GameObject*>(job.activationRing), job.activationPos);
    }
    return 0.5;
}

double ProbeController::releasePhaseFor(PlayLayer* layer, ClickJob const& job) const {
    if (!layer) return 0.5;
    if (job.releasePhaseKnown) {
        return std::clamp(static_cast<double>(job.releasePhase), 0.0, 0.999);
    }
    if (job.activationRing) {
        return penetrationFraction(static_cast<GameObject*>(job.activationRing), job.releasePos);
    }
    return 0.5;
}

void ProbeController::clearSubFrameHUD() {
    auto* sentinel = m_subFrameSentinel;
    m_subFrameSentinel = nullptr;
    if (sentinel) sentinel->disarm();

    auto* hud = m_subFrameHud;
    m_subFrameHud = nullptr;
    m_subFrameMainLabel = nullptr;
    m_subFrameSubLabel = nullptr;
    m_subFrameCbfLabel = nullptr;
    m_subFrameGauge = nullptr;
    m_subFrameRamp = nullptr;
    m_subFrameAccent = nullptr;

    if (hud && hud->getParent()) {
        hud->removeFromParentAndCleanup(true);
    }
}

void ProbeController::buildSubFrameHUD() {
    clearSubFrameHUD();
    if (!m_level || !m_cfg.enabled || !m_cfg.subFramePrecisionHud || m_platformer) return;

    constexpr float kCardW = 196.f;
    constexpr float kCardH = 52.f;

    auto* root = CCNode::create();
    if (!root) return;

    const auto winSize = CCDirector::sharedDirector()->getWinSize();

    // Sits bottom-centre, above GD's progress bar, so it never overlaps the
    // tally HUD / points HUD in either corner.
    float baseY = 64.f;
    if (m_cfg.showCounters && m_cfg.hudCorner == core::HudCorner::BottomRight) baseY = 78.f;

    root->setPosition({winSize.width * 0.5f, baseY});
    root->setID("subframe-hud"_spr);
    m_subFrameSentinel = CleanupSentinel::create(SentinelKind::SubFrameHud, root);
    if (m_subFrameSentinel) root->addChild(m_subFrameSentinel, -999999);
    m_level->addChild(root, 1000003);
    m_subFrameHud = root;

    if (auto* card = NineSlice::create("square02b_001.png")) {
        card->setContentSize({kCardW, kCardH});
        card->setPosition({0.f, 0.f});
        card->setColor({10, 14, 24});
        card->setOpacity(150);
        root->addChild(card, 0);
    }

    // Left accent bar: recoloured on every update to reflect the current
    // verdict (perfect / good / over-limit / CBF), so the card's state is
    // readable at a glance even before the labels are parsed.
    if (auto* accent = CCLayerColor::create({255, 255, 255, 255})) {
        accent->setContentSize({3.f, kCardH - 8.f});
        accent->setAnchorPoint({0.f, 0.5f});
        accent->setPosition({-kCardW * 0.5f + 1.f, 0.f});
        accent->setOpacity(235);
        root->addChild(accent, 4);
        m_subFrameAccent = accent;
    }

    // Thin gauge showing where inside the tick the last input landed.
    m_subFrameGauge = CCDrawNode::create();
    if (m_subFrameGauge) {
        root->addChild(m_subFrameGauge, 3);
    }

    // Heat map strip: last 24 inputs, best -> worst left to right.
    m_subFrameRamp = CCDrawNode::create();
    if (m_subFrameRamp) {
        root->addChild(m_subFrameRamp, 2);
    }

    if (auto* main = CCLabelBMFont::create("0.0F", "bigFont.fnt")) {
        main->setAnchorPoint({0.f, 0.5f});
        main->setPosition({-kCardW * 0.5f + 12.f, 10.f});
        main->setScale(0.52f);
        main->setColor({255, 245, 190});
        root->addChild(main, 4);
        m_subFrameMainLabel = main;
    }

    if (auto* sub = CCLabelBMFont::create("sub-frame", "chatFont.fnt")) {
        sub->setAnchorPoint({0.f, 0.5f});
        sub->setPosition({-kCardW * 0.5f + 12.f, -8.f});
        sub->setScale(0.62f);
        sub->setColor({170, 190, 215});
        root->addChild(sub, 4);
        m_subFrameSubLabel = sub;
    }

    if (auto* cbf = CCLabelBMFont::create("OK", "bigFont.fnt")) {
        cbf->setAnchorPoint({1.f, 0.5f});
        cbf->setPosition({kCardW * 0.5f - 12.f, 10.f});
        cbf->setScale(0.38f);
        cbf->setColor({150, 230, 255});
        root->addChild(cbf, 4);
        m_subFrameCbfLabel = cbf;
    }

    updateSubFrameHUD();
}

void ProbeController::resetSubFrameHUD() {
    m_subFrameSamples.clear();
    m_cbfSamples.clear();
    m_stats.lastPhase = 0.0;
    m_stats.lastScore = 0.0;
    m_stats.avgScore = 0.0;
    m_stats.phaseSum = 0.0;
    m_stats.subFrameCount = 0;
    m_stats.subFramePerfects = 0;
    m_stats.cbfCount = 0;
    m_stats.cbfRequiredCount = 0;
    m_stats.lastWasCbf = false;
    m_stats.lastCbfRequired = false;
    m_stats.lastMillisMicro = 0;
    m_stats.streak = 0;
    updateSubFrameHUD();
}

void ProbeController::recordSubFrameSample(
    std::uint64_t frames,
    double phase,
    double points,
    bool isRelease,
    bool overLimit,
    math::CbfAnalysis const& cbf
) {
    SubFrameSample sample;
    sample.frames = frames;
    sample.phase = std::clamp(phase, 0.0, 0.999);
    sample.isRelease = isRelease;
    sample.overLimit = overLimit;
    sample.cbf = (cbf.verdict == math::CbfVerdict::Cbf || cbf.verdict == math::CbfVerdict::CbfRequired);
    sample.cbfRequired = cbf.required;
    sample.score = overLimit ? 0.0 : core::windowScore(frames, sample.phase, m_cfg);
    sample.points = points;
    sample.frame = m_clockReady && m_level ? rawFrame(m_level) : 0;

    m_subFrameSamples.push_back(sample);
    while (m_subFrameSamples.size() > 256) {
        m_subFrameSamples.pop_front();
    }

    m_stats.lastPhase = sample.phase;
    m_stats.lastScore = sample.score;
    m_stats.lastMillisMicro = static_cast<std::uint64_t>(
        std::llround(core::framesToMillis(static_cast<double>(frames) + sample.phase) * 1000.0)
    );

    if (!overLimit && frames > 0) {
        ++m_stats.subFrameCount;
        m_stats.phaseSum += sample.phase;
        if (frames == 1 && sample.phase <= 0.25) ++m_stats.subFramePerfects;
        if (sample.score > m_stats.bestScore) m_stats.bestScore = sample.score;
        if (m_stats.subFrameCount > 0) {
            m_stats.avgScore = (m_stats.avgScore * (m_stats.subFrameCount - 1) + sample.score)
                / static_cast<double>(m_stats.subFrameCount);
        }
        if (frames == 1) {
            ++m_stats.streak;
            if (m_stats.streak > m_stats.bestStreak) m_stats.bestStreak = m_stats.streak;
        } else {
            m_stats.streak = 0;
        }
    }

    updateSubFrameHUD();
}

void ProbeController::maybeToastMeasurement(
    std::uint64_t frames,
    double phase,
    bool isRelease,
    bool overLimit,
    math::CbfAnalysis const& cbf
) {
    if (!m_cfg.subFrameToast || m_platformer) return;

    const auto decimals = std::clamp(m_cfg.subFrameDecimals, 0, 3);
    ui::ToastTone tone = ui::ToastTone::Neutral;
    if (overLimit) tone = ui::ToastTone::Warn;
    else if (frames == 1 && phase <= 0.25) tone = ui::ToastTone::Great;
    else if (frames == 1) tone = ui::ToastTone::Good;
    else if (frames <= 3) tone = ui::ToastTone::Info;
    else tone = ui::ToastTone::Neutral;

    if (cbf.verdict == math::CbfVerdict::CbfRequired) tone = ui::ToastTone::Bad;
    else if (cbf.verdict == math::CbfVerdict::Cbf && tone != ui::ToastTone::Great) {
        tone = ui::ToastTone::Warn;
    }

    const auto tag = core::formatWindowLabel(frames, phase, overLimit, m_cfg);
    const auto ms = core::framesToMillis(static_cast<double>(frames) + phase);

    std::string detail;
    const auto cbfMode = m_cfg.cbfBadge;
    if (cbfMode != core::CbfBadgeMode::Off) {
        if (cbf.verdict == math::CbfVerdict::CbfRequired) {
            detail = fmt::format("{} - needs CBF ({:.2f}F off-frame)", ms > 0.0 ? fmt::format("{:.2f} ms", ms) : "--", cbf.offsetFrames);
        } else if (cbf.verdict == math::CbfVerdict::Cbf) {
            detail = (cbfMode == core::CbfBadgeMode::Verbose)
                ? fmt::format("{} - CBF {:.2f}F {} of boundary", fmt::format("{:.2f} ms", ms), cbf.offsetFrames,
                              cbf.signedOffsetFrames < 0.0 ? "early" : "late")
                : fmt::format("{} ms - CBF in play", ms);
        } else if (cbfMode == core::CbfBadgeMode::Verbose) {
            detail = fmt::format("{:.2f} ms  {}  score {:.0f}", ms,
                                 isRelease ? "release" : "click", m_stats.lastScore);
        }
    }
    if (detail.empty()) {
        detail = fmt::format("{:.2f} ms  score {:.0f}/100", ms, m_stats.lastScore);
    }

    auto request = ui::makeMeasurementToast(
        frames, phase, isRelease, overLimit,
        tag,
        tone,
        detail
    );
    if (decimals > 1 && !overLimit) {
        request.body = fmt::format("{:.{}f}F", static_cast<double>(frames) + phase, decimals);
    }
    request.duration = std::clamp(m_cfg.toastDuration, 0.6f, 5.0f);
    request.style = static_cast<ui::ToastStyle>(std::clamp(m_cfg.toastStyle, 0, 2));
    ui::Toast::enqueue(std::move(request));

    // Separate, quieter CBF toast only when CBF is the headline news.
    if (cbf.verdict == math::CbfVerdict::CbfRequired) {
        auto cbfToast = ui::makeCbfToast(
            cbf.label,
            math::cbfDescription(cbf),
            ui::ToastTone::Bad
        );
        cbfToast.duration = std::clamp(m_cfg.toastDuration + 0.4f, 0.6f, 5.0f);
        cbfToast.style = ui::ToastStyle::Pop;
        ui::Toast::enqueue(std::move(cbfToast));
    }

    // Milestone toasts (streaks) are once-per-run and skip the rest.
    if (m_stats.streak >= 5 && (m_stats.streak % 5) == 0 && !m_lastMilestoneStreak) {
        m_lastMilestoneStreak = true;
        ui::Toast::enqueue(ui::makeMilestoneToast(
            "Frame Perfect Streak",
            fmt::format("{} in a row", m_stats.streak),
            fmt::format("{}x", m_stats.streak),
            ui::ToastTone::Great
        ));
    } else if (m_stats.streak % 5 != 0) {
        m_lastMilestoneStreak = false;
    }
}

void ProbeController::updateSubFrameHUD() {
    if (!m_level || !m_subFrameHud || m_subFrameHud->getParent() != m_level) return;
    if (!m_cfg.subFramePrecisionHud) return;

    constexpr float kCardW = 196.f;
    constexpr float kCardH = 52.f;
    const auto decimals = std::clamp(m_cfg.subFrameDecimals, 0, 3);

    const double lastPhase = std::clamp(m_stats.lastPhase, 0.0, 0.999);
    const auto lastFrames = m_subFrameSamples.empty() ? 0 : m_subFrameSamples.back().frames;
    const bool overLimit = m_subFrameSamples.empty() ? false : m_subFrameSamples.back().overLimit;
    const bool isRelease = m_subFrameSamples.empty() ? false : m_subFrameSamples.back().isRelease;

    const auto display = core::formatWindowLabel(lastFrames, lastPhase, overLimit, m_cfg);
    const auto color = (lastFrames == 0)
        ? core::RGBColor{180, 190, 205}
        : rampColor(1.0 - std::clamp(m_stats.lastScore / 100.0, 0.0, 1.0), m_cfg);

    if (hasChild(m_subFrameHud, m_subFrameMainLabel)) {
        m_subFrameMainLabel->setString(fmt::format("{}F", display).c_str());
        m_subFrameMainLabel->limitLabelWidth(104.f, 0.52f, 0.24f);
        m_subFrameMainLabel->setColor(toCCColor(color));
    } else {
        m_subFrameMainLabel = nullptr;
    }

    if (hasChild(m_subFrameHud, m_subFrameSubLabel)) {
        std::string sub;
        if (m_stats.subFrameCount == 0) {
            sub = "waiting for first input";
        } else {
            sub = fmt::format(
                "{} | avg {:.0f} | best {:.0f} | {} ms",
                isRelease ? "release" : "click",
                m_stats.avgScore,
                m_stats.bestScore,
                fmt::format("{:.2f}", static_cast<double>(m_stats.lastMillisMicro) / 1000.0)
            );
        }
        m_subFrameSubLabel->setString(sub.c_str());
        m_subFrameSubLabel->limitLabelWidth(kCardW - 90.f, 0.62f, 0.34f);
    } else {
        m_subFrameSubLabel = nullptr;
    }

    if (hasChild(m_subFrameHud, m_subFrameCbfLabel)) {
        math::CbfConfig cbfCfg;
        cbfCfg.enabled = m_cfg.cbfBadge != core::CbfBadgeMode::Off;
        cbfCfg.guardFrames = std::clamp(static_cast<double>(m_cfg.cbfGuardFrames), 0.0, 0.5);
        cbfCfg.requireFrames = std::max(cbfCfg.guardFrames, 0.25);
        cbfCfg.includeReleases = m_cfg.cbfIncludeReleases;

        const auto attempt = math::analyzeAttempt(m_cbfSamples, cbfCfg);
        std::string text;
        if (m_cfg.cbfBadge == core::CbfBadgeMode::Off) {
            text = "CBF off";
        } else if (m_cfg.cbfBadge == core::CbfBadgeMode::Verbose) {
            text = fmt::format("{} {}", math::cbfTag(attempt.verdict), m_stats.cbfCount);
        } else {
            text = math::cbfTag(attempt.verdict);
        }
        m_subFrameCbfLabel->setString(text.c_str());
        m_subFrameCbfLabel->limitLabelWidth(84.f, 0.38f, 0.20f);

        ccColor3B tagColor{150, 230, 255};
        switch (attempt.verdict) {
            case math::CbfVerdict::CbfRequired: tagColor = {255, 120, 120}; break;
            case math::CbfVerdict::Cbf: tagColor = {255, 186, 96}; break;
            case math::CbfVerdict::Boundary: tagColor = {150, 255, 170}; break;
            case math::CbfVerdict::NearBoundary: tagColor = {210, 230, 255}; break;
            case math::CbfVerdict::Disabled: tagColor = {150, 150, 160}; break;
            default: tagColor = {150, 230, 255}; break;
        }
        m_subFrameCbfLabel->setColor(tagColor);
    } else {
        m_subFrameCbfLabel = nullptr;
    }

    // Gauge: the tick, the input position, and the "safe" band.
    if (hasChild(m_subFrameHud, m_subFrameGauge)) {
        m_subFrameGauge->clear();
        constexpr float kGaugeW = 96.f;
        constexpr float kGaugeH = 5.f;
        const float x0 = -kCardW * 0.5f + 12.f;
        const float y0 = -22.f;

        const ccColor4F bg{0.16f, 0.19f, 0.26f, 0.85f};
        m_subFrameGauge->drawRect(
            CCPoint{x0, y0}, CCPoint{x0 + kGaugeW, y0 + kGaugeH}, bg
        );

        const auto tickCol = toCCColor(color);
        const ccColor4F fill{tickCol.r / 255.f, tickCol.g / 255.f, tickCol.b / 255.f, 1.f};
        const float markerX = x0 + static_cast<float>(lastPhase) * kGaugeW;
        m_subFrameGauge->drawRect(
            CCPoint{markerX - 1.2f, y0 - 2.f},
            CCPoint{markerX + 1.2f, y0 + kGaugeH + 2.f},
            fill
        );
        m_subFrameGauge->drawRect(
            CCPoint{x0, y0}, CCPoint{markerX, y0 + kGaugeH}, fill
        );

        // Quarter-tick notches so "0.25/0.5/0.75" is readable at a glance.
        const ccColor4F notch{1.f, 1.f, 1.f, 0.22f};
        for (int i = 1; i < 4; ++i) {
            const float nx = x0 + kGaugeW * (static_cast<float>(i) * 0.25f);
            m_subFrameGauge->drawRect(
                CCPoint{nx - 0.5f, y0 - 1.5f},
                CCPoint{nx + 0.5f, y0 + kGaugeH + 1.5f},
                notch
            );
        }
    } else {
        m_subFrameGauge = nullptr;
    }

    // Heat map ramp: last 24 measurements.
    if (hasChild(m_subFrameHud, m_subFrameRamp)) {
        m_subFrameRamp->clear();
        constexpr float kRampW = 96.f;
        constexpr float kCellH = 4.f;
        const float x0 = -kCardW * 0.5f + 12.f;
        const float y0 = -34.f;
        const int maxCells = 24;
        const int count = static_cast<int>(m_subFrameSamples.size());

        for (int i = 0; i < maxCells; ++i) {
            const int idx = count - maxCells + i;
            if (idx < 0) continue;
            const auto& sample = m_subFrameSamples[static_cast<std::size_t>(idx)];
            const double t = sample.overLimit ? 1.0
                : 1.0 - std::clamp(sample.score / 100.0, 0.0, 1.0);
            const auto c = rampColor(t, m_cfg);
            const ccColor4F col{c.r / 255.f, c.g / 255.f, c.b / 255.f, 0.95f};
            const float cx = x0 + static_cast<float>(i) * (kRampW / maxCells) + 0.6f;
            m_subFrameRamp->drawRect(
                CCPoint{cx, y0},
                CCPoint{cx + (kRampW / maxCells) - 1.2f, y0 + kCellH},
                col
            );
        }
    } else {
        m_subFrameRamp = nullptr;
    }

    if (hasChild(m_subFrameHud, m_subFrameAccent)) {
        m_subFrameAccent->setColor(toCCColor(color));
    }
}

void ProbeController::pulseSubFrameGauge(
    std::uint64_t windowFrames,
    double phase,
    bool isRelease
) {
    if (!m_level || !m_subFrameHud || m_subFrameHud->getParent() != m_level) return;
    if (!m_cfg.subFramePrecisionHud) return;

    const bool excellent = (windowFrames == 1 && phase <= 0.25);
    if (hasChild(m_subFrameHud, m_subFrameMainLabel)) {
        m_subFrameMainLabel->stopAllActions();
        const float target = 0.52f;
        m_subFrameMainLabel->setScale(target);
        if (excellent) {
            m_subFrameMainLabel->setScale(target * 1.28f);
            m_subFrameMainLabel->runAction(CCEaseBackOut::create(
                CCScaleTo::create(0.26f, target)
            ));
        }
    }

    if (!m_cfg.subFrameToast && !excellent) return;

    // Small "ping" burst on the card edge for perfect placements.
    if (excellent) {
        auto* glow = NineSlice::create("square02b_001.png");
        if (glow) {
            glow->setContentSize({196.f, 52.f});
            glow->setColor({255, 236, 150});
            glow->setOpacity(70);
            glow->setScale(1.0f);
            m_subFrameHud->addChild(glow, 1);
            glow->runAction(CCSequence::createWithTwoActions(
                CCSpawn::createWithTwoActions(
                    CCEaseSineOut::create(CCScaleTo::create(0.34f, 1.16f)),
                    CCFadeOut::create(0.34f)
                ),
                CCRemoveSelf::create(true)
            ));
        }
        CC_UNUSED_PARAM(isRelease);
    }
}

void ProbeController::showSessionReportToast() {
    if (!m_cfg.subFrameToast || m_platformer) return;
    if (m_stats.totalMeasured == 0) return;

    std::vector<SubFrameSample> samples(m_subFrameSamples.begin(), m_subFrameSamples.end());
    const auto report = buildSessionReport(m_stats, samples, m_cfg);
    const auto summary = summarizeSession(m_stats, m_cfg);

    ui::Toast::enqueue(ui::makeMilestoneToast(
        "Session Complete",
        summary,
        fmt::format("{:.1f}", m_stats.totalPoints),
        ui::ToastTone::Great
    ));

    // Keep the full body available for the clipboard action in the mod menu.
    m_lastReport = report.body;
    m_lastReportCsv = report.csv;
}

void ProbeController::copySessionReportToClipboard() {
    std::vector<SubFrameSample> samples(m_subFrameSamples.begin(), m_subFrameSamples.end());
    const auto report = buildSessionReport(m_stats, samples, m_cfg);
    const bool ok = geode::utils::clipboard::write(report.body);
    ui::Toast::enqueue(ui::makeMilestoneToast(
        ok ? "Report Copied" : "Clipboard Failed",
        ok ? summarizeSession(m_stats, m_cfg) : "Could not write to the clipboard",
        ok ? "COPY" : "ERR",
        ok ? ui::ToastTone::Info : ui::ToastTone::Bad
    ));
}

} // namespace na_frame_counter::engine
