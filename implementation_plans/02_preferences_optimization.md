# Implementation Plan: Optimize Preferences Access Patterns

**Priority:** HIGH  
**Estimated Effort:** 8 hours  
**Complexity:** Medium  
**Risk:** Medium (affects persistence layer)  
**Impact:** HIGH - Reduces flash wear, improves performance, extends device lifetime

---

## Problem Statement

The current implementation opens and closes ESP32 Preferences (NVS) on every setter call, causing:

1. **Flash Wear:** Each write causes flash erase cycle (~100,000 cycle lifetime)
   - Typical usage: 50+ writes per day
   - Device lifetime reduced to ~5 years instead of 20+

2. **Performance Issues:** Each operation takes 10-50ms
   - Blocks main loop during writes
   - Causes animation stuttering
   - Poor user experience with MQTT rapid updates

3. **Battery Drain:** Flash operations consume significant power
   - ~30mA peak during write
   - Reduces battery life in portable scenarios

**Affected Classes:**
- `LedState` (`src/led_state.h`)
- `DisplaySettings` (`src/display_settings.h`)  
- `NightMode` (`src/night_mode.h`, `src/night_mode.cpp`)
- `SetupState` (`src/setup_state.h`, `src/setup_state.cpp`)

---

## Current Implementation Analysis

### Example: LedState Class

**Current Code** (`led_state.h` lines 19-30):
```cpp
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
    red = r; green = g; blue = b;
    white = (r == 255 && g == 255 && b == 255) ? 255 : 0;
    if (white) { red = green = blue = 0; }

    prefs.begin("led", false);      // Flash operation (~5ms)
    prefs.putUChar("r", red);       // Flash write (~10ms)
    prefs.putUChar("g", green);     // Flash write (~10ms)
    prefs.putUChar("b", blue);      // Flash write (~10ms)
    prefs.putUChar("w", white);     // Flash write (~10ms)
    prefs.end();                    // Flash operation (~5ms)
}                                   // Total: ~50ms blocking

void setBrightness(uint8_t b) {
    brightness = b;
    prefs.begin("led", false);
    prefs.putUChar("br", brightness);
    prefs.end();
}
```

**Frequency Analysis:**
- Color changes: 5-10 per day (user interaction)
- Brightness changes: 10-20 per day (user + night mode)
- MQTT updates: Can spike to 100+ per hour during automation

**Impact per Operation:**
- 50ms blocking time
- 4 flash sectors affected
- ~20µAh battery drain

---

## Solution Design

### Strategy: Deferred Write Pattern

Implement a write-behind cache with:
1. **Immediate in-memory updates** - UI sees instant response
2. **Dirty flag tracking** - Know what needs persisting
3. **Deferred persistence** - Write only when necessary
4. **Explicit flush API** - Manual control when needed
5. **Automatic flush on critical events** - Power loss protection

### Design Pattern

```cpp
class PersistentSettings {
protected:
    virtual void writeToPreferences() = 0;
    virtual void readFromPreferences() = 0;
    
    void markDirty() { 
        dirty_ = true; 
        lastModified_ = millis();
    }
    
    bool isDirty() const { return dirty_; }
    
    void flushIfNeeded(bool force = false) {
        if (!dirty_) return;
        
        if (force || shouldAutoFlush()) {
            writeToPreferences();
            dirty_ = false;
        }
    }
    
private:
    bool dirty_ = false;
    unsigned long lastModified_ = 0;
    static const unsigned long AUTO_FLUSH_DELAY_MS = 5000;
    
    bool shouldAutoFlush() const {
        return (millis() - lastModified_) >= AUTO_FLUSH_DELAY_MS;
    }
};
```

---

## Implementation Details

### Phase 1: LedState Refactoring

**File:** `src/led_state.h`

