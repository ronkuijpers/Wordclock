#include "clock_display.h"
#include "led_controller.h"
#include "led_state.h"
#include "setup_state.h"
#include "night_mode.h"
#include "time_sync.h"
#include "grid_layout.h"
#include "log.h"
#include <algorithm>
#include <cstring>
#include <map>

// Global instance
ClockDisplay clockDisplay;

// External references
extern bool clockEnabled;
extern bool g_initialTimeSyncSucceeded;

// ============================================================================
// Constructor and Lifecycle
// ============================================================================

ClockDisplay::ClockDisplay() {
    reset();
}

void ClockDisplay::reset() {
    animation_ = AnimationState();
    time_ = TimeState();
    hetIs_ = HetIsState();
    noTimeIndicator_ = NoTimeIndicatorState();
    lastSegments_.clear();
    targetSegments_.clear();
    forceAnimation_ = false;
    loggedInitialTimeFailure_ = false;
    fadeController_.clear();
    previewActive_ = false;
    previewLoopCount_ = 0;
    previewStartMs_ = 0;
    previewNeedsTrigger_ = false;
    lastExtraMinutes_ = -1;
}

// ============================================================================
// Main Update Loop
// ============================================================================

bool ClockDisplay::update() {
    unsigned long nowMs = millis();
    
    // Check preconditions
    if (!checkClockEnabled()) {
        return false;
    }
    
    // Update time cache
    if (!updateTimeCache(nowMs)) {
        handleNoTime(nowMs);
        return false;
    }
    
    // Prepare display time (with sell mode override)
    DisplayTime dt = prepareDisplayTime();
    
    // Check if we need to start animation
    triggerAnimationIfNeeded(dt, nowMs);
    
    // Update fade effects (if any active)
    if (fadeController_.hasActiveFades()) {
        fadeController_.updateFades(nowMs);
    }
    
    // Execute animation or display static
    if (animation_.active) {
        executeAnimationStep(nowMs);
    } else {
        displayStaticTime(dt);
    }
    
    return true;
}

// ============================================================================
// Precondition Checks
// ============================================================================

bool ClockDisplay::checkClockEnabled() {
    if (!clockEnabled) {
        animation_.active = false;
        showLeds({});
        resetNoTimeIndicator();
        return false;
    }
    
    if (!setupState.isComplete()) {
        animation_.active = false;
        showLeds({});
        resetNoTimeIndicator();
        return false;
    }
    
    return true;
}

// ============================================================================
// Time Management
// ============================================================================

bool ClockDisplay::updateTimeCache(unsigned long nowMs) {
    // Refresh cached time at most once per second
    if (!time_.valid || (nowMs - time_.lastFetchMs) >= 1000UL) {
        struct tm t = {};
        if (getLocalTime(&t)) {
            time_.cached = t;
            time_.valid = true;
            time_.lastFetchMs = nowMs;
            g_initialTimeSyncSucceeded = true;
            loggedInitialTimeFailure_ = false;
            nightMode.updateFromTime(time_.cached);
            resetNoTimeIndicator();
            return true;
        } else if (!time_.valid) {
            // First failure - no cached time available
            return false;
        }
    }
    
    return time_.valid;
}

void ClockDisplay::handleNoTime(unsigned long nowMs) {
    if (!loggedInitialTimeFailure_) {
        logWarn("❗ Unable to fetch time; showing no-time indicator");
        loggedInitialTimeFailure_ = true;
    }
    nightMode.markTimeInvalid();
    showNoTimeIndicator(nowMs);
}

void ClockDisplay::ensureNoTimeIndicatorLeds() {
    if (!noTimeIndicator_.leds.empty()) return;
    size_t count = EXTRA_MINUTE_LED_COUNT >= 4 ? 4 : EXTRA_MINUTE_LED_COUNT;
    for (size_t i = 0; i < count; ++i) {
        noTimeIndicator_.leds.push_back(EXTRA_MINUTE_LEDS[i]);
    }
}

