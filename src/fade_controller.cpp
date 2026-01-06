#include "fade_controller.h"
#include "log.h"
#include <algorithm>

void FadeController::startFade(uint16_t ledIndex, uint8_t targetBrightness, 
                                unsigned long duration, FadeEffect effect) {
    // Remove any existing fade for this LED
    activeFades_.erase(
        std::remove_if(activeFades_.begin(), activeFades_.end(),
                      [ledIndex](const FadeState& f) { return f.ledIndex == ledIndex; }),
        activeFades_.end()
    );

    FadeState fade;
    fade.ledIndex = ledIndex;
    fade.targetBrightness = targetBrightness;
    fade.duration = duration;
    fade.startTime = millis();
    fade.active = true;

    // Determine start brightness based on effect
    switch (effect) {
        case FadeEffect::FadeIn:
            fade.startBrightness = 0;
            break;
        case FadeEffect::FadeOut:
            fade.startBrightness = 255;
            fade.targetBrightness = 0;
            break;
        case FadeEffect::FadeInOut:
            // For FadeInOut, we'll start with fade-in (0->255)
            // The fade-out will be handled when the fade completes
            fade.startBrightness = 0;
            fade.targetBrightness = 255;
            break;
        case FadeEffect::None:
        default:
            // Instant - no fade
            fade.startBrightness = targetBrightness;
            fade.duration = 0;
            break;
    }

    activeFades_.push_back(fade);
}

bool FadeController::updateFades(unsigned long nowMs) {
    bool anyActive = false;
    std::vector<FadeState> newFades; // For FadeInOut second phase

    for (auto it = activeFades_.begin(); it != activeFades_.end();) {
        if (!it->active) {
            ++it;
            continue;
        }

        unsigned long elapsed = nowMs - it->startTime;
        
        if (elapsed >= it->duration) {
            // Fade complete
            // For FadeInOut, start fade-out phase if we just completed fade-in
            if (it->targetBrightness == 255 && fadeEffect_ == FadeEffect::FadeInOut) {
                // Start fade-out
                FadeState fadeOut;
                fadeOut.ledIndex = it->ledIndex;
                fadeOut.startBrightness = 255;
                fadeOut.targetBrightness = 0;
                fadeOut.duration = it->duration; // Same duration for fade-out
                fadeOut.startTime = nowMs;
                fadeOut.active = true;
                newFades.push_back(fadeOut);
            }
            
            it = activeFades_.erase(it);
        } else {
            anyActive = true;
            ++it;
        }
    }

    // Add new fades (FadeInOut second phase)
    activeFades_.insert(activeFades_.end(), newFades.begin(), newFades.end());

    return anyActive;
}

uint8_t FadeController::getCurrentBrightness(uint16_t ledIndex) const {
    for (const auto& fade : activeFades_) {
        if (fade.ledIndex == ledIndex && fade.active) {
            unsigned long elapsed = millis() - fade.startTime;
            if (elapsed >= fade.duration) {
                return fade.targetBrightness;
            }
            float progress = static_cast<float>(elapsed) / static_cast<float>(fade.duration);
            return lerp(fade.startBrightness, fade.targetBrightness, progress);
        }
    }
    return 255; // Not fading, full brightness
}

void FadeController::clear() {
    activeFades_.clear();
}