**New Implementation:**
```cpp
#ifndef LED_STATE_H
#define LED_STATE_H

#include <Preferences.h>

class LedState {
public:
    /**
     * @brief Initialize LED state from persistent storage
     * @note Call once during setup()
     */
    void begin() {
        prefs_.begin("wc_led", false);  // Note: renamed namespace for safety
        red_   = prefs_.getUChar("r", 0);
        green_ = prefs_.getUChar("g", 0);
        blue_  = prefs_.getUChar("b", 0);
        white_ = prefs_.getUChar("w", 255);
        brightness_ = prefs_.getUChar("br", 64);
        prefs_.end();
        
        dirty_ = false;
        lastFlush_ = millis();
    }

    /**
     * @brief Set RGB color (immediate in-memory, deferred persistence)
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     */
    void setRGB(uint8_t r, uint8_t g, uint8_t b) {
        // Early exit if no change
        if (red_ == r && green_ == g && blue_ == b) {
            return;
        }
        
        red_ = r; 
        green_ = g; 
        blue_ = b;
        white_ = (r == 255 && g == 255 && b == 255) ? 255 : 0;
        if (white_) { 
            red_ = green_ = blue_ = 0; 
        }
        
        markDirty();  // Flag for later persistence
    }

    /**
     * @brief Set brightness (immediate in-memory, deferred persistence)
     * @param b Brightness value (0-255)
     */
    void setBrightness(uint8_t b) {
        if (brightness_ == b) return;
        brightness_ = b;
        markDirty();
    }

    /**
     * @brief Force immediate write to persistent storage
     * @note Call before critical operations (OTA, deep sleep, restart)
     */
    void flush() {
        if (!dirty_) return;
        
        prefs_.begin("wc_led", false);
        prefs_.putUChar("r", red_);
        prefs_.putUChar("g", green_);
        prefs_.putUChar("b", blue_);
        prefs_.putUChar("w", white_);
        prefs_.putUChar("br", brightness_);
        prefs_.end();
        
        dirty_ = false;
        lastFlush_ = millis();
    }

    /**
     * @brief Automatic flush if dirty and sufficient time passed
     * @note Call periodically from main loop (every 1-5 seconds)
     */
    void loop() {
        if (dirty_ && (millis() - lastFlush_) >= AUTO_FLUSH_DELAY_MS) {
            flush();
        }
    }

    // Getters (unchanged)
    uint8_t getBrightness() const { return brightness_; }
    void getRGBW(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &w) const {
        r = red_; g = green_; b = blue_; w = white_;
    }
    
    // New: Query persistence state
    bool isDirty() const { return dirty_; }
    unsigned long millisSinceLastFlush() const { 
        return millis() - lastFlush_; 
    }

private:
    void markDirty() {
        if (!dirty_) {
            dirty_ = true;
            lastFlush_ = millis();  // Track when change occurred
        }
    }

    uint8_t red_ = 0, green_ = 0, blue_ = 0, white_ = 255;
    uint8_t brightness_ = 64;
    bool dirty_ = false;
    unsigned long lastFlush_ = 0;
    
    Preferences prefs_;
    
    static const unsigned long AUTO_FLUSH_DELAY_MS = 5000;  // 5 seconds
};

extern LedState ledState;

#endif // LED_STATE_H
```

**File:** `src/led_state.cpp`
```cpp
#include "led_state.h"

LedState ledState;

// All implementation moved to header (inline for optimization)
```

---

### Phase 2: DisplaySettings Refactoring

**File:** `src/display_settings.h`

**Changes Required:**
1. Add `dirty_` flag and flush tracking
2. Replace immediate writes with `markDirty()`
3. Add `flush()` and `loop()` methods
4. Batch all preferences writes

**Key Improvements:**
```cpp
class DisplaySettings {
public:
    void begin() {
        // ... existing load logic ...
        dirty_ = false;
        lastFlush_ = millis();
    }
    
    // Setters now call markDirty() instead of immediate write
    void setHetIsDurationSec(uint16_t s) {
        if (s > 360) s = 360;
        if (hetIsDurationSec_ == s) return;
        hetIsDurationSec_ = s;
        markDirty();
    }
    
    void setSellMode(bool on) {
        if (sellMode_ == on) return;
        sellMode_ = on;
        markDirty();
    }
    
    // ... similar for all setters ...
    
    void flush() {
        if (!dirty_) return;
        
        prefs_.begin("wc_display", false);
        // Batch write all settings
        prefs_.putUShort("his_sec", hetIsDurationSec_);
        prefs_.putBool("sell_on", sellMode_);
        prefs_.putBool("anim_on", animateWords_);
        prefs_.putUChar("anim_mode", static_cast<uint8_t>(animationMode_));
        prefs_.putBool("auto_upd", autoUpdate_);
        prefs_.putString("upd_ch", updateChannel_);
        prefs_.putUChar("grid_id", gridVariantToId(gridVariant_));
        prefs_.end();
        
        dirty_ = false;
        lastFlush_ = millis();
    }
    
    void loop() {
        if (dirty_ && (millis() - lastFlush_) >= AUTO_FLUSH_DELAY_MS) {
            flush();
        }
    }
    
private:
    void markDirty() {
        if (!dirty_) {
            dirty_ = true;
            lastFlush_ = millis();
        }
    }
    
    bool dirty_ = false;
    unsigned long lastFlush_ = 0;
    static const unsigned long AUTO_FLUSH_DELAY_MS = 5000;
    
    // ... existing members ...
};
```

