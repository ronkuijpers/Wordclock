# Implementation Plan: Add Unit Tests for Core Logic

**Priority:** HIGH  
**Estimated Effort:** 16 hours  
**Complexity:** Medium  
**Risk:** Low  
**Impact:** HIGH - Enables confident refactoring, catches regressions, improves code quality

---

## Problem Statement

The current codebase has **zero automated test coverage**, making:

1. **Refactoring Risky:** No safety net for code changes
2. **Regression Detection:** Bugs can slip through unnoticed
3. **Documentation Gap:** Tests serve as executable documentation
4. **Onboarding Difficult:** New developers lack reference examples
5. **CI/CD Blocked:** Cannot implement automated quality gates

**Current State:**
- No unit tests
- No integration tests
- Manual testing only
- High risk of introducing bugs during refactoring

**Target State:**
- 80% coverage for critical logic (time_mapper, led_controller)
- 70% coverage for business logic (settings, night_mode)
- 50% coverage for integration points (MQTT, network)
- Automated testing in CI pipeline

---

## Technology Selection

### Framework: Google Test (gtest) + PlatformIO Native Testing

**Why Google Test:**
- ✅ Industry standard C++ testing framework
- ✅ Excellent PlatformIO integration
- ✅ Rich assertion library
- ✅ Mock/stub support (gmock)
- ✅ Cross-platform (test on PC, not ESP32)
- ✅ Fast execution (tests run in milliseconds)

**Why PlatformIO Native:**
- ✅ No hardware required for unit tests
- ✅ Faster test execution
- ✅ Better debugging tools
- ✅ CI/CD friendly
- ✅ Can test hardware-specific code with mocks

**Alternative Considered:**
- Unity (simpler but less powerful)
- Catch2 (good alternative, less mature in PlatformIO)

---

## Project Structure

### New Directory Layout

```
workspace/
├── src/                        # Production code
│   ├── time_mapper.cpp
│   ├── led_controller.cpp
│   └── ...
├── test/                       # Test code
│   ├── test_time_mapper/
│   │   ├── test_main.cpp
│   │   ├── test_time_to_leds.cpp
│   │   ├── test_word_segments.cpp
│   │   └── README.md
│   ├── test_led_controller/
│   │   ├── test_main.cpp
│   │   ├── test_led_state.cpp
│   │   └── README.md
│   ├── test_night_mode/
│   │   ├── test_main.cpp
│   │   ├── test_schedule.cpp
│   │   └── README.md
│   ├── test_settings/
│   │   ├── test_main.cpp
│   │   ├── test_display_settings.cpp
│   │   └── README.md
│   ├── mocks/                  # Mock implementations
│   │   ├── mock_preferences.h
│   │   ├── mock_grid_layout.h
│   │   └── mock_time.h
│   ├── helpers/                # Test utilities
│   │   ├── test_fixtures.h
│   │   └── test_utils.h
│   └── README.md               # Testing guide
├── platformio.ini              # Add test environments
└── .github/
    └── workflows/
        └── tests.yml           # CI configuration
```

---

## PlatformIO Configuration

### Update platformio.ini

```ini
[platformio]
extra_configs = upload_params.ini
data_dir = data

# Existing production environment
[env:esp32dev]
platform = espressif32@6.4.0
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps = 
    https://github.com/tzapu/WiFiManager.git
    adafruit/Adafruit NeoPixel @ ^1.12.1
    bblanchon/ArduinoJson@^7.4.1
    knolleary/PubSubClient@^2.8
extra_scripts = tools/full_upload.py, tools/generate_build_info.py

# NEW: Native testing environment
[env:native]
platform = native
test_framework = googletest
build_flags = 
    -std=c++17
    -DPIO_UNIT_TESTING
    -DNATIVE_BUILD
lib_deps =
    bblanchon/ArduinoJson@^7.4.1
# Note: Only include non-hardware dependencies

# NEW: ESP32 integration testing (on-hardware)
[env:esp32_test]
platform = espressif32@6.4.0
board = esp32dev
framework = arduino
test_framework = unity
build_flags = 
    -DPIO_UNIT_TESTING
lib_deps = 
    ${env:esp32dev.lib_deps}
test_filter = test_integration_*
```

