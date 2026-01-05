#include "mqtt_client.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "display_settings.h"
#include "led_state.h"
#include "log.h"
#include "ota_updater.h"
#include "wordclock.h"
#include "time_mapper.h"
#include "sequence_controller.h"
#include "mqtt_settings.h"
#include <esp_system.h>
#include <Preferences.h>
#include "night_mode.h"
#include "system_utils.h"

extern DisplaySettings displaySettings;
extern bool clockEnabled;
extern StartupSequence startupSequence;

static WiFiClient espClient;
static PubSubClient mqtt(espClient);

static String uniqId;
static MqttSettings g_mqttCfg;
static String base;         // e.g., MQTT_BASE_TOPIC
static String availTopic;   // base + "/availability"
static String tBirth;
static bool g_connected = false;
static String g_lastErr;

// Topics
static String tLightState, tLightSet;
static String tClockState, tClockSet;
static String tAnimState, tAnimSet;
static String tAutoUpdState, tAutoUpdSet;
static String tHetIsState, tHetIsSet;
static String tLogLvlState, tLogLvlSet;
static String tRestartCmd, tSeqCmd, tUpdateCmd;
static String tNightEnabledState, tNightEnabledSet;
static String tNightOverrideState, tNightOverrideSet;
static String tNightActiveState;
static String tNightEffectState, tNightEffectSet;
static String tNightDimState, tNightDimSet;
static String tNightStartState, tNightStartSet;
static String tNightEndState, tNightEndSet;
static String tVersion, tUiVersion, tIp, tRssi, tUptime;
static String tHeap, tWifiChan, tBootReason, tResetCount;
static String tUpdateChannelState, tUpdateAutoAllowed, tUpdateAvailable;

static unsigned long lastReconnectAttempt = 0;
static unsigned long lastStateAt = 0;
static const unsigned long STATE_INTERVAL_MS = 30000; // 30s
static const unsigned long RECONNECT_DELAY_MIN_MS = 2000;
static const unsigned long RECONNECT_DELAY_MAX_MS = 60000;
static unsigned long reconnectDelayMs = RECONNECT_DELAY_MIN_MS;
static uint8_t reconnectAttempts = 0;
static bool reconnectAborted = false;

static void buildTopics() {
  base = g_mqttCfg.baseTopic;
  availTopic = base + "/availability";
  tBirth        = base + "/birth";
  tLightState   = base + "/light/state";
  tLightSet     = base + "/light/set";
  tClockState   = base + "/clock/state";
  tClockSet     = base + "/clock/set";
  tAnimState    = base + "/animate/state";
  tAnimSet      = base + "/animate/set";
  tAutoUpdState = base + "/autoupdate/state";
  tAutoUpdSet   = base + "/autoupdate/set";
  tHetIsState   = base + "/hetis/state";
  tHetIsSet     = base + "/hetis/set";
  tNightEnabledState = base + "/nightmode/enabled/state";
  tNightEnabledSet   = base + "/nightmode/enabled/set";
  tNightOverrideState = base + "/nightmode/override/state";
  tNightOverrideSet   = base + "/nightmode/override/set";
  tNightActiveState   = base + "/nightmode/active";
  tNightEffectState = base + "/nightmode/effect/state";
  tNightEffectSet   = base + "/nightmode/effect/set";
  tNightDimState    = base + "/nightmode/dim/state";
  tNightDimSet      = base + "/nightmode/dim/set";
  tNightStartState  = base + "/nightmode/start/state";
  tNightStartSet    = base + "/nightmode/start/set";
  tNightEndState    = base + "/nightmode/end/state";
  tNightEndSet      = base + "/nightmode/end/set";
  tLogLvlState  = base + "/loglevel/state";
  tLogLvlSet    = base + "/loglevel/set";
  tRestartCmd   = base + "/restart/press";
  tSeqCmd       = base + "/sequence/press";
  tUpdateCmd    = base + "/update/press";
  tVersion      = base + "/version";
  tUiVersion    = base + "/uiversion";
  tIp           = base + "/ip";
  tRssi         = base + "/rssi";
  tUptime       = base + "/laststartup";
  tHeap         = base + "/heap";
  tWifiChan     = base + "/wifi_channel";
  tBootReason   = base + "/boot_reason";
  tResetCount   = base + "/reset_count";
  tUpdateChannelState = base + "/update/channel";
  tUpdateAutoAllowed  = base + "/update/auto_allowed";
  tUpdateAvailable    = base + "/update/available";
}