void ClockDisplay::showNoTimeIndicator(unsigned long nowMs) {
    ensureNoTimeIndicatorLeds();
    if (noTimeIndicator_.startMs == 0) {
        noTimeIndicator_.startMs = nowMs;
    }
    const unsigned long elapsed = nowMs - noTimeIndicator_.startMs;
    const unsigned long phase = elapsed % 5000UL; // 5 second cycle
    if (phase < 500UL) {
        showLeds(noTimeIndicator_.leds);
    } else {
        showLeds({});
    }
}

void ClockDisplay::resetNoTimeIndicator() {
    noTimeIndicator_.startMs = 0;
    noTimeIndicator_.leds.clear();
}

ClockDisplay::DisplayTime ClockDisplay::prepareDisplayTime() {
    ClockDisplay::DisplayTime dt;
    
    // Use preview time if preview is active, otherwise use cached time
    if (previewActive_) {
        dt.effective = previewTime_;
    } else {
        dt.effective = time_.cached;
    }
    
    // Apply sell-mode override (forces 10:47)
    if (displaySettings.isSellMode()) {
        dt.effective.tm_hour = 10;
        dt.effective.tm_min = 47;
    }
    
    dt.rounded = (dt.effective.tm_min / 5) * 5;
    dt.extra = dt.effective.tm_min % 5;
    
    return dt;
}

// ============================================================================
// Animation Management
// ============================================================================

void ClockDisplay::triggerAnimationIfNeeded(const DisplayTime& dt, unsigned long nowMs) {
    // Start animation when the rounded bucket changes, when forced externally, or when preview starts
    bool shouldAnimate = false;
    
    if (previewActive_) {
        // In preview mode, trigger animation when forced or when preview just started
        if (forceAnimation_ || previewNeedsTrigger_) {
            shouldAnimate = true;
            previewNeedsTrigger_ = false;
        }
    } else {
        // Normal mode: trigger on bucket change or force
        shouldAnimate = (dt.rounded != time_.lastRoundedMinute || forceAnimation_);
    }
    
    if (shouldAnimate) {
        if (!previewActive_) {
            time_.lastRoundedMinute = dt.rounded;
        }
        
        struct tm animTime = forceAnimation_ ? forcedTime_ : dt.effective;
        buildAnimationFrames(DisplayTime{animTime, dt.rounded, dt.extra}, nowMs);
        
        forceAnimation_ = false;
    }
}

void ClockDisplay::buildAnimationFrames(const DisplayTime& dt, unsigned long nowMs) {
    struct tm effectiveTime = dt.effective;
    targetSegments_ = get_word_segments_with_keys(&effectiveTime);
    
    uint16_t hisSec = displaySettings.getHetIsDurationSec();
    stripHetIsIfDisabled(targetSegments_, hisSec);
    
    bool animate = displaySettings.getAnimateWords();
    WordAnimationMode mode = displaySettings.getAnimationMode();
    
    if (animate) {
        bool hetIsVisible = hetIsCurrentlyVisible(hisSec, hetIs_.visibleUntil, nowMs);
        
        if (mode == WordAnimationMode::Smart && !lastSegments_.empty()) {
            buildSmartFrames(lastSegments_, targetSegments_, hetIsVisible, animation_.frames);
        } else {
            buildClassicFrames(targetSegments_, animation_.frames);
        }
        
        // Add extra minute LEDs progressively (one per frame) after all words
        // This ensures each stripe fades in separately and works correctly when minute changes
        if (!animation_.frames.empty() && dt.extra > 0) {
            for (int i = 0; i < dt.extra && i < 4; ++i) {
                std::vector<uint16_t> minuteFrame = animation_.frames.back();
                minuteFrame.push_back(EXTRA_MINUTE_LEDS[i]);
                animation_.frames.push_back(minuteFrame);
            }
        }
        
        if (!animation_.frames.empty()) {
            animation_.active = true;
            animation_.currentStep = 0;
            animation_.lastStepAt = millis();
            hetIs_.visibleUntil = 0;  // Reset; will be set when animation completes
            
            // Handle fade effects for LEDs that changed (including minute stripes)
            FadeEffect fadeEffect = displaySettings.getFadeEffect();
            if (fadeEffect != FadeEffect::None) {
                // Get the final frame (with minute stripes if any)
                std::vector<uint16_t> newFinalFrame = animation_.frames.back();
                
                // Clear fades for LEDs that are no longer needed
                fadeController_.clearFadesNotIn(newFinalFrame);
                
                // Handle minute stripe changes: fade out stripes that are no longer needed
                if (lastExtraMinutes_ >= 0 && lastExtraMinutes_ != dt.extra) {
                    uint16_t fadeDuration = displaySettings.getFadeDurationMs();
                    // Fade out minute stripes that were on before but shouldn't be now
                    for (int i = dt.extra; i < lastExtraMinutes_ && i < 4; ++i) {
                        uint16_t minuteLed = EXTRA_MINUTE_LEDS[i];
                        uint8_t currentBrightness = fadeController_.getCurrentBrightness(minuteLed);
                        if (currentBrightness > 0) {
                            fadeController_.startFade(minuteLed, 0, fadeDuration, FadeEffect::FadeOut);
                        }
                    }
                }
            }
            
            // Update last extra minutes count
            lastExtraMinutes_ = dt.extra;
        } else {
            animation_.active = false;
        }
    } else {
        animation_.active = false;
    }
    
    // If not animating, set HET IS visibility immediately
    if (!animation_.active) {
        updateHetIsVisibility(nowMs);
        lastSegments_ = targetSegments_;
    }
}

