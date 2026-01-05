# How to Use mqtt_force_reconnect()

The `mqtt_force_reconnect()` function forces an immediate MQTT reconnection attempt, clearing the "slow retry mode" state. Here are all the ways to call it:

---

## ✅ Method 1: Web UI Endpoint (IMPLEMENTED)

### Using Web Browser

**Endpoint:** `POST /api/mqtt/reconnect`

**Example using curl:**
```bash
curl -X POST http://wordclock.local/api/mqtt/reconnect
```

**Example using JavaScript (in browser console):**
```javascript
fetch('/api/mqtt/reconnect', { method: 'POST' })
  .then(response => response.text())
  .then(data => console.log(data));
```

**Example using wget:**
```bash
wget --post-data='' http://wordclock.local/api/mqtt/reconnect
```

### Add Button to Web UI

You can add a "Force Reconnect" button to your dashboard. In your HTML file (e.g., `data/mqtt.html`):

```html
<!-- Add this button to your MQTT settings page -->
<button onclick="forceReconnect()" id="btnForceReconnect" class="btn btn-warning">
  🔄 Force Reconnect
</button>

<script>
function forceReconnect() {
  if (!confirm('Force MQTT reconnection now?')) return;
  
  const btn = document.getElementById('btnForceReconnect');
  btn.disabled = true;
  btn.textContent = '🔄 Reconnecting...';
  
  fetch('/api/mqtt/reconnect', { method: 'POST' })
    .then(response => response.text())
    .then(data => {
      alert('Reconnection triggered: ' + data);
      // Check status after 2 seconds
      setTimeout(() => {
        fetch('/api/mqtt/status')
          .then(r => r.json())
          .then(status => {
            if (status.connected) {
              alert('✅ MQTT reconnected successfully!');
            } else {
              alert('⚠️ Still connecting... Error: ' + status.last_error);
            }
          });
      }, 2000);
    })
    .catch(err => alert('Error: ' + err))
    .finally(() => {
      btn.disabled = false;
      btn.textContent = '🔄 Force Reconnect';
    });
}
</script>
```

---

## 🔧 Method 2: MQTT Command Topic (Add This)

You can also trigger reconnect via MQTT itself (useful for automation). Add this handler:

### Implementation

**File:** `src/mqtt_client.cpp` (in `handleMessage()` function)

Add after line 584 (after existing command handlers):

```cpp
} else if (is(tForceReconnectCmd)) {
  logInfo("🔄 Force MQTT reconnect requested via MQTT command");
  mqtt_force_reconnect();
```

Then add the topic definition:

**File:** `src/mqtt_client.cpp` (around line 41, with other topic definitions)

```cpp
static String tRestartCmd, tSeqCmd, tUpdateCmd, tForceReconnectCmd;
```

**File:** `src/mqtt_client.cpp` (in `buildTopics()` function, around line 95)

```cpp
tRestartCmd   = base + "/restart/press";
tSeqCmd       = base + "/sequence/press";
tUpdateCmd    = base + "/update/press";
tForceReconnectCmd = base + "/mqtt_reconnect/press";  // ADD THIS
```

**File:** `src/mqtt_client.cpp` (in `publishDiscovery()` function, around line 272)

```cpp
publishButton("Restart", tRestartCmd, nodeId + String("_restart"));
publishButton("Start sequence", tSeqCmd, nodeId + String("_sequence"));
publishButton("Check for update", tUpdateCmd, nodeId + String("_update"));
publishButton("Force MQTT Reconnect", tForceReconnectCmd, nodeId + String("_mqtt_reconnect")); // ADD THIS
```

**File:** `src/mqtt_client.cpp` (in `mqtt_connect()` function, around line 640 with other subscriptions)

```cpp
mqtt.subscribe(tRestartCmd.c_str());
mqtt.subscribe(tSeqCmd.c_str());
mqtt.subscribe(tUpdateCmd.c_str());
mqtt.subscribe(tForceReconnectCmd.c_str());  // ADD THIS
```

### Usage via MQTT

Once implemented, you can trigger it from:

**Home Assistant:**
- A button entity will appear automatically: `button.wordclock_mqtt_reconnect`
- Press it to trigger force reconnect

**mosquitto_pub:**
```bash
mosquitto_pub -h localhost -t 'wordclock/mqtt_reconnect/press' -m 'PRESS'
```

**Node-RED:**
```javascript
msg.topic = "wordclock/mqtt_reconnect/press";
msg.payload = "PRESS";
return msg;
```

---

## 💻 Method 3: Direct Code Call (Testing)

For testing or custom logic, call it directly from your code:

### Example 1: Add to Serial Command Handler