static void publishDiscovery() {
  // Increase buffer size to accommodate discovery payloads
  mqtt.setBufferSize(1024);
  String nodeId = uniqId;
  String devIds = String("{\"ids\":[\"") + nodeId + "\"]}";

  auto pubCfg = [&](const String& comp, const String& objId, JsonDocument& cfg){
    String topic = String(g_mqttCfg.discoveryPrefix) + "/" + comp + "/" + objId + "/config";
    String out; serializeJson(cfg, out);
    mqtt.publish(topic.c_str(), out.c_str(), true);
  };

  // Light entity (JSON schema)
  {
    JsonDocument cfg;
    cfg["name"] = CLOCK_NAME;
    cfg["uniq_id"] = (nodeId + "_light");
    cfg["schema"] = "json";
    cfg["brightness"] = true;
    cfg["rgb"] = true;
    cfg["cmd_t"] = tLightSet;
    cfg["stat_t"] = tLightState;
    cfg["avty_t"] = availTopic;
    JsonObject dev = cfg["dev"].to<JsonObject>();
    dev["name"] = CLOCK_NAME;
    dev["ids"].add(nodeId);
    pubCfg("light", nodeId + "_light", cfg);
  }

  // Switches: animate, autoupdate
  auto publishSwitch = [&](const char* name, const String& st, const String& set, const String& id){
    JsonDocument cfg;
    cfg["name"] = name;
    cfg["uniq_id"] = id;
    cfg["cmd_t"] = set;
    cfg["stat_t"] = st;
    cfg["pl_on"] = "ON";
    cfg["pl_off"] = "OFF";
    cfg["avty_t"] = availTopic;
    JsonObject dev = cfg["dev"].to<JsonObject>();
    dev["name"] = CLOCK_NAME;
    dev["ids"].add(nodeId);
    pubCfg("switch", id, cfg);
  };
  publishSwitch("Animate words", tAnimState, tAnimSet, nodeId + String("_anim"));
  publishSwitch("Auto update", tAutoUpdState, tAutoUpdSet, nodeId + String("_autoupd"));
  publishSwitch("Night mode enabled", tNightEnabledState, tNightEnabledSet, nodeId + String("_night_enabled"));

  // Select: night mode effect
  {
    JsonDocument cfg;
    cfg["name"] = "Night mode effect";
    cfg["uniq_id"] = (nodeId + "_night_effect");
    cfg["cmd_t"] = tNightEffectSet;
    cfg["stat_t"] = tNightEffectState;
    JsonArray opts = cfg["options"].to<JsonArray>();
    opts.add("DIM");
    opts.add("OFF");
    cfg["avty_t"] = availTopic;
    JsonObject dev = cfg["dev"].to<JsonObject>();
    dev["name"] = CLOCK_NAME;
    dev["ids"].add(nodeId);
    pubCfg("select", nodeId + "_night_effect", cfg);
  }

  // Number: Night mode dim percentage
  {
    JsonDocument cfg;
    cfg["name"] = "Night mode dim %";
    cfg["uniq_id"] = (nodeId + "_night_dim");
    cfg["cmd_t"] = tNightDimSet;
    cfg["stat_t"] = tNightDimState;
    cfg["min"] = 0;
    cfg["max"] = 100;
    cfg["step"] = 1;
    cfg["unit_of_meas"] = "%";
    cfg["avty_t"] = availTopic;
    JsonObject dev = cfg["dev"].to<JsonObject>();
    dev["name"] = CLOCK_NAME;
    dev["ids"].add(nodeId);
    pubCfg("number", nodeId + "_night_dim", cfg);
  }

  // Number: Het Is duration
  {
    JsonDocument cfg;
    cfg["name"] = "'HET IS' seconds";
    cfg["uniq_id"] = (nodeId + "_hetis");
    cfg["cmd_t"] = tHetIsSet;
    cfg["stat_t"] = tHetIsState;
    cfg["min"] = 0; cfg["max"] = 360; cfg["step"] = 1;
    cfg["avty_t"] = availTopic;
    JsonObject dev = cfg["dev"].to<JsonObject>();
    dev["name"] = CLOCK_NAME;
    dev["ids"].add(nodeId);
    pubCfg("number", nodeId + "_hetis", cfg);
  }

  // Select: night mode override
  {
    JsonDocument cfg;
    cfg["name"] = "Night mode override";
    cfg["uniq_id"] = (nodeId + "_night_override");
    cfg["cmd_t"] = tNightOverrideSet;
    cfg["stat_t"] = tNightOverrideState;
    JsonArray opts = cfg["options"].to<JsonArray>();
    opts.add("AUTO");
    opts.add("ON");
    opts.add("OFF");
    cfg["avty_t"] = availTopic;
    JsonObject dev = cfg["dev"].to<JsonObject>();
    dev["name"] = CLOCK_NAME;
    dev["ids"].add(nodeId);
    pubCfg("select", nodeId + "_night_override", cfg);
  }

  // Select: log level
  {
    JsonDocument cfg;
    cfg["name"] = "Log level";
    cfg["uniq_id"] = (nodeId + "_loglevel");
    cfg["cmd_t"] = tLogLvlSet;
    cfg["stat_t"] = tLogLvlState;
    JsonArray opts = cfg["options"].to<JsonArray>();
    opts.add("DEBUG"); opts.add("INFO"); opts.add("WARN"); opts.add("ERROR");
    cfg["avty_t"] = availTopic;
    JsonObject dev = cfg["dev"].to<JsonObject>();
    dev["name"] = CLOCK_NAME;
    dev["ids"].add(nodeId);
    pubCfg("select", nodeId + "_loglevel", cfg);
  }

  // Binary sensor: Night mode active state
  {
    JsonDocument cfg;
    cfg["name"] = "Night mode active";
    cfg["uniq_id"] = (nodeId + "_night_active");
    cfg["stat_t"] = tNightActiveState;
    cfg["pl_on"] = "ON";
    cfg["pl_off"] = "OFF";
    cfg["avty_t"] = availTopic;
    JsonObject dev = cfg["dev"].to<JsonObject>();
    dev["name"] = CLOCK_NAME;
    dev["ids"].add(nodeId);
    pubCfg("binary_sensor", nodeId + "_night_active", cfg);
  }

  // Buttons: restart, start sequence, check for update
  auto publishButton = [&](const char* name, const String& cmd, const String& id){
    JsonDocument cfg;
    cfg["name"] = name;
    cfg["uniq_id"] = id;
    cfg["cmd_t"] = cmd;
    cfg["avty_t"] = availTopic;
    JsonObject dev = cfg["dev"].to<JsonObject>();
    dev["name"] = CLOCK_NAME;
    dev["ids"].add(nodeId);
    pubCfg("button", id, cfg);
  };
  publishButton("Restart", tRestartCmd, nodeId + String("_restart"));
  publishButton("Start sequence", tSeqCmd, nodeId + String("_sequence"));
  publishButton("Check for update", tUpdateCmd, nodeId + String("_update"));

  // Sensors: version, ui version, ip, rssi, startup time
  auto publishSensor = [&](const char* name, const String& st, const String& id){
    JsonDocument cfg;
    cfg["name"] = name;
    cfg["uniq_id"] = id;
    cfg["stat_t"] = st;
    cfg["avty_t"] = availTopic;
    JsonObject dev = cfg["dev"].to<JsonObject>();
    dev["name"] = CLOCK_NAME;
    dev["ids"].add(nodeId);
    pubCfg("sensor", id, cfg);
  };
  publishSensor("Firmware Version", tVersion, nodeId + String("_version"));
  publishSensor("UI Version", tUiVersion, nodeId + String("_uiversion"));
  publishSensor("IP Address", tIp, nodeId + String("_ip"));
  publishSensor("WiFi RSSI", tRssi, nodeId + String("_rssi"));
  publishSensor("Last Startup", tUptime, nodeId + String("_uptime"));
  publishSensor("Free Heap (bytes)", tHeap, nodeId + String("_heap"));
  publishSensor("WiFi Channel", tWifiChan, nodeId + String("_wifichan"));
  publishSensor("Boot Reason", tBootReason, nodeId + String("_bootreason"));
  publishSensor("Reset Count", tResetCount, nodeId + String("_resetcount"));

  auto publishText = [&](const char* name, const String& st, const String& set, const String& id){
    JsonDocument cfg;
    cfg["name"] = name;
    cfg["uniq_id"] = id;
    cfg["stat_t"] = st;
    cfg["cmd_t"] = set;
    cfg["mode"] = "text";
    cfg["min"] = 5;
    cfg["max"] = 5;
    cfg["pattern"] = "^([01][0-9]|2[0-3]):[0-5][0-9]$";
    cfg["avty_t"] = availTopic;
    JsonObject dev = cfg["dev"].to<JsonObject>();
    dev["name"] = CLOCK_NAME;
    dev["ids"].add(nodeId);
    pubCfg("text", id, cfg);
  };
  publishText("Night mode start", tNightStartState, tNightStartSet, nodeId + String("_night_start"));
  publishText("Night mode end", tNightEndState, tNightEndSet, nodeId + String("_night_end"));
}