---

## Phase 1: Test Infrastructure Setup

### Step 1: Create Mock Framework

**File:** `test/mocks/mock_preferences.h`

```cpp
#ifndef MOCK_PREFERENCES_H
#define MOCK_PREFERENCES_H

#include <map>
#include <string>
#include <Arduino.h>  // For String class

/**
 * @brief Mock implementation of ESP32 Preferences for unit testing
 * 
 * Provides in-memory storage that mimics Preferences API without
 * requiring actual NVS flash operations.
 */
class Preferences {
public:
    Preferences() : readOnly_(false) {}
    
    bool begin(const char* name, bool readOnly = false) {
        namespace_ = name;
        readOnly_ = readOnly;
        return true;
    }
    
    void end() {
        namespace_ = "";
    }
    
    void clear() {
        if (!readOnly_) {
            storage_[namespace_].clear();
        }
    }
    
    bool remove(const char* key) {
        if (readOnly_) return false;
        auto it = storage_[namespace_].find(key);
        if (it != storage_[namespace_].end()) {
            storage_[namespace_].erase(it);
            return true;
        }
        return false;
    }
    
    // Getters
    uint8_t getUChar(const char* key, uint8_t defaultValue = 0) {
        auto& ns = storage_[namespace_];
        auto it = ns.find(key);
        return (it != ns.end()) ? std::stoi(it->second) : defaultValue;
    }
    
    uint16_t getUShort(const char* key, uint16_t defaultValue = 0) {
        auto& ns = storage_[namespace_];
        auto it = ns.find(key);
        return (it != ns.end()) ? std::stoi(it->second) : defaultValue;
    }
    
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0) {
        auto& ns = storage_[namespace_];
        auto it = ns.find(key);
        return (it != ns.end()) ? std::stoul(it->second) : defaultValue;
    }
    
    bool getBool(const char* key, bool defaultValue = false) {
        auto& ns = storage_[namespace_];
        auto it = ns.find(key);
        return (it != ns.end()) ? (it->second == "1") : defaultValue;
    }
    
    String getString(const char* key, const String& defaultValue = "") {
        auto& ns = storage_[namespace_];
        auto it = ns.find(key);
        return (it != ns.end()) ? String(it->second.c_str()) : defaultValue;
    }
    
    // Setters
    size_t putUChar(const char* key, uint8_t value) {
        if (readOnly_) return 0;
        storage_[namespace_][key] = std::to_string(value);
        return sizeof(value);
    }
    
    size_t putUShort(const char* key, uint16_t value) {
        if (readOnly_) return 0;
        storage_[namespace_][key] = std::to_string(value);
        return sizeof(value);
    }
    
    size_t putUInt(const char* key, uint32_t value) {
        if (readOnly_) return 0;
        storage_[namespace_][key] = std::to_string(value);
        return sizeof(value);
    }
    
    size_t putBool(const char* key, bool value) {
        if (readOnly_) return 0;
        storage_[namespace_][key] = value ? "1" : "0";
        return sizeof(value);
    }
    
    size_t putString(const char* key, const String& value) {
        if (readOnly_) return 0;
        storage_[namespace_][std::string(key)] = value.c_str();
        return value.length();
    }
    
    bool isKey(const char* key) {
        auto& ns = storage_[namespace_];
        return ns.find(key) != ns.end();
    }
    
    // Test helpers
    static void reset() {
        storage_.clear();
    }
    
    static size_t getStorageSize() {
        size_t total = 0;
        for (const auto& ns : storage_) {
            total += ns.second.size();
        }
        return total;
    }

private:
    std::string namespace_;
    bool readOnly_;
    
    // Global storage: namespace -> (key -> value)
    static std::map<std::string, std::map<std::string, std::string>> storage_;
};

// Define static member
std::map<std::string, std::map<std::string, std::string>> Preferences::storage_;

#endif // MOCK_PREFERENCES_H
```