---

### Phase 3: NightMode Refactoring

**File:** `src/night_mode.h` and `src/night_mode.cpp`

Similar pattern:
1. Remove immediate `prefs.begin()`/`prefs.end()` calls
2. Add `markDirty()` calls
3. Implement `flush()` that writes all settings at once
4. Add `loop()` for auto-flush

**Key Change - Batch Persistence:**
```cpp
// night_mode.cpp
void NightMode::flush() {
    if (!dirty_) return;
    
    prefs_.begin("wc_night", false);
    prefs_.putBool("enabled", enabled_);
    prefs_.putUChar("effect", static_cast<uint8_t>(effect_));
    prefs_.putUChar("dim_pct", dimPercent_);
    prefs_.putUShort("start", startMinutes_);
    prefs_.putUShort("end", endMinutes_);
    prefs_.end();
    
    dirty_ = false;
    lastFlush_ = millis();
}

// Replace individual persist functions with markDirty()
void NightMode::setEnabled(bool on) {
    if (enabled_ == on) return;
    enabled_ = on;
    markDirty();  // Instead of persistEnabled()
    updateEffectiveState("enabled");
}
```

---

### Phase 4: Main Loop Integration

**File:** `src/main.cpp`

Add periodic flush calls:

```cpp
void loop() {
    // ... existing code ...
    
    // Add periodic flush for all settings (every ~1 second)
    static unsigned long lastSettingsFlush = 0;
    if (millis() - lastSettingsFlush >= 1000) {
        ledState.loop();
        displaySettings.loop();
        nightMode.loop();
        // setupState.loop();  // If applicable
        lastSettingsFlush = millis();
    }
    
    // ... rest of loop ...
}
```

Add explicit flush before critical operations:

```cpp
void setup() {
    // ... existing setup ...
    
    // Register flush handler for graceful shutdown
    registerShutdownHandler();
}

void registerShutdownHandler() {
    // Flush on OTA start
    ArduinoOTA.onStart([]() {
        flushAllSettings();
    });
    
    // Note: ESP32 doesn't have reliable shutdown hook
    // Best we can do is flush before known risky operations
}

void flushAllSettings() {
    logDebug("Flushing all settings to persistent storage...");
    ledState.flush();
    displaySettings.flush();
    nightMode.flush();
    setupState.flush();
    logDebug("Settings flush complete");
}

// Call before any ESP.restart()
void safeRestart() {
    flushAllSettings();
    delay(100);  // Allow flash write to complete
    ESP.restart();
}
```

---

## Migration Strategy

### Handling Namespace Changes

The new implementation uses prefixed namespaces (e.g., `wc_led` instead of `led`) for safety. Need migration code:

**File:** `src/settings_migration.h` (new)
```cpp
#ifndef SETTINGS_MIGRATION_H
#define SETTINGS_MIGRATION_H

#include <Preferences.h>
#include "log.h"

class SettingsMigration {
public:
    static void migrateIfNeeded() {
        Preferences prefs;
        
        // Check if migration already done
        prefs.begin("wc_system", true);
        bool migrated = prefs.getBool("migrated_v2", false);
        prefs.end();
        
        if (migrated) {
            return;  // Already migrated
        }
        
        logInfo("⚙️ Migrating settings to new format...");
        
        migrateLedState();
        migrateDisplaySettings();
        migrateNightMode();
        migrateLogSettings();
        
        // Mark as migrated
        prefs.begin("wc_system", false);
        prefs.putBool("migrated_v2", true);
        prefs.end();
        
        logInfo("✅ Settings migration complete");
    }
    
private:
    static void migrateLedState() {
        Preferences oldPrefs, newPrefs;
        
        if (!oldPrefs.begin("led", true)) {
            return;  // No old data
        }
        
        newPrefs.begin("wc_led", false);
        newPrefs.putUChar("r", oldPrefs.getUChar("r", 0));
        newPrefs.putUChar("g", oldPrefs.getUChar("g", 0));
        newPrefs.putUChar("b", oldPrefs.getUChar("b", 0));
        newPrefs.putUChar("w", oldPrefs.getUChar("w", 255));
        newPrefs.putUChar("br", oldPrefs.getUChar("br", 64));
        newPrefs.end();
        oldPrefs.end();
        
        // Clear old namespace
        oldPrefs.begin("led", false);
        oldPrefs.clear();
        oldPrefs.end();
        
        logInfo("  ✓ LED state migrated");
    }
    
    static void migrateDisplaySettings() {
        // Similar pattern for display settings
        // ... implementation
    }
    
    static void migrateNightMode() {
        // Similar pattern for night mode
        // ... implementation
    }
    
    static void migrateLogSettings() {
        // Similar pattern for log settings
        // ... implementation
    }
};

#endif // SETTINGS_MIGRATION_H
```