static void publishAvailability(const char* st) {
  mqtt.publish(availTopic.c_str(), st, true);
}

static void publishLightState() {
  JsonDocument doc;
  uint8_t r, g, b, w; ledState.getRGBW(r,g,b,w);
  doc["state"] = clockEnabled ? "ON" : "OFF";
  doc["brightness"] = ledState.getBrightness();
  JsonObject col = doc["color"].to<JsonObject>();
  col["r"] = r; col["g"] = g; col["b"] = b;
  String out; serializeJson(doc, out);
  mqtt.publish(tLightState.c_str(), out.c_str(), true);
}

static void publishSwitch(const String& topic, bool on) {
  mqtt.publish(topic.c_str(), on ? "ON" : "OFF", true);
}

static void publishNumber(const String& topic, int v) {
  char buf[16]; snprintf(buf, sizeof(buf), "%d", v);
  mqtt.publish(topic.c_str(), buf, true);
}

static void publishSelect(const String& topic) {
  extern LogLevel LOG_LEVEL;
  const char* s = "INFO";
  switch (LOG_LEVEL) {
    case LOG_LEVEL_DEBUG: s = "DEBUG"; break;
    case LOG_LEVEL_INFO:  s = "INFO";  break;
    case LOG_LEVEL_WARN:  s = "WARN";  break;
    case LOG_LEVEL_ERROR: s = "ERROR"; break;
  }
  mqtt.publish(topic.c_str(), s, true);
}