---

**File:** `test/mocks/mock_grid_layout.h`

```cpp
#ifndef MOCK_GRID_LAYOUT_H
#define MOCK_GRID_LAYOUT_H

#include <vector>
#include <cstring>
#include "../../src/wordposition.h"
#include "../../src/grid_layout.h"

// Simple test grid for unit testing
const char* const LETTER_GRID_TEST[] = {
    "HETAISAVIJF",
    "TIENBTZVOOR",
    "OVERMEKWART",
    "HALFSPWOVER",
    "VOORATHALF",
    nullptr
};

// Test word definitions
const WordPosition WORDS_TEST[] = {
    {"HET", {0, 1, 2, 0}},
    {"IS", {3, 4, 0}},
    {"VIJF_M", {6, 7, 8, 9, 0}},
    {"TIEN_M", {11, 12, 13, 14, 0}},
    {"KWART", {30, 31, 32, 33, 34, 0}},
    {"OVER", {22, 23, 24, 25, 0}},
    {"VOOR", {17, 18, 19, 20, 0}},
    {"HALF", {35, 36, 37, 38, 0}},
    {"EEN", {60, 61, 62, 0}},
    {"TWEE", {70, 71, 72, 73, 0}},
    // ... minimal set for testing
};

const size_t WORDS_TEST_COUNT = sizeof(WORDS_TEST) / sizeof(WORDS_TEST[0]);

const uint16_t EXTRA_MINUTES_TEST[] = {100, 101, 102, 103};
const size_t EXTRA_MINUTES_TEST_COUNT = 4;

// Helper to initialize test grid
inline void setupTestGrid() {
    LETTER_GRID = LETTER_GRID_TEST;
    ACTIVE_WORDS = WORDS_TEST;
    ACTIVE_WORD_COUNT = WORDS_TEST_COUNT;
    EXTRA_MINUTE_LEDS = EXTRA_MINUTES_TEST;
    EXTRA_MINUTE_LED_COUNT = EXTRA_MINUTES_TEST_COUNT;
}

#endif // MOCK_GRID_LAYOUT_H
```

---

**File:** `test/mocks/mock_time.h`

```cpp
#ifndef MOCK_TIME_H
#define MOCK_TIME_H

#include <time.h>

/**
 * @brief Helper to create test time structures
 */
struct tm createTestTime(int hour, int minute, int second = 0) {
    struct tm t = {};
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    t.tm_mday = 1;
    t.tm_mon = 0;
    t.tm_year = 2024 - 1900;
    return t;
}

#endif // MOCK_TIME_H
```

---

### Step 2: Create Test Helpers

**File:** `test/helpers/test_utils.h`

```cpp
#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <gtest/gtest.h>
#include <vector>
#include <algorithm>

// Helper to compare LED index vectors
inline bool ledVectorsEqual(const std::vector<uint16_t>& a, 
                            const std::vector<uint16_t>& b) {
    if (a.size() != b.size()) return false;
    
    // Sort both for comparison (order might not matter)
    std::vector<uint16_t> sortedA = a;
    std::vector<uint16_t> sortedB = b;
    std::sort(sortedA.begin(), sortedA.end());
    std::sort(sortedB.begin(), sortedB.end());
    
    return sortedA == sortedB;
}

// Custom assertion for LED vectors
#define ASSERT_LED_VECTOR_EQ(expected, actual) \
    ASSERT_TRUE(ledVectorsEqual(expected, actual)) \
        << "LED vectors differ\nExpected size: " << expected.size() \
        << "\nActual size: " << actual.size()

// Print LED vector for debugging
inline std::string ledVectorToString(const std::vector<uint16_t>& leds) {
    std::string result = "[";
    for (size_t i = 0; i < leds.size(); i++) {
        result += std::to_string(leds[i]);
        if (i < leds.size() - 1) result += ", ";
    }
    result += "]";
    return result;
}

#endif // TEST_UTILS_H
```