```cpp
// In main.cpp or a custom handler
void handleSerialCommands() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd == "mqtt_reconnect") {
      Serial.println("Force reconnecting MQTT...");
      mqtt_force_reconnect();
      Serial.println("Reconnection triggered");
    }
  }
}

void loop() {
  // ... existing code ...
  handleSerialCommands();
}
```

**Usage:**
Open serial monitor and type: `mqtt_reconnect`

### Example 2: Automatic Trigger on Network Recovery

```cpp
// In network.cpp or main.cpp
void processNetwork() {
  auto& wm = getManager();
  wm.process();

  bool connected = (WiFi.status() == WL_CONNECTED);
  
  static bool wasDisconnected = false;
  
  if (connected && wasDisconnected) {
    logInfo("✅ WiFi connection established: " + String(WiFi.SSID()));
    logInfo("📡 IP address: " + WiFi.localIP().toString());
    
    // AUTO-TRIGGER: Force MQTT reconnect after WiFi recovery
    if (!mqtt_is_connected()) {
      logInfo("🔄 Auto-triggering MQTT force reconnect after WiFi recovery");
      mqtt_force_reconnect();
    }
    
    wasDisconnected = false;
  } else if (!connected && !wasDisconnected) {
    logWarn("⚠️ WiFi connection lost.");
    wasDisconnected = true;
  }
  
  // ... rest of function
}
```

### Example 3: Watchdog Timer

Add a periodic check that forces reconnect if MQTT is stuck:

```cpp
// In main.cpp loop()
void loop() {
  // ... existing code ...
  
  static unsigned long lastMqttCheck = 0;
  unsigned long now = millis();
  
  // Check MQTT status every 5 minutes
  if (now - lastMqttCheck >= 300000UL) {  // 5 minutes
    lastMqttCheck = now;
    
    if (!mqtt_is_connected()) {
      String err = mqtt_last_error();
      if (err.length() > 0) {
        logWarn("⚠️ MQTT disconnected for 5+ minutes. Forcing reconnect...");
        mqtt_force_reconnect();
      }
    }
  }
}
```

---

## 📱 Method 4: Home Assistant Automation

### Using the Web API

Create a REST command in `configuration.yaml`:

```yaml
rest_command:
  wordclock_force_mqtt_reconnect:
    url: "http://wordclock.local/api/mqtt/reconnect"
    method: POST
    timeout: 10
```

Then create an automation:

```yaml
automation:
  - alias: "Word Clock - Force MQTT Reconnect on Error"
    trigger:
      - platform: state
        entity_id: sensor.wordclock_last_error
        to: ~
    condition:
      - condition: template
        value_template: "{{ trigger.to_state.state != 'unknown' and trigger.to_state.state != '' }}"
    action:
      - service: rest_command.wordclock_force_mqtt_reconnect
      - service: notify.mobile_app
        data:
          message: "Word Clock MQTT reconnect triggered"
```

### Using MQTT Button (After Method 2 Implementation)

```yaml
automation:
  - alias: "Word Clock - Auto Reconnect Daily"
    trigger:
      - platform: time
        at: "03:00:00"
    action:
      - service: button.press
        target:
          entity_id: button.wordclock_mqtt_reconnect
```

---

## 🧪 Testing the Function

### Test Scenario 1: Immediate Call

```bash
# Using web API
curl -X POST http://wordclock.local/api/mqtt/reconnect

# Check status
curl http://wordclock.local/api/mqtt/status
```

**Expected Output:**
```json
{"connected":true,"last_error":""}
```

### Test Scenario 2: During Slow Retry Mode

1. Stop MQTT broker
2. Wait 3+ minutes (enter slow retry mode)
3. Restart broker
4. Call force reconnect:
   ```bash
   curl -X POST http://wordclock.local/api/mqtt/reconnect
   ```
5. Check logs - should see:
   ```
   🔄 MQTT reconnection re-enabled by force reconnect
   ✅ MQTT reconnected successfully after slow retry mode!
   ```

### Test Scenario 3: Already Connected

```bash
curl -X POST http://wordclock.local/api/mqtt/reconnect
```

**Expected Log:**
```
MQTT already connected
```

No harm done - function handles this gracefully.

---

## 📊 Monitoring Force Reconnects

### Add Counter (Optional Enhancement)

Track how many times force reconnect is called:

**File:** `src/mqtt_client.cpp`

```cpp
static uint32_t g_forceReconnectCount = 0;

void mqtt_force_reconnect() {
  g_forceReconnectCount++;
  
  if (mqtt.connected()) {
    logInfo("MQTT already connected");
    return;
  }
  
  if (reconnectAborted) {
    logInfo(String("🔄 MQTT reconnection re-enabled by force reconnect (call #") + 
            g_forceReconnectCount + ")");
  }
  
  // ... rest of function
}
```

Expose via web API:

```cpp
server.on("/api/mqtt/stats", HTTP_GET, []() {
  if (!ensureUiAuth()) return;
  extern uint32_t g_forceReconnectCount;
  String json = String("{\"force_reconnect_count\":") + g_forceReconnectCount + "}";
  server.send(200, "application/json", json);
});
```

---

## 🔍 Troubleshooting

### Function Not Available Error

**Error:** `'mqtt_force_reconnect' was not declared in this scope`

**Solution:** Make sure you're including the header:
```cpp
#include "mqtt_client.h"
```

### Web Endpoint Returns 404

**Issue:** Endpoint not registered

**Solution:** Verify the endpoint was added to `setupWebRoutes()` in `web_routes.h`

### MQTT Button Not Appearing in Home Assistant

**Issue:** Discovery not published or topic not subscribed

**Solution:**
1. Check MQTT discovery is enabled
2. Verify topic is in `buildTopics()`
3. Verify subscription in `mqtt_connect()`
4. Reconnect to publish discovery

### Force Reconnect Has No Effect

**Issue:** Device might not be in slow retry mode

**Check:**
```bash
curl http://wordclock.local/api/mqtt/status
```

If `connected: true`, force reconnect does nothing (already connected).

---

## 📝 Summary

### Currently Implemented ✅
- ✅ `mqtt_force_reconnect()` function exists
- ✅ Web API endpoint: `POST /api/mqtt/reconnect`
- ✅ Function clears slow retry mode
- ✅ Triggers immediate reconnection

### To Implement (Optional) ⏳
- ⏳ MQTT command topic (for Home Assistant button)
- ⏳ Serial command handler (for debugging)
- ⏳ Automatic trigger on WiFi recovery
- ⏳ Watchdog timer (periodic check)
- ⏳ Statistics tracking

### When to Use
- **Automatic:** After prolonged MQTT outage (slow retry mode)
- **Manual:** Troubleshooting MQTT issues
- **Automation:** Scheduled maintenance (e.g., daily at 3 AM)
- **Debugging:** Testing MQTT reconnection logic

---

## Example Full Implementation

Here's a complete example showing how to use it from a dashboard:

```html
<!-- Add to data/mqtt.html or data/dashboard.html -->
<div class="mqtt-control">
  <h3>MQTT Connection</h3>
  
  <div id="mqtt-status">
    <span id="status-indicator">●</span>
    <span id="status-text">Checking...</span>
  </div>
  
  <button onclick="checkMqttStatus()" class="btn btn-primary">
    🔍 Check Status
  </button>
  
  <button onclick="forceMqttReconnect()" class="btn btn-warning">
    🔄 Force Reconnect
  </button>
</div>

<script>
async function checkMqttStatus() {
  try {
    const response = await fetch('/api/mqtt/status');
    const data = await response.json();
    
    const indicator = document.getElementById('status-indicator');
    const text = document.getElementById('status-text');
    
    if (data.connected) {
      indicator.style.color = '#00ff00';
      text.textContent = 'Connected';
    } else {
      indicator.style.color = '#ff0000';
      text.textContent = 'Disconnected: ' + (data.last_error || 'Unknown error');
    }
  } catch (error) {
    console.error('Status check failed:', error);
  }
}

async function forceMqttReconnect() {
  if (!confirm('Force MQTT reconnection now?')) return;
  
  try {
    const response = await fetch('/api/mqtt/reconnect', { method: 'POST' });
    const result = await response.text();
    
    alert('✅ ' + result);
    
    // Check status after 2 seconds
    setTimeout(checkMqttStatus, 2000);
  } catch (error) {
    alert('❌ Error: ' + error.message);
  }
}

// Check status on page load
window.addEventListener('load', checkMqttStatus);

// Auto-refresh status every 30 seconds
setInterval(checkMqttStatus, 30000);
</script>

<style>
#status-indicator {
  font-size: 24px;
  line-height: 1;
}

.mqtt-control {
  padding: 20px;
  border: 1px solid #ccc;
  border-radius: 8px;
  margin: 20px 0;
}

.btn {
  padding: 10px 20px;
  margin: 5px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
}

.btn-primary {
  background: #007bff;
  color: white;
}

.btn-warning {
  background: #ffc107;
  color: black;
}
</style>
```

---

## Quick Reference

| Method | Endpoint/Call | When to Use |
|--------|--------------|-------------|
| **Web API** | `POST /api/mqtt/reconnect` | Manual troubleshooting via browser |
| **MQTT Topic** | `wordclock/mqtt_reconnect/press` | Home Assistant automation |
| **Direct Code** | `mqtt_force_reconnect();` | Testing, debugging, custom logic |
| **Serial Console** | Type `mqtt_reconnect` | Development and debugging |

---

**Created:** January 4, 2026  
**Status:** Web API ✅ Implemented, MQTT Topic ⏳ Optional  
**Documentation:** Complete