static void publishNightOverrideState() {
  const char* s = "AUTO";
  switch (nightMode.getOverride()) {
    case NightModeOverride::ForceOn:  s = "ON"; break;
    case NightModeOverride::ForceOff: s = "OFF"; break;
    case NightModeOverride::Auto:
    default:                         s = "AUTO"; break;
  }
  mqtt.publish(tNightOverrideState.c_str(), s, true);
}

static void publishNightActiveState() {
  mqtt.publish(tNightActiveState.c_str(), nightMode.isActive() ? "ON" : "OFF", true);
}

static void publishNightEffectState() {
  const char* s = (nightMode.getEffect() == NightModeEffect::Off) ? "OFF" : "DIM";
  mqtt.publish(tNightEffectState.c_str(), s, true);
}

static void publishNightDimState() {
  publishNumber(tNightDimState, nightMode.getDimPercent());
}

static void publishNightScheduleState() {
  String start = nightMode.formatMinutes(nightMode.getStartMinutes());
  String end = nightMode.formatMinutes(nightMode.getEndMinutes());
  mqtt.publish(tNightStartState.c_str(), start.c_str(), true);
  mqtt.publish(tNightEndState.c_str(), end.c_str(), true);
}

// Cache computed boot time string once NTP is synced
static String g_bootTimeStr;
static bool g_bootTimeSet = false;
static String g_bootReasonStr;
static uint32_t g_resetCount = 0;
static bool mqttConfiguredLogged = false;