void ClockDisplay::executeAnimationStep(unsigned long nowMs) {
    unsigned long deltaMs = (animation_.currentStep == 0) ? 0 : (nowMs - animation_.lastStepAt);
    
    // Get configurable animation speed from settings
    uint16_t frameDelayMs = displaySettings.getAnimationSpeedMs();
    
    if (animation_.currentStep == 0 || deltaMs >= frameDelayMs) {
        if (animation_.currentStep < (int)animation_.frames.size()) {
            const auto& frame = animation_.frames[animation_.currentStep];
            
            // Logging
            size_t prevSize = (animation_.currentStep == 0) ? 0 
                            : animation_.frames[animation_.currentStep - 1].size();
            int stepIndex = animation_.currentStep; // capture before increment
            animation_.currentStep++;
            
            String msg = "Anim step ";
            msg += (stepIndex + 1);
            msg += "/";
            msg += animation_.frames.size();
            msg += " dt=";
            msg += deltaMs;
            msg += "ms (Δ";
            msg += (int)frame.size() - (int)prevSize;
            msg += " leds)";
            // Warn if actual delay is > 20% longer than configured delay
            uint16_t thresholdMs = frameDelayMs + (frameDelayMs / 5); // frameDelayMs * 1.2
            if (deltaMs > thresholdMs) {
                msg += " ⚠️ slow";
                logWarn(msg);
            } else {
                logDebug(msg);
            }
            
            // Apply fade effects if enabled
            FadeEffect fadeEffect = displaySettings.getFadeEffect();
            if (fadeEffect != FadeEffect::None) {
                uint16_t fadeDuration = displaySettings.getFadeDurationMs();
                fadeController_.setFadeEffect(fadeEffect);
                
                // Get previous frame to determine new LEDs
                std::vector<uint16_t> prevFrame;
                if (stepIndex > 0) {
                    prevFrame = animation_.frames[stepIndex - 1];
                }
                
                // Find new LEDs in current frame
                std::vector<uint16_t> newLeds;
                for (uint16_t led : frame) {
                    if (std::find(prevFrame.begin(), prevFrame.end(), led) == prevFrame.end()) {
                        newLeds.push_back(led);
                    }
                }
                
                // Start fades for new LEDs
                for (uint16_t led : newLeds) {
                    fadeController_.startFade(led, 255, fadeDuration, fadeEffect);
                }
                
                // Build brightness multipliers for all LEDs in frame
                std::vector<uint8_t> brightnessMultipliers;
                for (uint16_t led : frame) {
                    brightnessMultipliers.push_back(fadeController_.getCurrentBrightness(led));
                }
                
                // Show LEDs with fade effects
                showLedsWithBrightness(frame, brightnessMultipliers);
            } else {
                // No fade - instant display
                showLeds(frame);
            }
            
            animation_.lastStepAt = nowMs;
        }
        
        if (animation_.currentStep >= (int)animation_.frames.size()) {
            // Display final frame with fade effects before ending animation
            FadeEffect fadeEffect = displaySettings.getFadeEffect();
            if (fadeEffect != FadeEffect::None && !animation_.frames.empty()) {
                const auto& finalFrame = animation_.frames.back();
                uint16_t fadeDuration = displaySettings.getFadeDurationMs();
                fadeController_.setFadeEffect(fadeEffect);
                
                // Get previous frame to find new LEDs (including extra minute LEDs)
                std::vector<uint16_t> prevFrame;
                if (animation_.frames.size() > 1) {
                    prevFrame = animation_.frames[animation_.frames.size() - 2];
                }
                
                // Start fades for any new LEDs in final frame (including extra minute LEDs)
                for (uint16_t led : finalFrame) {
                    if (std::find(prevFrame.begin(), prevFrame.end(), led) == prevFrame.end()) {
                        fadeController_.startFade(led, 255, fadeDuration, fadeEffect);
                    }
                }
                
                // Display final frame with fade effects
                std::vector<uint8_t> brightnessMultipliers;
                for (uint16_t led : finalFrame) {
                    brightnessMultipliers.push_back(fadeController_.getCurrentBrightness(led));
                }
                showLedsWithBrightness(finalFrame, brightnessMultipliers);
            }
            
            animation_.active = false;
            updateHetIsVisibility(nowMs);
            lastSegments_ = targetSegments_;
        }
    } else if (animation_.currentStep > 0 && animation_.currentStep <= (int)animation_.frames.size()) {
        // Re-display current frame (called between animation steps)
        // Apply fade effects if enabled
        FadeEffect fadeEffect = displaySettings.getFadeEffect();
        if (fadeEffect != FadeEffect::None) {
            const auto& frame = animation_.frames[animation_.currentStep - 1];
            std::vector<uint8_t> brightnessMultipliers;
            for (uint16_t led : frame) {
                brightnessMultipliers.push_back(fadeController_.getCurrentBrightness(led));
            }
            showLedsWithBrightness(frame, brightnessMultipliers);
        } else {
            showLeds(animation_.frames[animation_.currentStep - 1]);
        }
    }
}