**Call from main.cpp setup():**
```cpp
void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    delay(MDNS_START_DELAY_MS);
    
    // IMPORTANT: Migrate before initializing settings
    SettingsMigration::migrateIfNeeded();
    
    initLogSettings();
    // ... rest of setup
}
```

---

## Testing Strategy

### Unit Tests

**File:** `test/test_settings_persistence.cpp` (new)
```cpp
#include <unity.h>
#include "led_state.h"
#include "display_settings.h"

void test_led_state_deferred_write() {
    LedState state;
    state.begin();
    
    // Change value
    state.setRGB(255, 0, 0);
    
    // Should be dirty
    TEST_ASSERT_TRUE(state.isDirty());
    
    // Value should be updated in memory immediately
    uint8_t r, g, b, w;
    state.getRGBW(r, g, b, w);
    TEST_ASSERT_EQUAL(255, r);
    TEST_ASSERT_EQUAL(0, g);
    TEST_ASSERT_EQUAL(0, b);
    
    // Flush
    state.flush();
    
    // Should not be dirty
    TEST_ASSERT_FALSE(state.isDirty());
}

void test_auto_flush_timing() {
    LedState state;
    state.begin();
    
    state.setRGB(0, 255, 0);
    TEST_ASSERT_TRUE(state.isDirty());
    
    // Wait less than auto-flush delay
    delay(2000);
    state.loop();
    TEST_ASSERT_TRUE(state.isDirty());  // Still dirty
    
    // Wait for auto-flush
    delay(4000);  // Total > 5 seconds
    state.loop();
    TEST_ASSERT_FALSE(state.isDirty());  // Auto-flushed
}

void test_no_flash_write_on_duplicate_value() {
    LedState state;
    state.begin();
    
    state.setRGB(128, 128, 128);
    state.flush();
    TEST_ASSERT_FALSE(state.isDirty());
    
    // Set to same value
    state.setRGB(128, 128, 128);
    TEST_ASSERT_FALSE(state.isDirty());  // Should not be dirty
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_led_state_deferred_write);
    RUN_TEST(test_auto_flush_timing);
    RUN_TEST(test_no_flash_write_on_duplicate_value);
    UNITY_END();
}

void loop() {}
```

**Run tests:**
```bash
pio test -e native
```

---

### Integration Tests

**Test Scenario 1: Rapid MQTT Updates**
```
Setup:
1. Connect to MQTT
2. Subscribe to color change commands
3. Send 100 color changes over 10 seconds

Expected:
- All color changes reflected immediately in LED display
- Only 1-2 flash writes occur (auto-flush at 5s intervals)
- No animation stuttering

Measure:
- Flash write count (should be ≤ 2)
- Response latency (should be < 50ms per change)
```

**Test Scenario 2: Power Loss Protection**
```
Setup:
1. Change brightness via web UI
2. Wait 3 seconds (less than auto-flush delay)
3. Force restart (simulated power loss)

Expected:
- On reboot, brightness should NOT be changed (didn't flush yet)

Then:
1. Change brightness again
2. Wait 6 seconds (more than auto-flush)
3. Force restart

Expected:
- On reboot, brightness SHOULD be changed (auto-flushed)
```

**Test Scenario 3: Explicit Flush Before OTA**
```
Setup:
1. Change multiple settings (color, brightness, night mode)
2. Immediately trigger OTA update

Expected:
- All settings flushed before OTA starts
- After OTA, all settings preserved correctly
```

---

### Performance Benchmarks

**Metrics to Measure:**

| Operation | Before | After | Improvement |
|-----------|--------|-------|-------------|
| setRGB() blocking time | 50ms | < 1ms | 50x faster |
| setBrightness() time | 25ms | < 1ms | 25x faster |
| 100 rapid updates | 5000ms | 100ms | 50x faster |
| Flash writes per day | 50-100 | 5-10 | 10x reduction |
| Animation smoothness | Stutters | Smooth | Qualitative |