---

## Phase 2: Core Logic Tests

### Test Suite 1: time_mapper.cpp

**File:** `test/test_time_mapper/test_main.cpp`

```cpp
#include <gtest/gtest.h>

// Include mocks before production code
#include "../mocks/mock_grid_layout.h"
#include "../mocks/mock_time.h"
#include "../helpers/test_utils.h"

// Include production code
#include "../../src/time_mapper.cpp"

// Test fixture
class TimeMapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        setupTestGrid();
    }
};

// Test: Exact hour (e.g., 12:00)
TEST_F(TimeMapperTest, ExactHour_12_00) {
    struct tm time = createTestTime(12, 0);
    auto leds = get_led_indices_for_time(&time);
    
    // Should contain: HET, IS, TWAALF, UUR
    // Expected LED indices based on test grid
    std::vector<uint16_t> expected = {
        0, 1, 2,      // HET
        3, 4,         // IS
        // Add expected indices for TWAALF and UUR
    };
    
    ASSERT_FALSE(leds.empty());
    // More specific assertions based on test grid layout
}

// Test: 5 minutes past (e.g., 12:05)
TEST_F(TimeMapperTest, FiveMinutesPast_12_05) {
    struct tm time = createTestTime(12, 5);
    auto leds = get_led_indices_for_time(&time);
    
    // Should contain: HET, IS, VIJF, OVER, TWAALF
    ASSERT_FALSE(leds.empty());
    
    // Verify "VIJF_M" is included
    auto vijfLeds = get_leds_for_word("VIJF_M");
    for (auto led : vijfLeds) {
        ASSERT_TRUE(std::find(leds.begin(), leds.end(), led) != leds.end())
            << "LED " << led << " from VIJF not found";
    }
}

// Test: Half past (e.g., 12:30)
TEST_F(TimeMapperTest, HalfPast_12_30) {
    struct tm time = createTestTime(12, 30);
    auto leds = get_led_indices_for_time(&time);
    
    // Should contain: HET, IS, HALF, EEN (next hour!)
    ASSERT_FALSE(leds.empty());
}

// Test: Extra minute LEDs (e.g., 12:32 = 30 + 2 extra)
TEST_F(TimeMapperTest, ExtraMinuteLEDs_12_32) {
    struct tm time = createTestTime(12, 32);
    auto leds = get_led_indices_for_time(&time);
    
    // Should include 2 extra minute LEDs
    int extraCount = 0;
    for (size_t i = 0; i < EXTRA_MINUTE_LED_COUNT && i < 4; i++) {
        if (std::find(leds.begin(), leds.end(), EXTRA_MINUTE_LEDS[i]) != leds.end()) {
            extraCount++;
        }
    }
    
    ASSERT_EQ(2, extraCount) << "Expected 2 extra minute LEDs for :32";
}

// Test: Midnight wraparound
TEST_F(TimeMapperTest, MidnightWraparound_23_55) {
    struct tm time = createTestTime(23, 55);
    auto leds = get_led_indices_for_time(&time);
    
    // 23:55 = "vijf voor twaalf" (5 to 12)
    ASSERT_FALSE(leds.empty());
}

// Test: Word segments with keys
TEST_F(TimeMapperTest, WordSegmentsWithKeys_12_15) {
    struct tm time = createTestTime(12, 15);
    auto segments = get_word_segments_with_keys(&time);
    
    // Should have multiple segments
    ASSERT_GT(segments.size(), 0);
    
    // First two should be HET and IS
    ASSERT_STREQ("HET", segments[0].key);
    ASSERT_STREQ("IS", segments[1].key);
    
    // Should include KWART
    bool hasKwart = false;
    for (const auto& seg : segments) {
        if (strcmp(seg.key, "KWART") == 0) {
            hasKwart = true;
            break;
        }
    }
    ASSERT_TRUE(hasKwart) << "KWART segment not found for 12:15";
}

// Test: All 5-minute intervals
TEST_F(TimeMapperTest, AllFiveMinuteIntervals) {
    for (int hour = 0; hour < 24; hour++) {
        for (int minute = 0; minute < 60; minute += 5) {
            struct tm time = createTestTime(hour, minute);
            auto leds = get_led_indices_for_time(&time);
            
            ASSERT_FALSE(leds.empty()) 
                << "No LEDs for " << hour << ":" << minute;
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

---

### Test Suite 2: led_controller.cpp

**File:** `test/test_led_controller/test_main.cpp`

```cpp
#include <gtest/gtest.h>