// ============================================================================
// Static Display
// ============================================================================

void ClockDisplay::displayStaticTime(const DisplayTime& dt) {
    struct tm effectiveTime = dt.effective;
    std::vector<WordSegment> baseSegs = get_word_segments_with_keys(&effectiveTime);
    uint16_t hisSec = displaySettings.getHetIsDurationSec();
    stripHetIsIfDisabled(baseSegs, hisSec);
    
    std::vector<uint16_t> indices;
    bool hideHetIs = shouldHideHetIs(millis());
    
    // Track state changes for logging
    if (hideHetIs && !hetIs_.lastHidden) {
        // 'HET IS' hidden after configured duration (optional debug log)
    }
    hetIs_.lastHidden = hideHetIs;
    
    for (const auto& seg : baseSegs) {
        if (hideHetIs && isHetIs(seg)) continue;
        indices.insert(indices.end(), seg.leds.begin(), seg.leds.end());
    }
    
    // Add extra minute LEDs
    for (int i = 0; i < dt.extra && i < 4; ++i) {
        indices.push_back(EXTRA_MINUTE_LEDS[i]);
    }
    
    // Apply fade effects if enabled (for extra minute LEDs that appear after animation)
    FadeEffect fadeEffect = displaySettings.getFadeEffect();
    if (fadeEffect != FadeEffect::None && !indices.empty()) {
        uint16_t fadeDuration = displaySettings.getFadeDurationMs();
        fadeController_.setFadeEffect(fadeEffect);
        
        // Get previous display state to find new LEDs
        // Build previous indices from lastSegments_ and check what was shown before
        std::vector<uint16_t> prevIndices;
        if (!lastSegments_.empty()) {
            for (const auto& seg : lastSegments_) {
                if (hideHetIs && isHetIs(seg)) continue;
                prevIndices.insert(prevIndices.end(), seg.leds.begin(), seg.leds.end());
            }
        }
        
        // Also check if we just finished animation - use the final animation frame
        if (!animation_.frames.empty() && !animation_.active) {
            const auto& finalFrame = animation_.frames.back();
            for (uint16_t led : finalFrame) {
                if (std::find(prevIndices.begin(), prevIndices.end(), led) == prevIndices.end()) {
                    prevIndices.push_back(led);
                }
            }
        }
        
        // Handle minute stripe changes in static display
        if (lastExtraMinutes_ >= 0 && lastExtraMinutes_ != dt.extra) {
            // Fade out minute stripes that are no longer needed
            for (int i = dt.extra; i < lastExtraMinutes_ && i < 4; ++i) {
                uint16_t minuteLed = EXTRA_MINUTE_LEDS[i];
                uint8_t currentBrightness = fadeController_.getCurrentBrightness(minuteLed);
                if (currentBrightness > 0) {
                    fadeController_.startFade(minuteLed, 0, fadeDuration, FadeEffect::FadeOut);
                }
            }
        }
        
        // Start fades for new LEDs (including extra minute LEDs that weren't in animation)
        for (uint16_t led : indices) {
            if (std::find(prevIndices.begin(), prevIndices.end(), led) == prevIndices.end()) {
                fadeController_.startFade(led, 255, fadeDuration, fadeEffect);
            }
        }
        
        // Update last extra minutes count
        lastExtraMinutes_ = dt.extra;
        
        // Build brightness multipliers for all LEDs (respecting ongoing fades)
        std::vector<uint8_t> brightnessMultipliers;
        for (uint16_t led : indices) {
            brightnessMultipliers.push_back(fadeController_.getCurrentBrightness(led));
        }
        
        showLedsWithBrightness(indices, brightnessMultipliers);
    } else {
        showLeds(indices);
    }
    
    lastSegments_ = baseSegs;
}