static bool mqtt_has_configuration() {
  if (g_mqttCfg.port == 0) return false;
  String host = g_mqttCfg.host;
  host.trim();
  return host.length() > 0;
}

static const char* reset_reason_to_str(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXTERNAL";
    case ESP_RST_SW:        return "SOFTWARE";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "UNKNOWN";
  }
}

void mqtt_publish_state(bool force) {
  unsigned long now = millis();
  if (!force && (now - lastStateAt) < STATE_INTERVAL_MS) return;
  lastStateAt = now;
  if (!mqtt.connected()) return;

  publishLightState();
  publishSwitch(tAnimState, displaySettings.getAnimateWords());
  publishSwitch(tAutoUpdState, displaySettings.getAutoUpdate());
  publishNumber(tHetIsState, displaySettings.getHetIsDurationSec());
  publishSwitch(tNightEnabledState, nightMode.isEnabled());
  publishNightEffectState();
  publishNightDimState();
  publishNightScheduleState();
  publishNightOverrideState();
  publishNightActiveState();
  publishSelect(tLogLvlState);

  // Update channel / auto-update status
  String updCh = displaySettings.getUpdateChannel();
  mqtt.publish(tUpdateChannelState.c_str(), updCh.c_str(), true);
  bool autoAllowed = displaySettings.getAutoUpdate() && updCh != "develop";
  mqtt.publish(tUpdateAutoAllowed.c_str(), autoAllowed ? "ON" : "OFF", true);
  mqtt.publish(tUpdateAvailable.c_str(), "unknown", true); // placeholder until a remote check runs

  mqtt.publish(tVersion.c_str(), FIRMWARE_VERSION, true);
  mqtt.publish(tUiVersion.c_str(), UI_VERSION, true);
  mqtt.publish(tIp.c_str(), WiFi.localIP().toString().c_str(), true);
  char rssi[16]; snprintf(rssi, sizeof(rssi), "%d", WiFi.RSSI()); mqtt.publish(tRssi.c_str(), rssi, true);
  char heap[24]; snprintf(heap, sizeof(heap), "%u", (unsigned)esp_get_free_heap_size()); mqtt.publish(tHeap.c_str(), heap, true);
  char ch[8]; snprintf(ch, sizeof(ch), "%d", WiFi.channel()); mqtt.publish(tWifiChan.c_str(), ch, true);
  if (g_bootReasonStr.length() == 0) {
    g_bootReasonStr = reset_reason_to_str(esp_reset_reason());
  }
  mqtt.publish(tBootReason.c_str(), g_bootReasonStr.c_str(), true);
  char rc[16]; snprintf(rc, sizeof(rc), "%lu", (unsigned long)g_resetCount); mqtt.publish(tResetCount.c_str(), rc, true);

  // Publish last startup timestamp (local time) once NTP is synced
  time_t nowEpoch = time(nullptr);
  if (!g_bootTimeSet && nowEpoch >= 1640995200) { // 2022-01-01 as "time is valid" threshold
    time_t boot = nowEpoch - (time_t)(millis() / 1000UL);
    struct tm lt = {};
    localtime_r(&boot, &lt);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &lt);
    g_bootTimeStr = String(buf);
    g_bootTimeSet = true;
  }
  const char* bootOut = g_bootTimeSet ? g_bootTimeStr.c_str() : "unknown";
  mqtt.publish(tUptime.c_str(), bootOut, true);
}

static void publishBirth() {
  // Publish a small JSON birth message with time and reason
  if (!mqtt.connected()) return;
  String out = String("{\"time\":\"") + (g_bootTimeSet ? g_bootTimeStr : String("unknown")) +
               "\",\"reason\":\"" + (g_bootReasonStr.length() ? g_bootReasonStr : String(reset_reason_to_str(esp_reset_reason()))) + "\"}";
  mqtt.publish(tBirth.c_str(), out.c_str(), true);
}