// Mock NeoPixel (simplified)
class Adafruit_NeoPixel {
public:
    void begin() { initialized_ = true; }
    void clear() { for (auto& p : pixels_) p = 0; }
    void show() { showCalled_ = true; }
    void setPixelColor(uint16_t n, uint32_t c) {
        if (n < pixels_.size()) pixels_[n] = c;
    }
    void setBrightness(uint8_t b) { brightness_ = b; }
    uint16_t numPixels() const { return pixels_.size(); }
    uint32_t Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0) {
        return ((uint32_t)w << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
    void updateLength(uint16_t n) { pixels_.resize(n, 0); }
    void updateType(uint16_t t) { type_ = t; }
    void setPin(uint8_t p) { pin_ = p; }
    
    // Test helpers
    bool wasShowCalled() const { return showCalled_; }
    void resetShowCalled() { showCalled_ = false; }
    uint32_t getPixelColor(uint16_t n) const {
        return n < pixels_.size() ? pixels_[n] : 0;
    }
    
private:
    std::vector<uint32_t> pixels_{100, 0};  // 100 LEDs default
    uint8_t brightness_ = 255;
    bool initialized_ = false;
    bool showCalled_ = false;
    uint16_t type_ = 0;
    uint8_t pin_ = 0;
};

#define NEO_GRBW 0
#define NEO_KHZ800 0

// Include production code with mocks
#include "../../src/led_controller.cpp"

class LedControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        setupTestGrid();
        initLeds();
        test_clearLastShownLeds();
    }
};

TEST_F(LedControllerTest, InitializesCorrectly) {
    // Init should succeed
    initLeds();
    SUCCEED();
}

TEST_F(LedControllerTest, ShowsSingleLED) {
    std::vector<uint16_t> leds = {5};
    showLeds(leds);
    
    auto shown = test_getLastShownLeds();
    ASSERT_EQ(1, shown.size());
    ASSERT_EQ(5, shown[0]);
}

TEST_F(LedControllerTest, ShowsMultipleLEDs) {
    std::vector<uint16_t> leds = {1, 2, 3, 10, 20};
    showLeds(leds);
    
    auto shown = test_getLastShownLeds();
    ASSERT_EQ(5, shown.size());
    ASSERT_LED_VECTOR_EQ(leds, shown);
}

TEST_F(LedControllerTest, ClearsLEDsWhenEmpty) {
    // First show some LEDs
    showLeds({1, 2, 3});
    
    // Then clear
    showLeds({});
    
    auto shown = test_getLastShownLeds();
    ASSERT_EQ(0, shown.size());
}

