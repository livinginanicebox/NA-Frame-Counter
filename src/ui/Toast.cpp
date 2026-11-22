#include "Toast.hpp"

#include <Geode/ui/NineSlice.hpp>
#include <fmt/format.h>
#include <algorithm>
#include <cmath>
#include <deque>
#include <utility>

using namespace geode::prelude;
using namespace cocos2d;

namespace na_frame_counter::ui {

namespace {

constexpr float kCardW = 250.f;
constexpr float kCardH = 46.f;
constexpr float kEdgeMargin = 10.f;
constexpr float kRiseDistance = 26.f;
constexpr int kMaxQueued = 6;

std::deque<ToastRequest> s_queue;
Toast* s_active = nullptr;

ccColor3B toneColor(ToastTone tone) {
    switch (tone) {
        case ToastTone::Good: return {255, 208, 96};
        case ToastTone::Great: return {255, 246, 140};
        case ToastTone::Warn: return {255, 168, 92};
        case ToastTone::Bad: return {255, 118, 118};
        case ToastTone::Info: return {140, 226, 255};
        case ToastTone::Neutral:
        default: return {226, 232, 244};
    }
}

void stopNodeActions(CCNode* node) {
    if (!node) return;
    node->stopAllActions();
}

} // namespace

bool Toast::animEnabled = true;

Toast* Toast::create(ToastRequest const& request) {
    auto* ret = new Toast();
    if (ret) {
        ret->m_request = request;
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

Toast::~Toast() {
    if (s_active == this) s_active = nullptr;
    m_root = nullptr;
    m_card = nullptr;
    m_titleLabel = nullptr;
    m_bodyLabel = nullptr;
    m_detailLabel = nullptr;
    m_badgeLabel = nullptr;
    m_accent = nullptr;
}

void Toast::enqueue(ToastRequest request) {
    if (request.title.empty() && request.body.empty()) return;
    if (s_queue.size() >= static_cast<std::size_t>(kMaxQueued)) {
        s_queue.pop_front();
    }
    s_queue.push_back(std::move(request));
}

void Toast::clearAll() {
    s_queue.clear();
    if (s_active) {
        s_active->dismiss();
    }
}

void Toast::pump(float dt) {
    if (!s_active && !s_queue.empty()) {
        ToastRequest next = std::move(s_queue.front());
        s_queue.pop_front();
        if (auto* toast = Toast::create(next)) {
            toast->show();
            s_active = toast;
        }
    }

    if (!s_active) return;

    auto* active = s_active;
    if (!active->m_root || !active->m_root->getParent()) {
        active->m_alive = false;
        s_active = nullptr;
        return;
    }

    active->m_elapsed += std::max(0.f, dt);
    if (active->m_elapsed >= active->m_holdTime + 0.32f && !active->m_leaving) {
        active->dismiss();
    }
    if (active->m_leaving && active->m_elapsed >= active->m_holdTime + 0.62f) {
        active->m_alive = false;
        if (auto* root = active->m_root; root && root->getParent()) {
            root->removeFromParentAndCleanup(true);
        }
        active->m_root = nullptr;
        s_active = nullptr;
    }
}

void Toast::show(CCNode* parent) {
    if (!m_alive) return;

    auto* host = parent;
    if (!host) {
        host = CCDirector::sharedDirector()->getRunningScene();
    }
    if (!host) return;

    const auto winSize = CCDirector::sharedDirector()->getWinSize();

    m_root = static_cast<CCNodeRGBA*>(CCNodeRGBA::create());
    if (!m_root) return;
    m_root->setID("na-frame-counter-toast");
    m_root->setPosition({winSize.width * 0.5f, 92.f});
    host->addChild(m_root, 1000005);

    buildChildren();
    runInAnimation();
}

void Toast::buildChildren() {
    if (!m_root) return;

    const auto tint = toneColor(m_request.tone);

    m_card = NineSlice::create("square02b_001.png");
    if (m_card) {
        m_card->setContentSize({kCardW, kCardH});
        m_card->setAnchorPoint({0.5f, 0.5f});
        m_card->setPosition({0.f, 0.f});
        m_card->setColor({12, 16, 26});
        m_card->setOpacity(216);
        m_root->addChild(m_card, 1);
    }

    // Left accent bar: instant visual tone read without recoloring the card.
    m_accent = CCLayerColor::create({tint.r, tint.g, tint.b, 255}, 4.f, kCardH - 6.f);
    if (m_accent) {
        // CCNode exposes this as ignoreAnchorPointForPosition(bool); there is no
        // setIgnoreAnchorPointForPosition in the Geode/cocos headers.
        m_accent->ignoreAnchorPointForPosition(false);
        m_accent->setAnchorPoint({0.f, 0.5f});
        m_accent->setPosition({-kCardW * 0.5f + 3.f, 0.f});
        m_root->addChild(m_accent, 2);
    }

    const bool hasBadge = !m_request.badge.empty();
    const bool hasDetail = !m_request.detail.empty();
    const float textRight = hasBadge ? (kCardW * 0.5f - 62.f) : (kCardW * 0.5f - 12.f);

    if (auto* label = CCLabelBMFont::create(m_request.title.c_str(), "goldFont.fnt")) {
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({-kCardW * 0.5f + 14.f, hasDetail ? 11.f : 7.f});
        label->setScale(0.46f);
        label->limitLabelWidth(textRight + kCardW * 0.5f - 14.f, 0.46f, 0.22f);
        m_root->addChild(label, 3);
        m_titleLabel = label;
    }

    if (auto* label = CCLabelBMFont::create(m_request.body.c_str(), "bigFont.fnt")) {
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({-kCardW * 0.5f + 14.f, hasDetail ? -3.f : -7.f});
        label->setScale(0.36f);
        label->limitLabelWidth(textRight + kCardW * 0.5f - 14.f, 0.36f, 0.18f);
        label->setColor(tint);
        m_root->addChild(label, 3);
        m_bodyLabel = label;
    }

    if (hasDetail) {
        if (auto* label = CCLabelBMFont::create(m_request.detail.c_str(), "chatFont.fnt")) {
            label->setAnchorPoint({0.f, 0.5f});
            label->setPosition({-kCardW * 0.5f + 14.f, -16.f});
            label->setScale(0.56f);
            label->limitLabelWidth(textRight + kCardW * 0.5f - 14.f, 0.56f, 0.30f);
            label->setColor({176, 190, 210});
            m_root->addChild(label, 3);
            m_detailLabel = label;
        }
    }

    if (hasBadge) {
        auto* pill = NineSlice::create("square02b_001.png");
        if (pill) {
            pill->setContentSize({52.f, 20.f});
            pill->setAnchorPoint({1.f, 0.5f});
            pill->setPosition({kCardW * 0.5f - 10.f, 0.f});
            pill->setColor(tint);
            pill->setOpacity(70);
            m_root->addChild(pill, 2);
        }
        if (auto* label = CCLabelBMFont::create(m_request.badge.c_str(), "bigFont.fnt")) {
            label->setAnchorPoint({0.5f, 0.5f});
            label->setPosition({kCardW * 0.5f - 36.f, 0.f});
            label->setScale(0.34f);
            label->limitLabelWidth(46.f, 0.34f, 0.18f);
            label->setColor(tint);
            m_root->addChild(label, 3);
            m_badgeLabel = label;
        }
    }
}

void Toast::runInAnimation() {
    if (!m_root) return;

    const bool animate = animEnabled;
    const bool pop = (m_request.style == ToastStyle::Pop || m_request.style == ToastStyle::SlidePop);
    const bool slide = (m_request.style == ToastStyle::Slide || m_request.style == ToastStyle::SlidePop);

    m_elapsed = 0.f;
    m_holdTime = std::max(0.4f, m_request.duration);
    m_baseScale = 1.f;

    if (!animate) {
        m_root->setScale(m_baseScale);
        m_root->setOpacity(255);
        if (m_card) m_card->setOpacity(255);
        return;
    }

    // Every animated style starts transparent and settles at full opacity.
    m_root->setOpacity(0);
    m_root->setScale(pop ? m_baseScale * 0.78f : m_baseScale);
    if (m_card) m_card->setOpacity(255);

    // Shared: fade the whole card in, then settle the container scale.
    CCFiniteTimeAction* fadeIn = CCFadeTo::create(0.15f, 255);
    CCFiniteTimeAction* settle = CCEaseSineOut::create(CCScaleTo::create(0.18f, m_baseScale));

    if (slide) {
        // Rise into place on an ease-out curve while fading in.
        m_root->setPositionY(m_root->getPositionY() - kRiseDistance);
        CCFiniteTimeAction* rise = CCEaseSineOut::create(
            CCMoveBy::create(0.26f, {0.f, kRiseDistance})
        );
        m_root->runAction(CCSpawn::createWithTwoActions(fadeIn, rise));
    } else {
        m_root->runAction(fadeIn);
    }

    if (pop) {
        // Overshoot with ease-back-out, then settle. This is what makes "Pop"
        // read as a pop rather than a silent scale-in.
        m_root->runAction(CCEaseBackOut::create(
            CCScaleTo::create(0.24f, m_baseScale * 1.04f)
        ));
        m_root->runAction(CCSequence::createWithTwoActions(
            CCDelayTime::create(0.26f),
            settle
        ));
    } else {
        m_root->runAction(settle);
    }

    if (pop && slide) {
        // Secondary bounce on the card so the content settles a beat after the
        // container, which reads as a clean staggered animation.
        if (auto* card = m_card) {
            card->setScale(0.94f);
            card->runAction(CCSequence::createWithTwoActions(
                CCDelayTime::create(0.06f),
                CCEaseBackOut::create(CCScaleTo::create(0.26f, 1.f))
            ));
        }
    }

    // Accent bar grows in from the middle.
    if (m_accent) {
        m_accent->setScaleY(0.15f);
        m_accent->runAction(CCSequence::createWithTwoActions(
            CCDelayTime::create(0.05f),
            CCEaseSineOut::create(CCScaleTo::create(0.22f, 1.f))
        ));
    }

    // Body label nudges right then settles: cheap, but very readable.
    if (m_bodyLabel) {
        const auto base = m_bodyLabel->getPosition();
        m_bodyLabel->setPositionX(base.x - 6.f);
        m_bodyLabel->setOpacity(0);
        m_bodyLabel->runAction(CCSequence::createWithTwoActions(
            CCDelayTime::create(0.08f),
            CCSpawn::createWithTwoActions(
                CCEaseSineOut::create(CCMoveTo::create(0.22f, base)),
                CCFadeTo::create(0.18f, 255)
            )
        ));
    }
}

void Toast::dismiss() {
    if (!m_alive || m_leaving) return;
    if (!m_root || !m_root->getParent()) {
        m_alive = false;
        if (s_active == this) s_active = nullptr;
        return;
    }

    m_leaving = true;
    m_elapsed = std::max(m_elapsed, m_holdTime);

    stopNodeActions(m_root);
    if (auto* card = m_card) {
        stopNodeActions(card);
    }

    if (!animEnabled) {
        m_root->setOpacity(0);
        return;
    }

    m_root->runAction(CCSequence::createWithTwoActions(
        CCSpawn::createWithTwoActions(
            CCFadeOut::create(0.22f),
            CCEaseSineOut::create(CCMoveBy::create(0.22f, {0.f, -14.f}))
        ),
        CCRemoveSelf::create(true)
    ));
}

ToastRequest makeMeasurementToast(
    std::uint64_t frames,
    double phase,
    bool isRelease,
    bool overLimit,
    std::string const& tag,
    ToastTone tone,
    std::string const& detail
) {
    ToastRequest req;
    req.tone = tone;
    req.badge = tag;
    req.detail = detail;
    req.style = ToastStyle::SlidePop;
    req.duration = 1.7f;

    if (isRelease) {
        req.title = "Release Window";
    } else {
        req.title = overLimit ? "Window Over Limit" : "Click Window";
    }

    std::string phaseText;
    if (!overLimit && phase > 0.001) {
        phaseText = fmt::format("  (+{:.2f}F)", phase);
    }
    req.body = fmt::format("{:.2f}F{}", static_cast<double>(frames) + phase, phaseText);
    if (overLimit) {
        req.body = "> max window";
    }
    return req;
}

ToastRequest makeCbfToast(std::string const& tag, std::string const& detail, ToastTone tone) {
    ToastRequest req;
    req.title = "Click Between Frames";
    req.body = tag;
    req.detail = detail;
    req.badge = "CBF";
    req.tone = tone;
    req.style = ToastStyle::Pop;
    req.duration = 2.2f;
    return req;
}

ToastRequest makeMilestoneToast(
    std::string const& title,
    std::string const& body,
    std::string const& badge,
    ToastTone tone
) {
    ToastRequest req;
    req.title = title;
    req.body = body;
    req.badge = badge;
    req.tone = tone;
    req.style = ToastStyle::SlidePop;
    req.duration = 2.0f;
    return req;
}

}
