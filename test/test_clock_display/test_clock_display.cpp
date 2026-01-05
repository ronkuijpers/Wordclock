#include <gtest/gtest.h>
#include "../mocks/mock_arduino.h"
#include "clock_display.h"
#include "../helpers/test_utils.h"

// External dependencies that need to be defined for linking
bool clockEnabled = true;
bool g_initialTimeSyncSucceeded = false;

class ClockDisplayTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset state before each test
        clockEnabled = true;
        g_initialTimeSyncSucceeded = false;
    }
    
    void TearDown() override {
        // Clean up after each test
    }
};

// ============================================================================
// Test: Basic Construction and Reset
// ============================================================================

TEST_F(ClockDisplayTest, Constructs) {
    ClockDisplay display;
    EXPECT_TRUE(true); // If we get here, construction succeeded
}

TEST_F(ClockDisplayTest, Reset) {
    ClockDisplay display;
    display.reset();
    EXPECT_TRUE(true); // Reset should not crash
}

// ============================================================================
// Test: Clock Disabled Scenarios
// ============================================================================

TEST_F(ClockDisplayTest, UpdateReturnsFalseWhenClockDisabled) {
    ClockDisplay display;
    clockEnabled = false;
    
    bool result = display.update();
    
    EXPECT_FALSE(result);
}

// ============================================================================
// Test: Helper Functions
// ============================================================================

TEST_F(ClockDisplayTest, IsHetIs_IdentifiesHet) {
    WordSegment seg;
    seg.key = "HET";
    seg.leds = {0, 1, 2};
    
    EXPECT_TRUE(ClockDisplay::isHetIs(seg));
}

TEST_F(ClockDisplayTest, IsHetIs_IdentifiesIs) {
    WordSegment seg;
    seg.key = "IS";
    seg.leds = {3, 4};
    
    EXPECT_TRUE(ClockDisplay::isHetIs(seg));
}

TEST_F(ClockDisplayTest, IsHetIs_RejectsOtherWords) {
    WordSegment seg;
    seg.key = "VIJF";
    seg.leds = {5, 6, 7, 8};
    
    EXPECT_FALSE(ClockDisplay::isHetIs(seg));
}

TEST_F(ClockDisplayTest, StripHetIsIfDisabled_RemovesWhenZero) {
    std::vector<WordSegment> segments = {
        {"HET", {0, 1, 2}},
        {"IS", {3, 4}},
        {"VIJF", {5, 6, 7, 8}}
    };
    
    ClockDisplay::stripHetIsIfDisabled(segments, 0);
    
    EXPECT_EQ(1, segments.size());
    EXPECT_STREQ("VIJF", segments[0].key);
}

TEST_F(ClockDisplayTest, StripHetIsIfDisabled_KeepsWhenNonzero) {
    std::vector<WordSegment> segments = {
        {"HET", {0, 1, 2}},
        {"IS", {3, 4}},
        {"VIJF", {5, 6, 7, 8}}
    };
    
    ClockDisplay::stripHetIsIfDisabled(segments, 5);
    
    EXPECT_EQ(3, segments.size());
}

TEST_F(ClockDisplayTest, FlattenSegments) {
    std::vector<WordSegment> segments = {
        {"HET", {0, 1, 2}},
        {"IS", {3, 4}},
        {"VIJF", {5, 6, 7, 8}}
    };
    
    std::vector<uint16_t> flattened = ClockDisplay::flattenSegments(segments);
    
    EXPECT_EQ(9, flattened.size());
    EXPECT_EQ(0, flattened[0]);
    EXPECT_EQ(8, flattened[8]);
}

TEST_F(ClockDisplayTest, FindSegment_FindsExisting) {
    std::vector<WordSegment> segments = {
        {"HET", {0, 1, 2}},
        {"IS", {3, 4}},
        {"VIJF", {5, 6, 7, 8}}
    };
    
    const WordSegment* found = ClockDisplay::findSegment(segments, "VIJF");
    
    EXPECT_NE(nullptr, found);
    EXPECT_STREQ("VIJF", found->key);
}