TEST_F(LedControllerTest, IgnoresOutOfBoundsLEDs) {
    // Assuming max 100 LEDs in test
    std::vector<uint16_t> leds = {1, 2, 999};  // 999 is out of bounds
    
    // Should not crash
    ASSERT_NO_THROW(showLeds(leds));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

---

### Test Suite 3: night_mode.cpp

**File:** `test/test_night_mode/test_main.cpp`

```cpp
#include <gtest/gtest.h>
#include "../mocks/mock_preferences.h"
#include "../mocks/mock_time.h"

// Mock dependencies
#define logInfo(x) 
#define logWarn(x)
#define mqtt_publish_state(x)

// Include production code
#include "../../src/night_mode.cpp"

class NightModeTest : public ::testing::Test {
protected:
    void SetUp() override {
        Preferences::reset();
        nightMode.begin();
    }
    
    void TearDown() override {
        Preferences::reset();
    }
};

TEST_F(NightModeTest, InitializesWithDefaults) {
    ASSERT_FALSE(nightMode.isEnabled());
    ASSERT_EQ(NightModeEffect::Dim, nightMode.getEffect());
    ASSERT_EQ(20, nightMode.getDimPercent());
    ASSERT_EQ(22 * 60, nightMode.getStartMinutes());
    ASSERT_EQ(6 * 60, nightMode.getEndMinutes());
}

TEST_F(NightModeTest, EnableDisable) {
    nightMode.setEnabled(true);
    ASSERT_TRUE(nightMode.isEnabled());
    
    nightMode.setEnabled(false);
    ASSERT_FALSE(nightMode.isEnabled());
}

TEST_F(NightModeTest, ScheduleCalculation_SimpleRange) {
    // 22:00 to 06:00 schedule
    nightMode.setEnabled(true);
    nightMode.setSchedule(22 * 60, 6 * 60);
    
    // Test times within range
    struct tm time = createTestTime(23, 30);  // 23:30
    nightMode.updateFromTime(time);
    ASSERT_TRUE(nightMode.isActive());
}

TEST_F(NightModeTest, ScheduleCalculation_WrapAroundMidnight) {
    nightMode.setEnabled(true);
    nightMode.setSchedule(22 * 60, 6 * 60);  // 22:00 to 06:00
    
    // Test time after midnight but before end
    struct tm time = createTestTime(2, 0);  // 02:00
    nightMode.updateFromTime(time);
    ASSERT_TRUE(nightMode.isActive());
    
    // Test time after end
    time = createTestTime(8, 0);  // 08:00
    nightMode.updateFromTime(time);
    ASSERT_FALSE(nightMode.isActive());
}

TEST_F(NightModeTest, ScheduleCalculation_NoWrapAround) {
    nightMode.setEnabled(true);
    nightMode.setSchedule(9 * 60, 17 * 60);  // 09:00 to 17:00
    
    // Within range
    struct tm time = createTestTime(12, 0);
    nightMode.updateFromTime(time);
    ASSERT_TRUE(nightMode.isActive());
    
    // Before range
    time = createTestTime(8, 0);
    nightMode.updateFromTime(time);
    ASSERT_FALSE(nightMode.isActive());
    
    // After range
    time = createTestTime(18, 0);
    nightMode.updateFromTime(time);
    ASSERT_FALSE(nightMode.isActive());
}

TEST_F(NightModeTest, DimPercentageApplication) {
    nightMode.setDimPercent(50);
    
    // 50% of 100 = 50
    ASSERT_EQ(50, nightMode.applyToBrightness(100));
    
    // 50% of 200 = 100
    ASSERT_EQ(100, nightMode.applyToBrightness(200));
    
    // 50% of 10 = 5
    ASSERT_EQ(5, nightMode.applyToBrightness(10));
}

TEST_F(NightModeTest, DimPercentageRoundingUpFromZero) {
    nightMode.setDimPercent(10);  // 10%
    
    // 10% of 5 = 0.5, should round up to 1
    uint8_t result = nightMode.applyToBrightness(5);
    ASSERT_GE(result, 1) << "Should not dim to zero when both base and percent > 0";
}

TEST_F(NightModeTest, EffectOff_ReturnsZeroBrightness) {
    nightMode.setEffect(NightModeEffect::Off);
    nightMode.setEnabled(true);
    
    struct tm time = createTestTime(23, 0);  // Within default schedule
    nightMode.updateFromTime(time);
    
    ASSERT_EQ(0, nightMode.applyToBrightness(255));
}

TEST_F(NightModeTest, OverrideForceOn) {
    nightMode.setEnabled(true);
    nightMode.setSchedule(22 * 60, 6 * 60);
    nightMode.setOverride(NightModeOverride::ForceOn);
    
    // Even outside schedule, should be active
    struct tm time = createTestTime(12, 0);  // Daytime
    nightMode.updateFromTime(time);
    ASSERT_TRUE(nightMode.isActive());
}

TEST_F(NightModeTest, OverrideForceOff) {
    nightMode.setEnabled(true);
    nightMode.setSchedule(22 * 60, 6 * 60);
    nightMode.setOverride(NightModeOverride::ForceOff);
    
    // Even within schedule, should not be active
    struct tm time = createTestTime(23, 0);  // Nighttime
    nightMode.updateFromTime(time);
    ASSERT_FALSE(nightMode.isActive());
}

TEST_F(NightModeTest, TimeStringParsing_Valid) {
    uint16_t minutes;
    
    ASSERT_TRUE(NightMode::parseTimeString("12:30", minutes));
    ASSERT_EQ(12 * 60 + 30, minutes);
    
    ASSERT_TRUE(NightMode::parseTimeString("00:00", minutes));
    ASSERT_EQ(0, minutes);
    
    ASSERT_TRUE(NightMode::parseTimeString("23:59", minutes));
    ASSERT_EQ(23 * 60 + 59, minutes);
}

TEST_F(NightModeTest, TimeStringParsing_Invalid) {
    uint16_t minutes;
    
    ASSERT_FALSE(NightMode::parseTimeString("25:00", minutes));  // Invalid hour
    ASSERT_FALSE(NightMode::parseTimeString("12:60", minutes));  // Invalid minute
    ASSERT_FALSE(NightMode::parseTimeString("12", minutes));     // No colon
    ASSERT_FALSE(NightMode::parseTimeString("", minutes));       // Empty
    ASSERT_FALSE(NightMode::parseTimeString("ab:cd", minutes));  // Non-numeric
}

TEST_F(NightModeTest, Persistence) {
    // Set values
    nightMode.setEnabled(true);
    nightMode.setDimPercent(75);
    nightMode.setSchedule(20 * 60, 8 * 60);
    
    // Create new instance (simulates reboot)
    NightMode newInstance;
    newInstance.begin();
    
    // Values should be restored
    ASSERT_TRUE(newInstance.isEnabled());
    ASSERT_EQ(75, newInstance.getDimPercent());
    ASSERT_EQ(20 * 60, newInstance.getStartMinutes());
    ASSERT_EQ(8 * 60, newInstance.getEndMinutes());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

---

## Phase 3: CI/CD Integration

### GitHub Actions Workflow

**File:** `.github/workflows/tests.yml`

```yaml
name: Unit Tests

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main, develop ]

