#include "SubFrameGeometry.hpp"

#include <algorithm>
#include <cmath>

using namespace cocos2d;

namespace na_frame_counter::engine {

double sanitizePhase(double phase) {
    if (!std::isfinite(phase)) return 0.0;
    double p = std::fmod(phase, 1.0);
    if (p < 0.0) p += 1.0;
    // Never report exactly 1.0: that belongs to the next tick.
    if (p >= 0.9995) p = 0.9995;
    return p;
}

double progressToPhase(double progress) {
    if (!std::isfinite(progress)) return 0.0;
    // Half-tick centred mapping: an input that lands dead-centre inside the
    // collision span is treated as "on the boundary", and the extremes of the
    // span map to the edges of the tick.
    const double centred = (std::clamp(progress, 0.0, 1.0) - 0.5) * 2.0; // -1..1
    return sanitizePhase(0.5 + centred * 0.5);
}

SubFrameGeometry measureSubFrame(
    CCPoint const& playerPos,
    CCPoint const& previousPos,
    CCRect const& targetRect,
    bool grounded,
    double yVelocity,
    double fallbackPhase
) {
    SubFrameGeometry out;
    out.phase = sanitizePhase(fallbackPhase);
    out.offsetFrames = out.phase <= 0.5 ? -out.phase : (1.0 - out.phase);

    if (targetRect.size.width > 0.f && targetRect.size.height > 0.f) {
        // Horizontal travel of the hitbox across the object this tick. A cube
        // that enters an object by half its width was half a tick late.
        const double span = static_cast<double>(targetRect.size.width);
        const double travel = std::abs(static_cast<double>(playerPos.x - previousPos.x));
        if (span > 1e-6) {
            const double entry = static_cast<double>(playerPos.x - targetRect.getMinX());
            const double width = static_cast<double>(targetRect.size.width);
            if (width > 1e-6) {
                // progress: 0 = just touched the leading edge, 1 = about to exit.
                const double progress = std::clamp((entry + travel) / width, 0.0, 1.0);
                out.progress = progress;
                out.phase = progressToPhase(progress);
                out.offsetFrames = out.phase <= 0.5 ? -out.phase : (1.0 - out.phase);
                out.valid = true;
                return out;
            }
        }
    }

    if (grounded) {
        // On the ground a press is essentially instantaneous at the tick, so we
        // keep the fallback (normally 0 = whole-frame) unless the caller has
        // better evidence.
        out.valid = false;
        return out;
    }

    // Airborne: the jump arc gives us a usable "how far into the tick" signal.
    // |v| near the apex means the press was close to the tick; a large |v|
    // means it was closer to the mid-tick.
    const double speed = std::abs(yVelocity);
    if (speed > 1e-6) {
        constexpr double kApexSpeed = 8.0; // px/tick-ish, tuned to GD's jump curve
        const double u = std::clamp(speed / kApexSpeed, 0.0, 1.0);
        out.phase = sanitizePhase(u * 0.5);
        out.offsetFrames = out.phase <= 0.5 ? -out.phase : (1.0 - out.phase);
        out.valid = true;
    }
    return out;
}

}