**Benchmark Code:**
```cpp
void benchmarkSettingsPer formance() {
    unsigned long start, end;
    
    // Test 1: Single write performance
    start = micros();
    ledState.setRGB(255, 0, 0);
    end = micros();
    logInfo(String("setRGB time: ") + (end - start) + " µs");
    
    // Test 2: Rapid updates
    start = millis();
    for (int i = 0; i < 100; i++) {
        ledState.setRGB(i, i, i);
    }
    end = millis();
    logInfo(String("100 updates: ") + (end - start) + " ms");
    
    // Test 3: Flash write count
    int flashCount = 0;
    for (int i = 0; i < 100; i++) {
        ledState.setRGB(i, i, i);
        if (ledState.isDirty()) {
            ledState.flush();
            flashCount++;
        }
    }
    logInfo(String("Flash writes: ") + flashCount);
}
```

---

## Rollback Plan

### If Issues Discovered

**Phase 1: Quick Disable**
```cpp
// Add to config.h
#define ENABLE_DEFERRED_PERSISTENCE false

// Wrap new code
#if ENABLE_DEFERRED_PERSISTENCE
    // New deferred implementation
#else
    // Old immediate write implementation
#endif
```

**Phase 2: Full Rollback**
```bash
# Restore from backup
git checkout HEAD~1 src/led_state.h
git checkout HEAD~1 src/display_settings.h
git checkout HEAD~1 src/night_mode.h src/night_mode.cpp

# Rebuild
pio run

# Deploy
pio run -t upload
```

**Phase 3: Migration Reversal**
If namespace change causes issues:
```cpp
// Copy wc_* namespaces back to original names
void rollbackMigration() {
    Preferences oldPrefs, newPrefs;
    
    newPrefs.begin("wc_led", true);
    oldPrefs.begin("led", false);
    // Copy all values back
    oldPrefs.putUChar("r", newPrefs.getUChar("r", 0));
    // ...
    oldPrefs.end();
    newPrefs.end();
}
```

---

## Implementation Timeline

### Week 1: Implementation
- **Day 1-2:** Implement LedState refactoring + unit tests
- **Day 3:** Implement DisplaySettings refactoring
- **Day 4:** Implement NightMode refactoring
- **Day 5:** Main loop integration + migration code

### Week 2: Testing
- **Day 1:** Unit test completion
- **Day 2-3:** Integration testing
- **Day 4:** Performance benchmarking
- **Day 5:** Beta deployment to test devices

### Week 3: Deployment
- **Day 1-2:** Code review and fixes
- **Day 3:** Documentation updates
- **Day 4:** Staged OTA rollout (10% → 50%)
- **Day 5:** Full rollout + monitoring

---

## Success Criteria

### Must Have
- ✅ Flash writes reduced by 80%+
- ✅ Setter operations complete in < 1ms
- ✅ No settings lost on power cycle (after auto-flush delay)
- ✅ All existing functionality preserved
- ✅ Migration from old format successful

### Nice to Have
- ✅ 90%+ flash write reduction
- ✅ Measurable animation smoothness improvement
- ✅ Reduced power consumption (measurable)
- ✅ No memory leaks over 72-hour test

### Documentation
- ✅ Code documentation complete (Doxygen)
- ✅ User guide updated (persistence behavior)
- ✅ Developer guide updated (new pattern)

---

## Related Work

**Dependencies:**
- Bug fix 3.2.1 (Preferences namespace collisions) - addresses same code
- Can be combined into single PR

**Future Enhancements:**
- Wear leveling at application layer
- Settings checksum validation
- Cloud backup integration
- A/B configuration slots

---

## Resources

**ESP32 NVS Documentation:**
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html

**Flash Wear Analysis:**
- Typical NVS: 100,000 erase cycles
- With 10 writes/day: 27+ years
- With 100 writes/day: 2.7 years
- **Target:** < 10 writes/day for 20+ year lifetime

**Power Consumption:**
- Flash write: ~30mA @ 20ms = 0.17mAh per operation
- 100 writes/day = 17mAh/day (6.2Ah/year)
- 10 writes/day = 1.7mAh/day (0.62Ah/year)

---

## Approval Checklist

- [ ] Code implementation complete
- [ ] Unit tests passing (>80% coverage)
- [ ] Integration tests passing
- [ ] Performance benchmarks meet targets
- [ ] Migration tested on old devices
- [ ] Documentation updated
- [ ] Code review approved
- [ ] Beta test successful (7 days)
- [ ] Rollback plan tested

---

**Plan Version:** 1.0  
**Last Updated:** January 4, 2026  
**Status:** Ready for Implementation