jobs:
  test:
    runs-on: ubuntu-latest
    
    steps:
    - name: Checkout code
      uses: actions/checkout@v3
    
    - name: Cache PlatformIO
      uses: actions/cache@v3
      with:
        path: |
          ~/.platformio
          .pio
        key: ${{ runner.os }}-pio-${{ hashFiles('**/platformio.ini') }}
        restore-keys: |
          ${{ runner.os }}-pio-
    
    - name: Set up Python
      uses: actions/setup-python@v4
      with:
        python-version: '3.11'
    
    - name: Install PlatformIO
      run: |
        python -m pip install --upgrade pip
        pip install platformio
    
    - name: Run tests
      run: pio test -e native -v
    
    - name: Generate coverage report
      run: |
        # TODO: Add gcov/lcov integration
        echo "Coverage reporting to be implemented"
    
    - name: Upload test results
      if: always()
      uses: actions/upload-artifact@v3
      with:
        name: test-results
        path: .pio/test/
    
    - name: Comment PR with results
      if: github.event_name == 'pull_request'
      uses: actions/github-script@v6
      with:
        script: |
          const fs = require('fs');
          // Parse test results and post comment
          // TODO: Implement result parsing
```

---

## Phase 4: Documentation

### Test Documentation

**File:** `test/README.md`

```markdown
# Word Clock Firmware - Test Suite

