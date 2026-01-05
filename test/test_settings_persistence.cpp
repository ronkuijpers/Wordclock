#include <unity.h>
#include "led_state.h"

// Simple tests for LedState deferred persistence
// Note: Tests for DisplaySettings, NightMode, and SetupState require
// full project dependencies and are better suited for integration testing
// on actual hardware.

void test_led_state_deferred_write() {
    LedState testLedState;
    testLedState.begin();
    
    // Change value
    testLedState.setRGB(255, 0, 0);
    
    // Should be dirty
    TEST_ASSERT_TRUE(testLedState.isDirty());
    
    // Value should be updated in memory immediately
    uint8_t r, g, b, w;
    testLedState.getRGBW(r, g, b, w);
    TEST_ASSERT_EQUAL(255, r);
    TEST_ASSERT_EQUAL(0, g);
    TEST_ASSERT_EQUAL(0, b);
    
    // Flush
    testLedState.flush();
    
    // Should not be dirty
    TEST_ASSERT_FALSE(testLedState.isDirty());
}

void test_no_flash_write_on_duplicate_value() {
    LedState testLedState;
    testLedState.begin();
    
    testLedState.setRGB(128, 128, 128);
    testLedState.flush();
    TEST_ASSERT_FALSE(testLedState.isDirty());
    
    // Set to same value
    testLedState.setRGB(128, 128, 128);
    TEST_ASSERT_FALSE(testLedState.isDirty());  // Should not be dirty
}

void test_brightness_deferred_write() {
    LedState testLedState;
    testLedState.begin();
    
    // Change brightness
    testLedState.setBrightness(128);
    
    // Should be dirty
    TEST_ASSERT_TRUE(testLedState.isDirty());
    
    // Value should be updated in memory immediately
    TEST_ASSERT_EQUAL(128, testLedState.getBrightness());
    
    // Flush
    testLedState.flush();
    
    // Should not be dirty
    TEST_ASSERT_FALSE(testLedState.isDirty());
}

void test_multiple_changes_before_flush() {
    LedState testLedState;
    testLedState.begin();
    
    // Multiple changes
    testLedState.setRGB(255, 0, 0);
    testLedState.setBrightness(200);
    testLedState.setRGB(0, 255, 0);
    
    // Should still be dirty (only one flush needed)
    TEST_ASSERT_TRUE(testLedState.isDirty());
    
    // Flush once
    testLedState.flush();
    
    // Should not be dirty
    TEST_ASSERT_FALSE(testLedState.isDirty());
}

void test_white_color_logic() {
    LedState testLedState;
    testLedState.begin();
    
    // Set to white (255, 255, 255)
    testLedState.setRGB(255, 255, 255);
    
    uint8_t r, g, b, w;
    testLedState.getRGBW(r, g, b, w);
    
    // When white is detected, RGB should be 0 and W should be 255
    TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT_EQUAL(0, g);
    TEST_ASSERT_EQUAL(0, b);
    TEST_ASSERT_EQUAL(255, w);
}

void test_flush_timing_tracking() {
    LedState testLedState;
    testLedState.begin();
    
    // Make a change
    testLedState.setRGB(100, 100, 100);
    
    // Time since last flush should be very small
    unsigned long timeSinceFlush = testLedState.millisSinceLastFlush();
    TEST_ASSERT_LESS_THAN(100, timeSinceFlush);  // Should be < 100ms
    
    // Flush
    testLedState.flush();
    
    // After flush, time should reset
    timeSinceFlush = testLedState.millisSinceLastFlush();
    TEST_ASSERT_LESS_THAN(10, timeSinceFlush);  // Should be very recent
}

void setup() {
    // NOTE: Wait 2 seconds before the Unity test runner starts
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_led_state_deferred_write);
    RUN_TEST(test_no_flash_write_on_duplicate_value);
    RUN_TEST(test_brightness_deferred_write);
    RUN_TEST(test_multiple_changes_before_flush);
    RUN_TEST(test_white_color_logic);
    RUN_TEST(test_flush_timing_tracking);
    
    UNITY_END();
}

void loop() {
    // Nothing here - tests run once in setup()
}

/* 
 * INTEGRATION TESTS
 * 
 * For complete testing of DisplaySettings, NightMode, and SetupState,
 * run integration tests on actual hardware:
 * 
 * 1. Flash firmware to device
 * 2. Test rapid MQTT color changes (should not cause stuttering)
 * 3. Change settings via web UI and power cycle after 3 seconds (should not persist)
 * 4. Change settings and wait 6+ seconds, then power cycle (should persist)
 * 5. Trigger OTA update with unsaved changes (should flush before OTA)
 * 6. Monitor serial output for migration messages on first boot
 * 
 * Auto-flush timing test (5 seconds):
 * 1. Change a setting via MQTT
 * 2. Wait 4 seconds - setting should still be dirty
 * 3. Wait 2 more seconds - setting should auto-flush
 * 4. Power cycle - setting should be persisted
 */
