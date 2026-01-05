# Implementation Plan: Refactor Long Functions

**Priority:** MEDIUM  
**Estimated Effort:** 12 hours  
**Complexity:** Medium  
**Risk:** Medium (requires careful testing)  
**Impact:** MEDIUM - Improves maintainability, testability, and code comprehension

---

## Problem Statement

The codebase contains several functions exceeding 50-100 lines, making them:

1. **Difficult to Understand:** Complex control flow with nested logic
2. **Hard to Test:** Cannot test individual responsibilities in isolation
3. **Bug-Prone:** More code paths = more opportunities for errors
4. **Difficult to Modify:** Changes risk breaking unrelated functionality
5. **Poor Documentation:** Too much happening in one place

**Functions Requiring Refactoring:**

| File | Function | Lines | Complexity | Priority |
|------|----------|-------|------------|----------|
| wordclock.cpp | `wordclock_loop()` | 196 | High | HIGH |
| mqtt_client.cpp | `handleMessage()` | 110 | High | HIGH |
| mqtt_client.cpp | `publishDiscovery()` | 200+ | Medium | MEDIUM |
| main.cpp | `loop()` | 68 | Medium | MEDIUM |
| mqtt_client.cpp | `mqtt_connect()` | 50+ | Medium | LOW |

---

## Refactoring Principles

### Target Metrics
- **Lines per function:** < 50 (ideal: 20-30)
- **Cyclomatic complexity:** < 10
- **Single Responsibility:** One clear purpose per function
- **Testability:** Can test each function in isolation

### Patterns to Apply
1. **Extract Method:** Pull related code into separate functions
2. **Extract Class:** Group related state and behavior
3. **Command Pattern:** For complex message handling
4. **Strategy Pattern:** For algorithmic variations
5. **Template Method:** For common workflows with variations

---

## Refactoring Plan

### Priority 1: wordclock.cpp::wordclock_loop()

**Current State** (196 lines):
```cpp
void wordclock_loop() {
    static bool animating = false;
    static unsigned long lastStepAt = 0;
    static int animStep = 0;
    static int lastRounded = -1;
    static unsigned long hetIsVisibleUntil = 0;
    static struct tm cachedTime = {};
    static bool haveTime = false;
    static unsigned long lastTimeFetchMs = 0;

    unsigned long nowMs = millis();

    // Early returns for disabled/incomplete states
    if (!clockEnabled) { /* ... */ }
    if (!setupState.isComplete()) { /* ... */ }

    // Time fetching logic (20 lines)
    if (!haveTime || (nowMs - lastTimeFetchMs) >= 1000UL) {
        // ... complex time caching logic
    }

    // Time rounding logic (10 lines)
    struct tm effective = timeinfo;
    if (displaySettings.isSellMode()) { /* ... */ }
    int minute = effective.tm_min;
    int rounded = (minute / 5) * 5;
    int extra = minute % 5;

    // Animation trigger logic (40 lines)
    if (rounded != lastRounded || g_forceAnim) {
        // ... complex animation setup
    }

    // Animation execution logic (50 lines)
    if (animating) {
        // ... frame-by-frame animation
    }

    // Static display logic (30 lines)
    // Not animating: update full phrase + extra minute LEDs
    std::vector<WordSegment> baseSegs = get_word_segments_with_keys(&effective);
    // ... complex display logic
}
```

**Problems:**
- 196 lines with multiple responsibilities
- 8 static variables (hidden state)
- Complex control flow
- Difficult to unit test
- Hard to understand flow

---

**Refactored Design:**

Create a `ClockDisplay` class to encapsulate state and logic:

**File:** `src/clock_display.h` (new)

