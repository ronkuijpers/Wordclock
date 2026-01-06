#ifndef FADE_CONTROLLER_H
#define FADE_CONTROLLER_H

#include <vector>
#include <time.h>
#include "display_settings.h"

/**
 * @brief Manages fade effects for LED animations
 * 
 * Handles smooth brightness transitions for individual LEDs during animations.
 * Uses color intensity adjustment since NeoPixel doesn't support per-LED brightness.
 */
class FadeController {
public:
    struct FadeState {
        uint16_t ledIndex;
        uint8_t startBrightness;      // 0-255, represents color intensity
        uint8_t targetBrightness;     // 0-255
        unsigned long startTime;
        unsigned long duration;
        bool active;
    };

    FadeController() = default;

    /**
     * @brief Start a fade for a single LED
     * @param ledIndex LED index to fade
     * @param targetBrightness Target brightness (0-255, where 255 = full color)
     * @param duration Fade duration in milliseconds
     * @param effect Fade effect type (determines start brightness)
     */
    void startFade(uint16_t ledIndex, uint8_t targetBrightness, 
                   unsigned long duration, FadeEffect effect);

    /**
     * @brief Set fade effect for tracking FadeInOut second phase
     */
    void setFadeEffect(FadeEffect effect) { fadeEffect_ = effect; }

    /**
     * @brief Update all active fades (call every frame, ~50ms)
     * @param nowMs Current time in milliseconds
     * @return true if any fades are still active
     */
    bool updateFades(unsigned long nowMs);

    /**
     * @brief Get current brightness for an LED (0-255)
     * @param ledIndex LED index
     * @return Current brightness, or 255 if not fading
     */
    uint8_t getCurrentBrightness(uint16_t ledIndex) const;

    /**
     * @brief Clear all active fades
     */
    void clear();

    /**
     * @brief Check if any fades are active
     */
    bool hasActiveFades() const { return !activeFades_.empty(); }

private:
    std::vector<FadeState> activeFades_;
    FadeEffect fadeEffect_ = FadeEffect::None;

    uint8_t lerp(uint8_t start, uint8_t end, float t) const {
        return start + static_cast<uint8_t>((end - start) * t);
    }
};

#endif // FADE_CONTROLLER_H