static void handleMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  auto is = [&](const String& t){ return strcmp(topic, t.c_str())==0; };

  if (is(tLightSet)) {
    // Expect JSON payload {state, brightness, color:{r,g,b}}
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, msg);
    if (!err) {
      if (doc["state"].is<const char*>()) {
        const char* st = doc["state"];
        clockEnabled = (strcmp(st, "ON")==0);
      }
      if (doc["brightness"].is<int>()) {
        int br = doc["brightness"].as<int>(); br = constrain(br, 0, 255); ledState.setBrightness(br);
      }
      if (doc["color"].is<JsonObject>()) {
        uint8_t r = doc["color"]["r"] | 0; uint8_t g = doc["color"]["g"] | 0; uint8_t b = doc["color"]["b"] | 0;
        ledState.setRGB(r,g,b);
      }
      // Apply display immediately
      struct tm timeinfo; if (getLocalTime(&timeinfo)) { auto idx = get_led_indices_for_time(&timeinfo); showLeds(idx); }
      publishLightState();
    }
  } else if (is(tClockSet)) {
    bool on = (msg == "ON" || msg == "on" || msg == "1");
    clockEnabled = on;
    publishSwitch(tClockState, on);
  } else if (is(tAnimSet)) {
    bool on = (msg == "ON" || msg == "on" || msg == "1");
    displaySettings.setAnimateWords(on);
    publishSwitch(tAnimState, on);
  } else if (is(tAutoUpdSet)) {
    bool on = (msg == "ON" || msg == "on" || msg == "1");
    displaySettings.setAutoUpdate(on);
    publishSwitch(tAutoUpdState, on);
  } else if (is(tHetIsSet)) {
    int v = msg.toInt(); v = constrain(v, 0, 360); displaySettings.setHetIsDurationSec((uint16_t)v);
    publishNumber(tHetIsState, v);
  } else if (is(tNightEnabledSet)) {
    bool on = (msg == "ON" || msg == "on" || msg == "1" || msg == "true" || msg == "True");
    nightMode.setEnabled(on);
    publishSwitch(tNightEnabledState, nightMode.isEnabled());
  } else if (is(tNightOverrideSet)) {
    String lower = msg;
    lower.toUpperCase();
    if (lower == "AUTO") {
      nightMode.setOverride(NightModeOverride::Auto);
    } else if (lower == "ON") {
      nightMode.setOverride(NightModeOverride::ForceOn);
    } else if (lower == "OFF") {
      nightMode.setOverride(NightModeOverride::ForceOff);
    } else {
      logWarn(String("MQTT night override invalid payload: ") + msg);
      return;
    }
    publishNightOverrideState();
    publishNightActiveState();
  } else if (is(tNightEffectSet)) {
    String eff = msg;
    eff.toUpperCase();
    if (eff == "OFF") {
      nightMode.setEffect(NightModeEffect::Off);
    } else if (eff == "DIM") {
      nightMode.setEffect(NightModeEffect::Dim);
    } else {
      logWarn(String("MQTT night effect invalid payload: ") + msg);
      return;
    }
    publishNightEffectState();
  } else if (is(tNightDimSet)) {
    int pct = msg.toInt();
    pct = constrain(pct, 0, 100);
    nightMode.setDimPercent((uint8_t)pct);
    publishNightDimState();
  } else if (is(tNightStartSet)) {
    String startStr = msg;
    uint16_t minutes = 0;
    if (!NightMode::parseTimeString(startStr, minutes)) {
      logWarn(String("MQTT night start invalid payload: ") + msg);
      return;
    }
    nightMode.setSchedule(minutes, nightMode.getEndMinutes());
    publishNightScheduleState();
  } else if (is(tNightEndSet)) {
    String endStr = msg;
    uint16_t minutes = 0;
    if (!NightMode::parseTimeString(endStr, minutes)) {
      logWarn(String("MQTT night end invalid payload: ") + msg);
      return;
    }
    nightMode.setSchedule(nightMode.getStartMinutes(), minutes);
    publishNightScheduleState();
  } else if (is(tLogLvlSet)) {
    LogLevel level = LOG_LEVEL_INFO;
    if (msg == "DEBUG") level = LOG_LEVEL_DEBUG; else if (msg == "INFO") level = LOG_LEVEL_INFO; else if (msg == "WARN") level = LOG_LEVEL_WARN; else if (msg == "ERROR") level = LOG_LEVEL_ERROR;
    setLogLevel(level);
    publishSelect(tLogLvlState);
  } else if (is(tRestartCmd)) {
    safeRestart();
  } else if (is(tSeqCmd)) {
    extern StartupSequence startupSequence; startupSequence.start();
  } else if (is(tUpdateCmd)) {
    checkForFirmwareUpdate();
  }
}