```cpp
#ifndef CLOCK_DISPLAY_H
#define CLOCK_DISPLAY_H

#include <time.h>
#include <vector>
#include "time_mapper.h"
#include "display_settings.h"

/**
 * @brief Manages word clock display state and animation
 * 
 * Encapsulates the complex state machine for:
 * - Time synchronization and caching
 * - Animation state management
 * - HET IS visibility timing
 * - LED display updates
 */
class ClockDisplay {
public:
    ClockDisplay();
    
    /**
     * @brief Update display (call every 50ms)
     * @return true if clock is active, false if disabled/incomplete
     */
    bool update();
    
    /**
     * @brief Force animation for specific time (testing/preview)
     */
    void forceAnimationForTime(const struct tm& time);
    
    /**
     * @brief Reset display state
     */
    void reset();
    
private:
    // State management
    struct AnimationState {
        bool active = false;
        unsigned long lastStepAt = 0;
        int currentStep = 0;
        std::vector<std::vector<uint16_t>> frames;
    };
    
    struct TimeState {
        struct tm cached = {};
        bool valid = false;
        unsigned long lastFetchMs = 0;
        int lastRoundedMinute = -1;
    };
    
    struct HetIsState {
        unsigned long visibleUntil = 0;
    };
    
    AnimationState animation_;
    TimeState time_;
    HetIsState hetIs_;
    
    std::vector<WordSegment> lastSegments_;
    std::vector<WordSegment> targetSegments_;
    
    bool forceAnimation_ = false;
    struct tm forcedTime_ = {};
    
    // Extracted methods
    bool checkClockEnabled();
    bool updateTimeCache(unsigned long nowMs);
    void handleNoTime(unsigned long nowMs);
    
    struct DisplayTime {
        struct tm effective;
        int rounded;
        int extra;
    };
    DisplayTime prepareDisplayTime();
    
    void triggerAnimationIfNeeded(const DisplayTime& dt, unsigned long nowMs);
    void buildAnimationFrames(const DisplayTime& dt, unsigned long nowMs);
    void executeAnimationStep(unsigned long nowMs);
    void displayStaticTime(const DisplayTime& dt);
    
    bool shouldHideHetIs(unsigned long nowMs);
    void updateHetIsVisibility(unsigned long nowMs);
};

extern ClockDisplay clockDisplay;

#endif // CLOCK_DISPLAY_H
```

**File:** `src/clock_display.cpp` (new)