## Overview

This directory contains unit and integration tests for the Word Clock firmware.

## Structure

```
test/
├── test_time_mapper/      # Time-to-LED mapping logic
├── test_led_controller/   # LED control and display
├── test_night_mode/       # Night mode scheduling
├── test_settings/         # Settings persistence
├── mocks/                 # Mock implementations
├── helpers/               # Test utilities
└── README.md             # This file
```

## Running Tests

### All Tests
```bash
pio test -e native
```

### Specific Test Suite
```bash
pio test -e native -f test_time_mapper
```

### Verbose Output
```bash
pio test -e native -v
```

### With Coverage
```bash
pio test -e native --coverage
```

## Writing Tests

### Test Structure
```cpp
#include <gtest/gtest.h>
#include "../mocks/mock_dependencies.h"
#include "../../src/module_to_test.cpp"

class ModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup before each test
    }
    
    void TearDown() override {
        // Cleanup after each test
    }
};

TEST_F(ModuleTest, TestName) {
    // Arrange
    auto input = createTestInput();
    
    // Act
    auto result = functionUnderTest(input);
    
    // Assert
    ASSERT_EQ(expected, result);
}
```

### Best Practices
1. **One assertion per test** (when possible)
2. **Descriptive test names** (what is being tested)
3. **Arrange-Act-Assert** pattern
4. **Independent tests** (no shared state)
5. **Fast execution** (< 1ms per test)

## Test Coverage Goals

| Module | Target Coverage |
|--------|----------------|
| time_mapper.cpp | 90% |
| led_controller.cpp | 85% |
| night_mode.cpp | 80% |
| display_settings.h | 75% |
| network.cpp | 60% |
| mqtt_client.cpp | 60% |

## Continuous Integration

Tests run automatically on:
- Push to `main` or `develop`
- Pull requests
- Nightly builds

See `.github/workflows/tests.yml` for configuration.

## Troubleshooting

### Test Compilation Fails
```bash
# Clean and rebuild
pio test -e native --clean
```

### Tests Pass Locally but Fail in CI
- Check for system-dependent code
- Verify mock implementations
- Check for timing dependencies

### Adding New Tests
1. Create test file in appropriate directory
2. Include necessary mocks
3. Write tests following patterns above
4. Run locally: `pio test -e native -f your_new_test`
5. Commit and push
```

---

## Implementation Timeline

### Week 1: Infrastructure
- **Day 1:** PlatformIO configuration
- **Day 2:** Mock framework (Preferences, Grid Layout)
- **Day 3:** Test helpers and utilities
- **Day 4:** CI/CD workflow setup
- **Day 5:** Documentation and examples

### Week 2: Core Tests
- **Day 1-2:** time_mapper tests (comprehensive)
- **Day 3:** led_controller tests
- **Day 4:** night_mode tests
- **Day 5:** Review and refine

### Week 3: Integration
- **Day 1:** settings persistence tests
- **Day 2:** Integration test framework
- **Day 3:** Test coverage analysis
- **Day 4:** Documentation completion
- **Day 5:** Team training

---

## Success Criteria

### Coverage Targets
- ✅ time_mapper: 90% coverage
- ✅ led_controller: 85% coverage
- ✅ night_mode: 80% coverage
- ✅ display_settings: 75% coverage

### Quality Metrics
- ✅ All tests pass consistently
- ✅ < 5 second total test execution
- ✅ CI integration working
- ✅ Documentation complete
- ✅ Team trained on writing tests

---

**Plan Version:** 1.0  
**Last Updated:** January 4, 2026  
**Status:** Ready for Implementation
