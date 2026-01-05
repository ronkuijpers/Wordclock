# Implementation Plan: Fix Critical MQTT Reconnect Abort Bug

**Priority:** CRITICAL  
**Estimated Effort:** 2 hours  
**Complexity:** Low  
**Risk:** Low  
**Impact:** HIGH - Fixes permanent MQTT failure after network issues

---

## Problem Statement

The MQTT client has a critical bug where once the reconnection backoff reaches maximum delay (60 seconds), the `reconnectAborted` flag is set to `true` and never reset. This permanently disables MQTT functionality until the device is rebooted.

**Affected File:** `src/mqtt_client.cpp`  
**Bug Location:** Lines 705-710

```cpp
if (reconnectDelayMs >= RECONNECT_DELAY_MAX_MS) {
    String errMsg = String("MQTT reconnect aborted after reaching max backoff (") +
                    RECONNECT_DELAY_MAX_MS + " ms); last error: " +
                    (g_lastErr.length() ? g_lastErr : String("unknown"));
    logError(errMsg);
    reconnectAborted = true;  // BUG: Never reset!
    return;
}
```

**Impact on Users:**
- MQTT stops working after prolonged network outage
- Device appears functional but no Home Assistant integration
- Requires physical reboot to restore functionality
- Poor reliability for production deployments

---

## Root Cause Analysis

### Why This Happens
1. Network outage lasts > 2 minutes
2. Exponential backoff reaches 60-second maximum
3. `reconnectAborted` flag set to prevent infinite retries
4. Flag is never cleared on:
   - Successful connection
   - Configuration change
   - Network recovery

### Design Flaw
The abort mechanism was intended to prevent resource exhaustion but lacks a reset path for recovery scenarios.

---

## Solution Design

### Approach
Add reset logic for `reconnectAborted` flag at appropriate recovery points:

1. **On successful connection** - Primary recovery path
2. **On configuration change** - Manual intervention path
3. **On explicit reconnect request** - API/MQTT command path

### Changes Required

#### Change 1: Reset on Successful Connection
**File:** `src/mqtt_client.cpp`  
**Function:** `mqtt_connect()`  
**Location:** After successful connection (around line 643)

```cpp
// CURRENT CODE (lines 640-648):
if (!ok) {
    int st = mqtt.state();
    g_connected = false;
    g_lastErr = String("connect failed (state ") + st + ")";
    return false;
}

publishAvailability("online");
publishBirth();
publishDiscovery();

// ADD AFTER LINE 648:
// Reset reconnection state on success
reconnectAttempts = 0;
reconnectDelayMs = RECONNECT_DELAY_MIN_MS;
reconnectAborted = false;  // CRITICAL: Allow future reconnections
logInfo("MQTT connected successfully - reconnection state reset");
```

#### Change 2: Reset on Configuration Change
**File:** `src/mqtt_client.cpp`  
**Function:** `mqtt_apply_settings()`  
**Location:** After existing reset logic (around line 752)

```cpp
// CURRENT CODE (lines 748-752):
buildTopics();
reconnectDelayMs = RECONNECT_DELAY_MIN_MS;
reconnectAttempts = 0;
reconnectAborted = false;  // ALREADY PRESENT BUT ADD COMMENT
lastReconnectAttempt = 0;

// ENHANCE WITH LOGGING:
if (reconnectAborted) {
    logInfo("MQTT reconnection re-enabled after configuration change");
}
reconnectAborted = false;
```

#### Change 3: Add Public Reconnect API
**File:** `src/mqtt_client.h`  
**Add new function declaration:**

```cpp
// Add to public API
void mqtt_force_reconnect();
```

**File:** `src/mqtt_client.cpp`  
**Add implementation:**

```cpp
/**
 * @brief Force MQTT reconnection attempt, clearing abort state
 * 
 * Useful for manual reconnection via web UI or MQTT command after
 * prolonged network outages that triggered reconnection abort.
 */
void mqtt_force_reconnect() {
    if (mqtt.connected()) {
        logInfo("MQTT already connected");
        return;
    }
    
    if (reconnectAborted) {
        logInfo("MQTT reconnection re-enabled by force reconnect");
    }
    
    // Reset all reconnection state
    reconnectAborted = false;
    reconnectAttempts = 0;
    reconnectDelayMs = RECONNECT_DELAY_MIN_MS;
    lastReconnectAttempt = 0;  // Trigger immediate attempt
    
    g_lastErr = "";
}
```

#### Change 4: Improve Abort Logging
**File:** `src/mqtt_client.cpp`  
**Location:** Lines 705-710

```cpp
// REPLACE EXISTING CODE WITH:
if (reconnectDelayMs >= RECONNECT_DELAY_MAX_MS) {
    String errMsg = String("MQTT reconnect paused after reaching max backoff (") +
                    RECONNECT_DELAY_MAX_MS + " ms); last error: " +
                    (g_lastErr.length() ? g_lastErr : String("unknown")) +
                    String(". Will retry on config change or manual reconnect.");
    logError(errMsg);
    reconnectAborted = true;
    
    // Note: reconnectAborted will be cleared on:
    // 1. Successful connection (mqtt_connect success path)
    // 2. Configuration change (mqtt_apply_settings)
    // 3. Manual reconnect (mqtt_force_reconnect)
    return;
}
```