bool ClockDisplay::shouldHideHetIs(unsigned long nowMs) {
    uint16_t hisSec = displaySettings.getHetIsDurationSec();
    
    if (hisSec == 0) return true;              // Never show
    if (hisSec >= 360) return false;           // Always show
    
    return (hetIs_.visibleUntil != 0) && (nowMs >= hetIs_.visibleUntil);
}

void ClockDisplay::updateHetIsVisibility(unsigned long nowMs) {
    uint16_t hisSec = displaySettings.getHetIsDurationSec();
    
    if (hisSec >= 360) {
        hetIs_.visibleUntil = 0;  // Always on
    } else if (hisSec == 0) {
        hetIs_.visibleUntil = 1;  // Hidden immediately
    } else {
        hetIs_.visibleUntil = nowMs + (unsigned long)hisSec * 1000UL;
    }
}

// ============================================================================
// Public API
// ============================================================================

void ClockDisplay::forceAnimationForTime(const struct tm& time) {
    forcedTime_ = time;
    forceAnimation_ = true;
}

// ============================================================================
// Preview System
// ============================================================================

void ClockDisplay::startPreview(const struct tm& time, int loopCount) {
    previewActive_ = true;
    previewTime_ = time;
    previewStartMs_ = millis();
    previewLoopCount_ = loopCount;
    previewNeedsTrigger_ = true; // Flag to trigger animation on next update
    // Reset animation state to ensure fresh start
    animation_.active = false;
    animation_.currentStep = 0;
    time_.lastRoundedMinute = -1; // Force trigger
}

void ClockDisplay::stopPreview() {
    previewActive_ = false;
    previewLoopCount_ = 0;
    previewStartMs_ = 0;
    previewNeedsTrigger_ = false;
    fadeController_.clear();
    // Reset animation state
    animation_.active = false;
    animation_.currentStep = 0;
    time_.lastRoundedMinute = -1; // Force refresh on next update
}

