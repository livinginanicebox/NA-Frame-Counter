#pragma once

#include <Geode/Geode.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace na_frame_counter::ui {

// ---------------------------------------------------------------------------
// Toast
//
// A lightweight, fully-animated in-game notification card. It rises in from
// below the HUD on an ease-out curve, overshoots slightly with an
// ease-back-out, holds, then fades/slides away. Toasts are queued so a burst of
// measurements never fights over the same screen space.
//
// Draw calls are intentionally tiny (one NineSlice + labels) so this is safe to
// fire from inside PlayLayer::postUpdate.
// ---------------------------------------------------------------------------

enum class ToastTone : int {
    Neutral = 0,
    Good = 1,
    Great = 2,
    Warn = 3,
    Bad = 4,
    Info = 5,
};

enum class ToastStyle : int {
    Slide = 0,   // rise + fade (default)
    Pop = 1,     // scale bounce in place
    SlidePop = 2, // rise + bounce
};

struct ToastRequest {
    std::string title;
    std::string body;
    std::string badge;   // short right-aligned tag, e.g. "CBF!", "1.0F"
    ToastTone tone = ToastTone::Neutral;
    ToastStyle style = ToastStyle::SlidePop;
    float duration = 1.9f;
    // Optional second line (small, dimmer). Empty hides it.
    std::string detail;
};

class Toast : public cocos2d::CCNodeRGBA {
public:
    static Toast* create(ToastRequest const& request);

    // Shows on the running scene. Safe to call from gameplay code.
    void show(cocos2d::CCNode* parent = nullptr);
    bool isAlive() const { return m_alive; }
    void dismiss();

    ToastRequest const& request() const { return m_request; }

    // Queue helper: pushes to the global FIFO, shown one at a time.
    static void enqueue(ToastRequest request);

    // Drops the queue and any visible toast (level exit / reset).
    static void clearAll();

    // Pumped once per frame from PlayLayer::postUpdate so timings stay in sync
    // with the game clock rather than relying on scheduled selectors.
    static void pump(float dt);

    static bool animEnabled;

private:
    Toast() = default;
    ~Toast() override;

    void buildChildren();
    void runInAnimation();

    ToastRequest m_request;
    cocos2d::CCNodeRGBA* m_root = nullptr;
    cocos2d::CCNodeRGBA* m_card = nullptr;
    cocos2d::CCLabelBMFont* m_titleLabel = nullptr;
    cocos2d::CCLabelBMFont* m_bodyLabel = nullptr;
    cocos2d::CCLabelBMFont* m_detailLabel = nullptr;
    cocos2d::CCLabelBMFont* m_badgeLabel = nullptr;
    cocos2d::CCLayerColor* m_accent = nullptr;

    bool m_alive = true;
    bool m_leaving = false;
    float m_elapsed = 0.f;
    float m_holdTime = 0.f;
    float m_baseScale = 1.f;
};

// Convenience builders used by the measurement pipeline.
ToastRequest makeMeasurementToast(
    std::uint64_t frames,
    double phase,
    bool isRelease,
    bool overLimit,
    std::string const& tag,
    ToastTone tone,
    std::string const& detail
);

ToastRequest makeCbfToast(std::string const& tag, std::string const& detail, ToastTone tone);

ToastRequest makeMilestoneToast(
    std::string const& title,
    std::string const& body,
    std::string const& badge,
    ToastTone tone
);

}