---

## Implementation Steps

### Step 1: Backup and Branch (5 minutes)
```bash
# Create feature branch
git checkout -b fix/mqtt-reconnect-abort

# Backup current file
cp src/mqtt_client.cpp src/mqtt_client.cpp.backup
cp src/mqtt_client.h src/mqtt_client.h.backup
```

### Step 2: Implement Changes (30 minutes)
1. Add reset in `mqtt_connect()` success path
2. Enhance `mqtt_apply_settings()` with logging
3. Add `mqtt_force_reconnect()` function
4. Update abort logging message
5. Add function documentation

### Step 3: Code Review (15 minutes)
- Review all changes for correctness
- Check for missing edge cases
- Verify logging is appropriate
- Ensure thread safety (not needed for current architecture)

### Step 4: Compile and Test (45 minutes)

#### Compilation Test
```bash
# Build firmware
pio run -e esp32dev

# Check for compilation errors
# Expected: 0 errors, 0 warnings
```

#### Unit Test (Manual - future: automate)
Create test scenarios:

**Test 1: Normal Reconnection**
1. Device connected to MQTT
2. Stop MQTT broker
3. Wait 10 seconds
4. Restart broker
5. Verify reconnection within 30 seconds
6. Check logs for "reconnection state reset"

**Test 2: Max Backoff Recovery**
1. Device connected to MQTT
2. Stop MQTT broker
3. Wait 3 minutes (reach max backoff)
4. Verify "reconnect paused" error logged
5. Restart broker
6. Verify device reconnects within 60 seconds
7. Check for "reconnection state reset" log

**Test 3: Configuration Change Recovery**
1. Trigger max backoff (broker down 3 min)
2. Change MQTT configuration via web UI
3. Restore broker
4. Verify reconnection within 30 seconds
5. Check for "re-enabled after configuration change"

**Test 4: Manual Reconnect**
1. Trigger max backoff
2. Call `mqtt_force_reconnect()` (add test endpoint)
3. Restore broker
4. Verify immediate reconnection attempt
5. Check for "re-enabled by force reconnect"

**Test 5: Long-term Stability**
1. Run device for 24 hours
2. Simulate 5 network outages (random duration 1-5 min)
3. Verify MQTT recovers after each outage
4. Check memory leaks (heap should be stable)

### Step 5: Integration Testing (30 minutes)

#### Home Assistant Integration Test
1. Add device to Home Assistant
2. Verify all entities appear
3. Trigger network outage (2 minutes)
4. Verify entities become "unavailable"
5. Restore network
6. Verify entities return to "online" within 60 seconds
7. Test entity controls (brightness, color, etc.)

#### Web UI Test
1. Access web interface during normal operation
2. Trigger network outage
3. Verify web UI shows connection status
4. Restore network
5. Verify web UI updates reflect MQTT reconnection

---

## Testing Strategy

### Automated Tests (Future Enhancement)

```cpp
// test/test_mqtt_reconnect.cpp
#include <unity.h>
#include "mqtt_client.h"

void test_reconnect_abort_resets_on_success() {
    // Setup: Trigger abort condition
    extern bool reconnectAborted;
    reconnectAborted = true;
    
    // Simulate successful connection
    mqtt_connect();  // Mock to return true
    
    // Assert: Flag should be cleared
    TEST_ASSERT_FALSE(reconnectAborted);
}

void test_force_reconnect_clears_abort() {
    extern bool reconnectAborted;
    reconnectAborted = true;
    
    mqtt_force_reconnect();
    
    TEST_ASSERT_FALSE(reconnectAborted);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_reconnect_abort_resets_on_success);
    RUN_TEST(test_force_reconnect_clears_abort);
    UNITY_END();
}
```

### Manual Test Checklist

- [ ] Device boots normally with MQTT enabled
- [ ] MQTT connects on first boot
- [ ] Short network outage (10s) - recovers automatically
- [ ] Medium outage (60s) - recovers with backoff
- [ ] Long outage (3min) - triggers abort, but recovers when network returns
- [ ] Configuration change during abort - allows reconnection
- [ ] 24-hour stability test passes
- [ ] Memory usage stable (no leaks)
- [ ] Home Assistant integration works after recovery
- [ ] Web UI reflects correct MQTT status

---

## Rollback Plan

If issues are discovered after deployment:

### Quick Rollback
```bash
# Restore backup
cp src/mqtt_client.cpp.backup src/mqtt_client.cpp
cp src/mqtt_client.h.backup src/mqtt_client.h

# Rebuild and deploy
pio run -e esp32dev
pio run -t upload
```