```cpp
#include "clock_display.h"
#include "led_controller.h"
#include "setup_state.h"
#include "night_mode.h"
#include "log.h"

ClockDisplay clockDisplay;

extern bool clockEnabled;
extern bool g_initialTimeSyncSucceeded;

ClockDisplay::ClockDisplay() {
    reset();
}

void ClockDisplay::reset() {
    animation_ = AnimationState();
    time_ = TimeState();
    hetIs_ = HetIsState();
    lastSegments_.clear();
    forceAnimation_ = false;
}

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
    
    // Execute animation or display static
    if (animation_.active) {
        executeAnimationStep(nowMs);
    } else {
        displayStaticTime(dt);
    }
    
    return true;
}

bool ClockDisplay::checkClockEnabled() {
    if (!clockEnabled) {
        animation_.active = false;
        showLeds({});
        resetNoTimeIndicator();  // From wordclock.cpp
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

bool ClockDisplay::updateTimeCache(unsigned long nowMs) {
    // Refresh cached time at most once per second
    if (!time_.valid || (nowMs - time_.lastFetchMs) >= 1000UL) {
        struct tm t = {};
        if (getLocalTime(&t)) {
            time_.cached = t;
            time_.valid = true;
            time_.lastFetchMs = nowMs;
            g_initialTimeSyncSucceeded = true;
            nightMode.updateFromTime(time_.cached);
            resetNoTimeIndicator();
            return true;
        } else if (!time_.valid) {
            // First failure
            return false;
        }
    }
    
    return time_.valid;
}

void ClockDisplay::handleNoTime(unsigned long nowMs) {
    static bool logged = false;
    if (!logged) {
        logWarn("❗ Unable to fetch time; showing no-time indicator");
        logged = true;
    }
    nightMode.markTimeInvalid();
    showNoTimeIndicator(nowMs);  // From wordclock.cpp
}

ClockDisplay::DisplayTime ClockDisplay::prepareDisplayTime() {
    DisplayTime dt;
    dt.effective = time_.cached;
    
    // Apply sell-mode override
    if (displaySettings.isSellMode()) {
        dt.effective.tm_hour = 10;
        dt.effective.tm_min = 47;
    }
    
    dt.rounded = (dt.effective.tm_min / 5) * 5;
    dt.extra = dt.effective.tm_min % 5;
    
    return dt;
}

void ClockDisplay::triggerAnimationIfNeeded(const DisplayTime& dt, unsigned long nowMs) {
    if (dt.rounded != time_.lastRoundedMinute || forceAnimation_) {
        time_.lastRoundedMinute = dt.rounded;
        
        struct tm animTime = forceAnimation_ ? forcedTime_ : dt.effective;
        buildAnimationFrames(DisplayTime{animTime, dt.rounded, dt.extra}, nowMs);
        
        forceAnimation_ = false;
    }
}

void ClockDisplay::buildAnimationFrames(const DisplayTime& dt, unsigned long nowMs) {
    targetSegments_ = get_word_segments_with_keys(&dt.effective);
    
    uint16_t hisSec = displaySettings.getHetIsDurationSec();
    stripHetIsIfDisabled(targetSegments_, hisSec);  // From wordclock.cpp
    
    bool animate = displaySettings.getAnimateWords();
    WordAnimationMode mode = displaySettings.getAnimationMode();
    
    if (animate) {
        bool hetIsVisible = hetIsCurrentlyVisible(hisSec, hetIs_.visibleUntil, nowMs);
        
        if (mode == WordAnimationMode::Smart && !lastSegments_.empty()) {
            buildSmartFrames(lastSegments_, targetSegments_, hetIsVisible, animation_.frames);
        } else {
            buildClassicFrames(targetSegments_, animation_.frames);
        }
        
        if (!animation_.frames.empty()) {
            animation_.active = true;
            animation_.currentStep = 0;
            animation_.lastStepAt = millis();
            hetIs_.visibleUntil = 0;  // Reset; will be set when animation completes
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
    
    if (animation_.currentStep == 0 || deltaMs >= 500) {
        if (animation_.currentStep < (int)animation_.frames.size()) {
            const auto& frame = animation_.frames[animation_.currentStep];
            
            // Logging (can be extracted)
            size_t prevSize = (animation_.currentStep == 0) ? 0 
                            : animation_.frames[animation_.currentStep - 1].size();
            logDebug(String("Anim step ") + (animation_.currentStep + 1) + "/" 
                    + animation_.frames.size() + " dt=" + deltaMs + "ms");
            
            showLeds(frame);
            animation_.currentStep++;
            animation_.lastStepAt = nowMs;
        }
        
        if (animation_.currentStep >= (int)animation_.frames.size()) {
            animation_.active = false;
            updateHetIsVisibility(nowMs);
            lastSegments_ = targetSegments_;
        }
    } else if (animation_.currentStep > 0 && animation_.currentStep <= (int)animation_.frames.size()) {
        // Re-display current frame (animation between steps)
        showLeds(animation_.frames[animation_.currentStep - 1]);
    }
}

void ClockDisplay::displayStaticTime(const DisplayTime& dt) {
    std::vector<WordSegment> baseSegs = get_word_segments_with_keys(&dt.effective);
    uint16_t hisSec = displaySettings.getHetIsDurationSec();
    stripHetIsIfDisabled(baseSegs, hisSec);
    
    std::vector<uint16_t> indices;
    bool hideHetIs = shouldHideHetIs(millis());
    
    for (const auto& seg : baseSegs) {
        if (hideHetIs && isHetIs(seg)) continue;
        indices.insert(indices.end(), seg.leds.begin(), seg.leds.end());
    }
    
    // Add extra minute LEDs
    for (int i = 0; i < dt.extra && i < 4; ++i) {
        indices.push_back(EXTRA_MINUTE_LEDS[i]);
    }
    
    showLeds(indices);
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

void ClockDisplay::forceAnimationForTime(const struct tm& time) {
    forcedTime_ = time;
    forceAnimation_ = true;
}
```

**Updated `wordclock.cpp`:**