bool ClockDisplay::isPreviewActive() const {
    return previewActive_;
}

// ============================================================================
// Helper Methods (static)
// ============================================================================

bool ClockDisplay::isHetIs(const WordSegment& seg) {
    return strcmp(seg.key, "HET") == 0 || strcmp(seg.key, "IS") == 0;
}

void ClockDisplay::stripHetIsIfDisabled(std::vector<WordSegment>& segs, uint16_t hetIsDurationSec) {
    if (hetIsDurationSec != 0) return;
    segs.erase(std::remove_if(segs.begin(), segs.end(), 
                             [](const WordSegment& s) { return isHetIs(s); }), 
               segs.end());
}

std::vector<uint16_t> ClockDisplay::flattenSegments(const std::vector<WordSegment>& segs) {
    std::vector<uint16_t> indices;
    for (const auto& seg : segs) {
        indices.insert(indices.end(), seg.leds.begin(), seg.leds.end());
    }
    return indices;
}

const WordSegment* ClockDisplay::findSegment(const std::vector<WordSegment>& segs, const char* key) {
    for (const auto& seg : segs) {
        if (strcmp(seg.key, key) == 0) return &seg;
    }
    return nullptr;
}

void ClockDisplay::removeLeds(std::vector<uint16_t>& base, const std::vector<uint16_t>& toRemove) {
    base.erase(std::remove_if(base.begin(), base.end(), [&](uint16_t idx) {
        return std::find(toRemove.begin(), toRemove.end(), idx) != toRemove.end();
    }), base.end());
}

bool ClockDisplay::hetIsCurrentlyVisible(uint16_t hetIsDurationSec, unsigned long hetIsVisibleUntil, unsigned long nowMs) {
    if (hetIsDurationSec == 0) return false;
    if (hetIsDurationSec >= 360) return true;
    if (hetIsVisibleUntil == 0) return true;
    return nowMs < hetIsVisibleUntil;
}

void ClockDisplay::buildClassicFrames(const std::vector<WordSegment>& segs, 
                                     std::vector<std::vector<uint16_t>>& frames) {
    frames.clear();
    std::vector<uint16_t> cumulative;
    for (const auto& seg : segs) {
        cumulative.insert(cumulative.end(), seg.leds.begin(), seg.leds.end());
        frames.push_back(cumulative);
    }
}

void ClockDisplay::buildSmartFrames(const std::vector<WordSegment>& prevSegments,
                                   const std::vector<WordSegment>& nextSegments,
                                   bool hetIsVisible,
                                   std::vector<std::vector<uint16_t>>& frames) {
    frames.clear();
    if (prevSegments.empty()) {
        buildClassicFrames(nextSegments, frames);
        return;
    }
    
    std::vector<WordSegment> prevVisible;
    prevVisible.reserve(prevSegments.size());
    for (const auto& seg : prevSegments) {
        if (isHetIs(seg) && !hetIsVisible) continue;
        prevVisible.push_back(seg);
    }
    
    std::vector<uint16_t> current = flattenSegments(prevVisible);
    
    std::vector<WordSegment> removals;
    for (const auto& seg : prevVisible) {
        bool presentInNext = findSegment(nextSegments, seg.key) != nullptr;
        if (isHetIs(seg) || !presentInNext) {
            removals.push_back(seg);
        }
    }
    
    std::vector<WordSegment> additions;
    for (const auto& seg : nextSegments) {
        bool visibleBefore = findSegment(prevVisible, seg.key) != nullptr;
        if (isHetIs(seg) || !visibleBefore) {
            additions.push_back(seg);
        }
    }
    
    if (!removals.empty()) {
        std::vector<uint16_t> removalLeds;
        for (const auto& rem : removals) {
            removalLeds.insert(removalLeds.end(), rem.leds.begin(), rem.leds.end());
        }
        removeLeds(current, removalLeds);
        frames.push_back(current);
    }
    for (const auto& add : additions) {
        current.insert(current.end(), add.leds.begin(), add.leds.end());
        frames.push_back(current);
    }
    if (frames.empty()) {
        frames.push_back(current);
    }
}