### OTA Rollback
If devices are in field:
1. Keep previous firmware version available on update server
2. Set update channel to previous version
3. Push OTA update to all devices
4. Monitor recovery

---

## Verification Criteria

### Success Metrics
- ✅ Device reconnects within 60 seconds of network restoration
- ✅ No permanent MQTT failures observed in 24-hour test
- ✅ Configuration change allows immediate reconnection
- ✅ Manual reconnect API works correctly
- ✅ No memory leaks detected
- ✅ No new bugs introduced

### Performance Impact
- **Expected:** Negligible (< 1ms per reconnection)
- **Memory:** No additional RAM usage
- **Flash:** ~200 bytes additional code

---

## Documentation Updates

### Code Documentation
- Add function header comment for `mqtt_force_reconnect()`
- Update `mqtt_connect()` inline comments
- Document reconnection state machine

### User Documentation
Update user guide with:
- MQTT reconnection behavior explanation
- Troubleshooting: "MQTT not reconnecting" section
- Manual reconnect procedure (if exposed via web UI)

### Developer Documentation
Update architecture docs:
- MQTT state machine diagram
- Reconnection flow chart
- Error recovery paths

---

## Future Enhancements

### Short-term (Next Release)
1. Add web UI button for manual reconnect
2. Expose reconnect API via MQTT command
3. Add MQTT connection status to web UI dashboard
4. Log reconnection metrics for analytics

### Medium-term
1. Configurable max backoff duration
2. Smart backoff based on network quality
3. Automatic broker failover (primary/secondary)
4. Health check endpoint for monitoring

### Long-term
1. Predictive reconnection (detect network degradation)
2. Offline queuing (store messages during outage)
3. Connection quality metrics (latency, packet loss)
4. Advanced diagnostics (connection history, failure patterns)

---

## Related Issues

**GitHub Issues** (if applicable):
- #XX: MQTT stops working after network outage
- #YY: Device requires reboot to restore Home Assistant integration
- #ZZ: Reconnection backoff never resets

**Related Bugs:**
- Time sync race condition (may cause related symptoms)
- Network reconnect strategy (WiFi layer)

---

## Approval and Sign-off

### Pre-merge Checklist
- [ ] Code changes implemented and reviewed
- [ ] All manual tests passed
- [ ] No compilation warnings
- [ ] Memory usage verified (no leaks)
- [ ] Documentation updated
- [ ] Changelog entry added
- [ ] Version bumped (patch level)

### Reviewers
- [ ] Code review: _____________
- [ ] Testing sign-off: _____________
- [ ] Documentation review: _____________

### Deployment Plan
1. Merge to `develop` branch
2. Beta test on 3-5 devices (1 week)
3. Collect feedback and metrics
4. Merge to `main` branch
5. Tag release (e.g., v26.1.6)
6. OTA rollout (phased: 10% → 50% → 100%)

---

## Appendix: Code Snippets

### Full Implementation Reference

**mqtt_client.h additions:**
```cpp
// Add to public API section
void mqtt_force_reconnect();
```

**mqtt_client.cpp changes:**

```cpp
// Change 1: Reset on successful connection (add after line 648)
reconnectAttempts = 0;
reconnectDelayMs = RECONNECT_DELAY_MIN_MS;
reconnectAborted = false;
if (g_lastErr.length() > 0) {
    logInfo(String("MQTT reconnected after error: ") + g_lastErr);
}
g_lastErr = "";

// Change 2: Already present in mqtt_apply_settings(), enhance logging
if (reconnectAborted) {
    logInfo("MQTT reconnection re-enabled after configuration change");
}

// Change 3: New function
void mqtt_force_reconnect() {
    if (mqtt.connected()) {
        logInfo("MQTT already connected");
        return;
    }
    
    if (reconnectAborted) {
        logInfo("MQTT reconnection re-enabled by force reconnect");
    }
    
    reconnectAborted = false;
    reconnectAttempts = 0;
    reconnectDelayMs = RECONNECT_DELAY_MIN_MS;
    lastReconnectAttempt = 0;
    g_lastErr = "";
}

// Change 4: Update abort message (replace lines 705-710)
if (reconnectDelayMs >= RECONNECT_DELAY_MAX_MS) {
    String errMsg = String("MQTT reconnect paused after reaching max backoff (") +
                    RECONNECT_DELAY_MAX_MS + " ms); last error: " +
                    (g_lastErr.length() ? g_lastErr : String("unknown")) +
                    String(". Will retry on config change or manual reconnect.");
    logWarn(errMsg);  // Changed from logError
    reconnectAborted = true;
    return;
}
```

---

## Contact

**Implementation Owner:** [Assign developer]  
**Technical Reviewer:** [Assign reviewer]  
**Testing Lead:** [Assign tester]  

**Questions/Issues:** Create GitHub issue with tag `mqtt-reconnect-fix`

---

**Plan Version:** 1.0  
**Last Updated:** January 4, 2026  
**Status:** Ready for Implementation