```cpp
#include "clock_display.h"

void wordclock_setup() {
    initLeds();
    clockDisplay.reset();
    logInfo("Wordclock setup complete");
}

void wordclock_loop() {
    clockDisplay.update();
}

void wordclock_force_animation_for_time(struct tm* timeinfo) {
    if (!timeinfo) return;
    clockDisplay.forceAnimationForTime(*timeinfo);
}
```

**Benefits:**
- ✅ Functions now < 30 lines each
- ✅ Clear single responsibilities
- ✅ Testable in isolation
- ✅ State encapsulated in class
- ✅ Easier to understand flow
- ✅ Can mock for testing

---

### Priority 2: mqtt_client.cpp::handleMessage()

**Current State** (110 lines of if-else chains):
```cpp
static void handleMessage(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

    auto is = [&](const String& t){ return strcmp(topic, t.c_str())==0; };

    if (is(tLightSet)) {
        // 20 lines of JSON parsing
    } else if (is(tClockSet)) {
        // ...
    } else if (is(tAnimSet)) {
        // ...
    } 
    // ... 30+ more branches
}
```

**Problems:**
- 110 lines, single function
- 30+ if-else branches
- Cannot test individual handlers
- Adding new command requires modifying large function

---

**Refactored Design - Command Pattern:**

**File:** `src/mqtt_command_handler.h` (new)

```cpp
#ifndef MQTT_COMMAND_HANDLER_H
#define MQTT_COMMAND_HANDLER_H

#include <Arduino.h>
#include <functional>
#include <map>

/**
 * @brief Base class for MQTT command handlers
 */
class MqttCommandHandler {
public:
    virtual ~MqttCommandHandler() = default;
    virtual void handle(const String& payload) = 0;
};

/**
 * @brief Registry of MQTT command handlers
 */
class MqttCommandRegistry {
public:
    static MqttCommandRegistry& instance();
    
    void registerHandler(const String& topic, MqttCommandHandler* handler);
    void handleMessage(const String& topic, const String& payload);
    
    // Helper for simple lambda handlers
    void registerLambda(const String& topic, 
                       std::function<void(const String&)> handler);
    
private:
    MqttCommandRegistry() = default;
    std::map<String, MqttCommandHandler*> handlers_;
    std::map<String, std::function<void(const String&)>> lambdaHandlers_;
};

// Specific handlers
class LightCommandHandler : public MqttCommandHandler {
public:
    void handle(const String& payload) override;
};

class SwitchCommandHandler : public MqttCommandHandler {
public:
    SwitchCommandHandler(const String& name, 
                        std::function<void(bool)> setter,
                        std::function<void()> publisher);
    void handle(const String& payload) override;
    
private:
    String name_;
    std::function<void(bool)> setter_;
    std::function<void()> publisher_;
};

class NumberCommandHandler : public MqttCommandHandler {
public:
    NumberCommandHandler(int min, int max,
                        std::function<void(int)> setter,
                        std::function<void()> publisher);
    void handle(const String& payload) override;
    
private:
    int min_, max_;
    std::function<void(int)> setter_;
    std::function<void()> publisher_;
};

#endif // MQTT_COMMAND_HANDLER_H
```

**File:** `src/mqtt_command_handler.cpp` (new)