static bool mqtt_connect() {
  if (mqtt.connected()) return true;
  if (WiFi.status() != WL_CONNECTED) {
    g_lastErr = "WiFi not connected";
    return false;
  }
  if (!mqtt_has_configuration()) {
    g_lastErr = "MQTT not configured";
    return false; // not configured yet
  }

  // Compute unique id based on MAC
  if (uniqId.isEmpty()) {
    uint8_t mac[6]; WiFi.macAddress(mac);
    char buf[13]; snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    uniqId = String("wordclock_") + buf;
    buildTopics();
  }

  String clientId = uniqId;
  bool ok;
  if (g_mqttCfg.user.length() > 0) {
    ok = mqtt.connect(clientId.c_str(), g_mqttCfg.user.c_str(), g_mqttCfg.pass.c_str(), availTopic.c_str(), 1, true, "offline");
  } else {
    ok = mqtt.connect(clientId.c_str());
  }
  if (!ok) {
    int st = mqtt.state();
    g_connected = false;
    g_lastErr = String("connect failed (state ") + st + ")";
    return false;
  }

  publishAvailability("online");
  publishBirth();
  publishDiscovery();

  // Subscriptions
  mqtt.subscribe(tLightSet.c_str());
  mqtt.subscribe(tClockSet.c_str());
  mqtt.subscribe(tAnimSet.c_str());
  mqtt.subscribe(tAutoUpdSet.c_str());
  mqtt.subscribe(tHetIsSet.c_str());
  mqtt.subscribe(tNightEnabledSet.c_str());
  mqtt.subscribe(tNightOverrideSet.c_str());
  mqtt.subscribe(tNightEffectSet.c_str());
  mqtt.subscribe(tNightDimSet.c_str());
  mqtt.subscribe(tNightStartSet.c_str());
  mqtt.subscribe(tNightEndSet.c_str());
  mqtt.subscribe(tLogLvlSet.c_str());
  mqtt.subscribe(tRestartCmd.c_str());
  mqtt.subscribe(tSeqCmd.c_str());
  mqtt.subscribe(tUpdateCmd.c_str());

  mqtt_publish_state(true);
  g_connected = true;
  
  // Reset reconnection state on success
  reconnectAttempts = 0;
  reconnectDelayMs = RECONNECT_DELAY_MIN_MS;
  reconnectAborted = false;
  
  // Log successful recovery if there was a previous error
  if (g_lastErr.length() > 0) {
    logInfo(String("✅ MQTT reconnected successfully after error: ") + g_lastErr);
  }
  g_lastErr = "";
  
  return true;
}

void mqtt_begin() {
  // Load saved settings or fall back to compile-time defaults
  mqtt_settings_load(g_mqttCfg);
  mqtt.setServer(g_mqttCfg.host.c_str(), g_mqttCfg.port);
  mqtt.setCallback(handleMessage);
  reconnectDelayMs = RECONNECT_DELAY_MIN_MS;
  reconnectAttempts = 0;
  reconnectAborted = false;
  lastReconnectAttempt = 0;
  // Bump reset counter (persisted), and cache boot reason string
  g_bootReasonStr = reset_reason_to_str(esp_reset_reason());
  Preferences p;
  if (p.begin("sys", false)) {
    uint32_t cnt = p.getULong("resets", 0);
    cnt += 1;
    p.putULong("resets", cnt);
    p.end();
    g_resetCount = cnt;
  }

  if (!mqtt_has_configuration() && !mqttConfiguredLogged) {
    logInfo("MQTT disabled (no broker configured)");
    mqttConfiguredLogged = true;
  } else {
    mqttConfiguredLogged = false;
  }
}