TEST_F(ClockDisplayTest, FindSegment_ReturnsNullForMissing) {
    std::vector<WordSegment> segments = {
        {"HET", {0, 1, 2}},
        {"IS", {3, 4}}
    };
    
    const WordSegment* found = ClockDisplay::findSegment(segments, "MISSING");
    
    EXPECT_EQ(nullptr, found);
}

TEST_F(ClockDisplayTest, RemoveLeds) {
    std::vector<uint16_t> base = {0, 1, 2, 3, 4, 5};
    std::vector<uint16_t> toRemove = {1, 3, 5};
    
    ClockDisplay::removeLeds(base, toRemove);
    
    EXPECT_EQ(3, base.size());
    EXPECT_EQ(0, base[0]);
    EXPECT_EQ(2, base[1]);
    EXPECT_EQ(4, base[2]);
}

// ============================================================================
// Test: HET IS Visibility Logic
// ============================================================================

TEST_F(ClockDisplayTest, HetIsCurrentlyVisible_AlwaysFalseWhenZero) {
    bool visible = ClockDisplay::hetIsCurrentlyVisible(0, 0, 1000);
    EXPECT_FALSE(visible);
}

TEST_F(ClockDisplayTest, HetIsCurrentlyVisible_AlwaysTrueWhen360Plus) {
    bool visible = ClockDisplay::hetIsCurrentlyVisible(360, 5000, 10000);
    EXPECT_TRUE(visible);
    
    visible = ClockDisplay::hetIsCurrentlyVisible(500, 5000, 10000);
    EXPECT_TRUE(visible);
}

TEST_F(ClockDisplayTest, HetIsCurrentlyVisible_TrueWhenNotExpired) {
    unsigned long visibleUntil = 10000;
    unsigned long nowMs = 5000;
    
    bool visible = ClockDisplay::hetIsCurrentlyVisible(30, visibleUntil, nowMs);
    EXPECT_TRUE(visible);
}

TEST_F(ClockDisplayTest, HetIsCurrentlyVisible_FalseWhenExpired) {
    unsigned long visibleUntil = 5000;
    unsigned long nowMs = 10000;
    
    bool visible = ClockDisplay::hetIsCurrentlyVisible(30, visibleUntil, nowMs);
    EXPECT_FALSE(visible);
}

// ============================================================================
// Test: Animation Frame Building
// ============================================================================

TEST_F(ClockDisplayTest, BuildClassicFrames_CreatesCumulativeFrames) {
    std::vector<WordSegment> segments = {
        {"HET", {0, 1, 2}},
        {"IS", {3, 4}},
        {"VIJF", {5, 6, 7, 8}}
    };
    std::vector<std::vector<uint16_t>> frames;
    
    ClockDisplay::buildClassicFrames(segments, frames);
    
    EXPECT_EQ(3, frames.size());
    EXPECT_EQ(3, frames[0].size()); // HET
    EXPECT_EQ(5, frames[1].size()); // HET + IS
    EXPECT_EQ(9, frames[2].size()); // HET + IS + VIJF
}

TEST_F(ClockDisplayTest, BuildClassicFrames_EmptyInput) {
    std::vector<WordSegment> segments;
    std::vector<std::vector<uint16_t>> frames;
    
    ClockDisplay::buildClassicFrames(segments, frames);
    
    EXPECT_EQ(0, frames.size());
}

TEST_F(ClockDisplayTest, BuildSmartFrames_FallsBackToClassicWhenNoPrevious) {
    std::vector<WordSegment> prevSegments;  // Empty
    std::vector<WordSegment> nextSegments = {
        {"HET", {0, 1, 2}},
        {"IS", {3, 4}}
    };
    std::vector<std::vector<uint16_t>> frames;
    
    ClockDisplay::buildSmartFrames(prevSegments, nextSegments, true, frames);
    
    EXPECT_EQ(2, frames.size());
    EXPECT_EQ(3, frames[0].size());
    EXPECT_EQ(5, frames[1].size());
}

// ============================================================================
// Test: Force Animation API
// ============================================================================

TEST_F(ClockDisplayTest, ForceAnimationForTime) {
    ClockDisplay display;
    struct tm testTime = {0};
    testTime.tm_hour = 15;
    testTime.tm_min = 30;
    
    // Should not crash
    display.forceAnimationForTime(testTime);
    
    EXPECT_TRUE(true);
}