```cpp
#include "mqtt_command_handler.h"
#include "display_settings.h"
#include "night_mode.h"
#include "led_state.h"
#include "log.h"
#include <ArduinoJson.h>

extern DisplaySettings displaySettings;
extern bool clockEnabled;

MqttCommandRegistry& MqttCommandRegistry::instance() {
    static MqttCommandRegistry registry;
    return registry;
}

void MqttCommandRegistry::registerHandler(const String& topic, MqttCommandHandler* handler) {
    handlers_[topic] = handler;
}

void MqttCommandRegistry::registerLambda(const String& topic, 
                                        std::function<void(const String&)> handler) {
    lambdaHandlers_[topic] = handler;
}

void MqttCommandRegistry::handleMessage(const String& topic, const String& payload) {
    // Try direct handler first
    auto it = handlers_.find(topic);
    if (it != handlers_.end()) {
        it->second->handle(payload);
        return;
    }
    
    // Try lambda handler
    auto lit = lambdaHandlers_.find(topic);
    if (lit != lambdaHandlers_.end()) {
        lit->second(payload);
        return;
    }
    
    logWarn(String("Unhandled MQTT topic: ") + topic);
}

// Light command handler
void LightCommandHandler::handle(const String& payload) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        logWarn("Light command JSON parse error");
        return;
    }
    
    if (doc["state"].is<const char*>()) {
        const char* st = doc["state"];
        clockEnabled = (strcmp(st, "ON") == 0);
    }
    
    if (doc["brightness"].is<int>()) {
        int br = doc["brightness"].as<int>();
        br = constrain(br, 0, 255);
        ledState.setBrightness(br);
    }
    
    if (doc["color"].is<JsonObject>()) {
        uint8_t r = doc["color"]["r"] | 0;
        uint8_t g = doc["color"]["g"] | 0;
        uint8_t b = doc["color"]["b"] | 0;
        ledState.setRGB(r, g, b);
    }
    
    publishLightState();  // Defined in mqtt_client.cpp
}

// Switch command handler
SwitchCommandHandler::SwitchCommandHandler(const String& name,
                                          std::function<void(bool)> setter,
                                          std::function<void()> publisher)
    : name_(name), setter_(setter), publisher_(publisher) {}

void SwitchCommandHandler::handle(const String& payload) {
    bool on = (payload == "ON" || payload == "on" || payload == "1");
    setter_(on);
    publisher_();
}

// Number command handler
NumberCommandHandler::NumberCommandHandler(int min, int max,
                                          std::function<void(int)> setter,
                                          std::function<void()> publisher)
    : min_(min), max_(max), setter_(setter), publisher_(publisher) {}

void NumberCommandHandler::handle(const String& payload) {
    int value = payload.toInt();
    value = constrain(value, min_, max_);
    setter_(value);
    publisher_();
}
```

**Updated `mqtt_client.cpp`:**

```cpp
#include "mqtt_command_handler.h"

static void initCommandHandlers() {
    auto& registry = MqttCommandRegistry::instance();
    
    // Light (complex JSON)
    registry.registerHandler(tLightSet, new LightCommandHandler());
    
    // Simple switches
    registry.registerHandler(tAnimSet, new SwitchCommandHandler(
        "animate",
        [](bool on) { displaySettings.setAnimateWords(on); },
        []() { publishSwitch(tAnimState, displaySettings.getAnimateWords()); }
    ));
    
    registry.registerHandler(tAutoUpdSet, new SwitchCommandHandler(
        "auto_update",
        [](bool on) { displaySettings.setAutoUpdate(on); },
        []() { publishSwitch(tAutoUpdState, displaySettings.getAutoUpdate()); }
    ));
    
    // Number handlers
    registry.registerHandler(tHetIsSet, new NumberCommandHandler(
        0, 360,
        [](int v) { displaySettings.setHetIsDurationSec((uint16_t)v); },
        []() { publishNumber(tHetIsState, displaySettings.getHetIsDurationSec()); }
    ));
    
    // Simple commands (no response)
    registry.registerLambda(tRestartCmd, [](const String&) {
        ESP.restart();
    });
    
    registry.registerLambda(tSeqCmd, [](const String&) {
        extern StartupSequence startupSequence;
        startupSequence.start();
    });
    
    // ... register remaining handlers
}

static void handleMessage(char* topic, byte* payload, unsigned int length) {
    String topicStr(topic);
    String payloadStr;
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    
    MqttCommandRegistry::instance().handleMessage(topicStr, payloadStr);
}
```

**Benefits:**
- ✅ Main handler now ~10 lines
- ✅ Each command handler testable in isolation
- ✅ Easy to add new commands
- ✅ Clear separation of concerns
- ✅ Type-safe command handling

---

### Priority 3: mqtt_client.cpp::publishDiscovery()