void mqtt_loop() {
  if (!mqtt_has_configuration()) {
    g_connected = false;
    if (!mqttConfiguredLogged) {
      logInfo("MQTT disabled (no broker configured)");
      mqttConfiguredLogged = true;
    }
    return;
  }
  mqttConfiguredLogged = false;
  if (reconnectAborted) return;

  if (!mqtt.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= reconnectDelayMs) {
      lastReconnectAttempt = now;
      if (!mqtt_connect()) {
        g_connected = false;
        if (reconnectDelayMs < RECONNECT_DELAY_MIN_MS) reconnectDelayMs = RECONNECT_DELAY_MIN_MS;
        if (reconnectAttempts < 255) reconnectAttempts++;
        unsigned long nextDelay = reconnectDelayMs * 2;
        if (nextDelay > RECONNECT_DELAY_MAX_MS) nextDelay = RECONNECT_DELAY_MAX_MS;
        uint32_t jitter = esp_random() % RECONNECT_DELAY_MIN_MS;
        unsigned long jittered = nextDelay + jitter;
        if (jittered > RECONNECT_DELAY_MAX_MS) jittered = RECONNECT_DELAY_MAX_MS;
        reconnectDelayMs = jittered;
        if (!reconnectAborted && reconnectDelayMs >= RECONNECT_DELAY_MAX_MS) {
          String errMsg = String("⏸️ MQTT reconnect paused after reaching max backoff (") +
                          RECONNECT_DELAY_MAX_MS + " ms); last error: " +
                          (g_lastErr.length() ? g_lastErr : String("unknown")) +
                          String(". Will retry on network recovery, config change, or manual reconnect.");
          logWarn(errMsg);
          reconnectAborted = true;
          // Note: reconnectAborted will be cleared on:
          // 1. Successful connection (mqtt_connect success path)
          // 2. Configuration change (mqtt_apply_settings)
          // 3. Manual reconnect (mqtt_force_reconnect)
          return;
        } else if (g_lastErr != "MQTT not configured") {
          String warnMsg = String("MQTT reconnect failed (") + (g_lastErr.length() ? g_lastErr : String("unknown")) +
                           "); retry in " + reconnectDelayMs + " ms";
          logWarn(warnMsg);
        }
      }
    }
    return;
  }
  mqtt.loop();
  mqtt_publish_state(false);
}

bool mqtt_is_connected() {
  return g_connected && mqtt.connected();
}

const String& mqtt_last_error() {
  return g_lastErr;
}

void mqtt_apply_settings(const MqttSettings& s) {
  // Persist, then apply live
  MqttSettings toSave = s;
  if (!mqtt_settings_save(toSave)) {
    logError("❌ Failed to save MQTT settings");
    return;
  }
  // Update current config
  g_mqttCfg = toSave;

  // Disconnect and reconfigure server and topics
  if (mqtt.connected()) mqtt.disconnect();
  mqtt.setServer(g_mqttCfg.host.c_str(), g_mqttCfg.port);

  // Recompute topics based on new base/discovery
  buildTopics();
  
  // Reset reconnection state and enable reconnection attempts
  if (reconnectAborted) {
    logInfo("🔄 MQTT reconnection re-enabled after configuration change");
  }
  reconnectDelayMs = RECONNECT_DELAY_MIN_MS;
  reconnectAttempts = 0;
  reconnectAborted = false;
  lastReconnectAttempt = 0; // trigger immediate reconnect in loop

  if (!mqtt_has_configuration()) {
    if (!mqttConfiguredLogged) {
      logInfo("MQTT disabled (no broker configured)");
      mqttConfiguredLogged = true;
    }
  } else {
    mqttConfiguredLogged = false;
  }
}

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
    logInfo("🔄 MQTT reconnection re-enabled by force reconnect");
  }
  
  // Reset all reconnection state
  reconnectAborted = false;
  reconnectAttempts = 0;
  reconnectDelayMs = RECONNECT_DELAY_MIN_MS;
  lastReconnectAttempt = 0;  // Trigger immediate attempt in next mqtt_loop()
  
  g_lastErr = "";
}