**Current State** (200+ lines of repetitive code):
```cpp
static void publishDiscovery() {
    mqtt.setBufferSize(1024);
    String nodeId = uniqId;
    String devIds = String("{\"ids\":[\"") + nodeId + "\"]}";

    auto pubCfg = [&](const String& comp, const String& objId, JsonDocument& cfg){
        String topic = String(g_mqttCfg.discoveryPrefix) + "/" + comp + "/" + objId + "/config";
        String out; serializeJson(cfg, out);
        mqtt.publish(topic.c_str(), out.c_str(), true);
    };

    // Light entity (20 lines)
    {
        JsonDocument cfg;
        cfg["name"] = CLOCK_NAME;
        // ... many fields
        pubCfg("light", nodeId + "_light", cfg);
    }

    // Switches (30+ lines of repetition)
    auto publishSwitch = [&](const char* name, const String& st, const String& set, const String& id){
        // ... repetitive code
    };
    publishSwitch("Animate words", tAnimState, tAnimSet, nodeId + String("_anim"));
    publishSwitch("Auto update", tAutoUpdState, tAutoUpdSet, nodeId + String("_autoupd"));
    // ... many more

    // ... 150 more lines
}
```

---

**Refactored Design - Builder Pattern:**

**File:** `src/mqtt_discovery_builder.h` (new)

```cpp
#ifndef MQTT_DISCOVERY_BUILDER_H
#define MQTT_DISCOVERY_BUILDER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

/**
 * @brief Builder for Home Assistant MQTT Discovery messages
 */
class MqttDiscoveryBuilder {
public:
    MqttDiscoveryBuilder(PubSubClient& mqtt, 
                        const String& discoveryPrefix,
                        const String& nodeId,
                        const String& baseTopic,
                        const String& availTopic);
    
    // Entity builders
    void addLight(const String& stateTopic, const String& cmdTopic);
    void addSwitch(const String& name, const String& uniqueId,
                   const String& stateTopic, const String& cmdTopic);
    void addNumber(const String& name, const String& uniqueId,
                   const String& stateTopic, const String& cmdTopic,
                   int min, int max, int step, const String& unit = "");
    void addSelect(const String& name, const String& uniqueId,
                   const String& stateTopic, const String& cmdTopic,
                   const std::vector<String>& options);
    void addBinarySensor(const String& name, const String& uniqueId,
                        const String& stateTopic);
    void addButton(const String& name, const String& uniqueId,
                   const String& cmdTopic);
    void addSensor(const String& name, const String& uniqueId,
                   const String& stateTopic,
                   const String& unit = "",
                   const String& deviceClass = "");
    void addText(const String& name, const String& uniqueId,
                 const String& stateTopic, const String& cmdTopic,
                 int minLen, int maxLen, const String& pattern = "");
    
    // Publish all configured entities
    void publish();
    
private:
    struct Entity {
        String component;
        String objectId;
        JsonDocument config;
    };
    
    void addDeviceInfo(JsonDocument& doc);
    void publishEntity(const Entity& entity);
    
    PubSubClient& mqtt_;
    String discoveryPrefix_;
    String nodeId_;
    String baseTopic_;
    String availTopic_;
    std::vector<Entity> entities_;
};

#endif // MQTT_DISCOVERY_BUILDER_H
```

**Updated `mqtt_client.cpp`:**

```cpp
#include "mqtt_discovery_builder.h"

static void publishDiscovery() {
    mqtt.setBufferSize(1024);
    
    MqttDiscoveryBuilder builder(mqtt, g_mqttCfg.discoveryPrefix, 
                                 uniqId, base, availTopic);
    
    // Light
    builder.addLight(tLightState, tLightSet);
    
    // Switches
    builder.addSwitch("Animate words", uniqId + "_anim", tAnimState, tAnimSet);
    builder.addSwitch("Auto update", uniqId + "_autoupd", tAutoUpdState, tAutoUpdSet);
    builder.addSwitch("Sell mode", uniqId + "_sell", tSellState, tSellSet);
    builder.addSwitch("Night mode enabled", uniqId + "_night_enabled", 
                     tNightEnabledState, tNightEnabledSet);
    
    // Select entities
    builder.addSelect("Night mode effect", uniqId + "_night_effect",
                     tNightEffectState, tNightEffectSet,
                     {"DIM", "OFF"});
    builder.addSelect("Night mode override", uniqId + "_night_override",
                     tNightOverrideState, tNightOverrideSet,
                     {"AUTO", "ON", "OFF"});
    builder.addSelect("Log level", uniqId + "_loglevel",
                     tLogLvlState, tLogLvlSet,
                     {"DEBUG", "INFO", "WARN", "ERROR"});
    
    // Numbers
    builder.addNumber("Night mode dim %", uniqId + "_night_dim",
                     tNightDimState, tNightDimSet,
                     0, 100, 1, "%");
    builder.addNumber("'HET IS' seconds", uniqId + "_hetis",
                     tHetIsState, tHetIsSet,
                     0, 360, 1);
    
    // Binary sensors
    builder.addBinarySensor("Night mode active", uniqId + "_night_active",
                           tNightActiveState);
    
    // Buttons
    builder.addButton("Restart", uniqId + "_restart", tRestartCmd);
    builder.addButton("Start sequence", uniqId + "_sequence", tSeqCmd);
    builder.addButton("Check for update", uniqId + "_update", tUpdateCmd);
    
    // Sensors
    builder.addSensor("Firmware Version", uniqId + "_version", tVersion);
    builder.addSensor("IP Address", uniqId + "_ip", tIp);
    builder.addSensor("WiFi RSSI", uniqId + "_rssi", tRssi, "dBm", "signal_strength");
    builder.addSensor("Free Heap", uniqId + "_heap", tHeap, "bytes");
    
    // Text entities (time inputs)
    builder.addText("Night mode start", uniqId + "_night_start",
                   tNightStartState, tNightStartSet,
                   5, 5, "^([01][0-9]|2[0-3]):[0-5][0-9]$");
    builder.addText("Night mode end", uniqId + "_night_end",
                   tNightEndState, tNightEndSet,
                   5, 5, "^([01][0-9]|2[0-3]):[0-5][0-9]$");
    
    // Publish all
    builder.publish();
}
```

**Benefits:**
- ✅ Main function now ~30 lines (from 200+)
- ✅ Builder reusable for other projects
- ✅ Clear entity definition
- ✅ Easy to add new entities
- ✅ Testable builder logic

---

## Testing Strategy

### Unit Tests for Refactored Code

**Test ClockDisplay class:**
```cpp
TEST(ClockDisplayTest, UpdateWithNoTime) {
    ClockDisplay display;
    // Mock getLocalTime to return false
    ASSERT_FALSE(display.update());
}

TEST(ClockDisplayTest, AnimationTriggerOnMinuteChange) {
    ClockDisplay display;
    // Setup mocks for time 12:00
    display.update();
    
    // Change time to 12:05
    // Verify animation triggered
}
```

**Test Command Handlers:**
```cpp
TEST(MqttCommandTest, SwitchHandlerSetsValue) {
    bool called = false;
    auto setter = [&](bool val) { called = true; };
    auto publisher = []() {};
    
    SwitchCommandHandler handler("test", setter, publisher);
    handler.handle("ON");
    
    ASSERT_TRUE(called);
}
```

---

## Implementation Timeline

### Week 1: ClockDisplay Refactoring
- **Day 1:** Design and create ClockDisplay class structure
- **Day 2:** Extract time management logic
- **Day 3:** Extract animation logic
- **Day 4:** Testing and integration
- **Day 5:** Documentation

### Week 2: MQTT Refactoring
- **Day 1:** Design command handler pattern
- **Day 2:** Implement handlers and registry
- **Day 3:** Design discovery builder
- **Day 4:** Implement and test builder
- **Day 5:** Integration and testing

---

## Success Criteria

- ✅ All functions < 50 lines
- ✅ Cyclomatic complexity < 10
- ✅ 80% test coverage on refactored code
- ✅ No functionality regressions
- ✅ Code review approved
- ✅ Documentation complete

---

**Plan Version:** 1.0  
**Last Updated:** January 4, 2026  
**Status:** Ready for Implementation
