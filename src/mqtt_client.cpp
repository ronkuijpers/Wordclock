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

extern DisplaySettings displaySettings;
extern bool clockEnabled;
extern StartupSequence startupSequence;

static WiFiClient espClient;
static PubSubClient mqtt(espClient);

static String uniqId;
static MqttSettings g_mqttCfg;
static String base;         // e.g., MQTT_BASE_TOPIC
static String availTopic;   // base + "/availability"
static bool g_connected = false;
static String g_lastErr;

// Topics
static String tLightState, tLightSet;
static String tClockState, tClockSet;
static String tAnimState, tAnimSet;
static String tAutoUpdState, tAutoUpdSet;
static String tSellState, tSellSet;
static String tHetIsState, tHetIsSet;
static String tLogLvlState, tLogLvlSet;
static String tRestartCmd, tSeqCmd, tUpdateCmd;
static String tVersion, tUiVersion, tIp, tRssi, tUptime;

static unsigned long lastReconnectAttempt = 0;
static unsigned long lastStateAt = 0;
static const unsigned long STATE_INTERVAL_MS = 30000; // 30s

static void buildTopics() {
  base = g_mqttCfg.baseTopic;
  availTopic = base + "/availability";
  tLightState   = base + "/light/state";
  tLightSet     = base + "/light/set";
  tClockState   = base + "/clock/state";
  tClockSet     = base + "/clock/set";
  tAnimState    = base + "/animate/state";
  tAnimSet      = base + "/animate/set";
  tAutoUpdState = base + "/autoupdate/state";
  tAutoUpdSet   = base + "/autoupdate/set";
  tSellState    = base + "/sell/state";
  tSellSet      = base + "/sell/set";
  tHetIsState   = base + "/hetis/state";
  tHetIsSet     = base + "/hetis/set";
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

  // Switches: animate, autoupdate, sellmode
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
  publishSwitch("Sell mode", tSellState, tSellSet, nodeId + String("_sell"));

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

// Cache computed boot time string once NTP is synced
static String g_bootTimeStr;
static bool g_bootTimeSet = false;

void mqtt_publish_state(bool force) {
  unsigned long now = millis();
  if (!force && (now - lastStateAt) < STATE_INTERVAL_MS) return;
  lastStateAt = now;
  if (!mqtt.connected()) return;

  publishLightState();
  publishSwitch(tAnimState, displaySettings.getAnimateWords());
  publishSwitch(tAutoUpdState, displaySettings.getAutoUpdate());
  publishSwitch(tSellState, displaySettings.isSellMode());
  publishNumber(tHetIsState, displaySettings.getHetIsDurationSec());
  publishSelect(tLogLvlState);

  mqtt.publish(tVersion.c_str(), FIRMWARE_VERSION, true);
  mqtt.publish(tUiVersion.c_str(), UI_VERSION, true);
  mqtt.publish(tIp.c_str(), WiFi.localIP().toString().c_str(), true);
  char rssi[16]; snprintf(rssi, sizeof(rssi), "%d", WiFi.RSSI()); mqtt.publish(tRssi.c_str(), rssi, true);

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
  } else if (is(tSellSet)) {
    bool on = (msg == "ON" || msg == "on" || msg == "1");
    displaySettings.setSellMode(on);
    publishSwitch(tSellState, on);
  } else if (is(tHetIsSet)) {
    int v = msg.toInt(); v = constrain(v, 0, 360); displaySettings.setHetIsDurationSec((uint16_t)v);
    publishNumber(tHetIsState, v);
  } else if (is(tLogLvlSet)) {
    LogLevel level = LOG_LEVEL_INFO;
    if (msg == "DEBUG") level = LOG_LEVEL_DEBUG; else if (msg == "INFO") level = LOG_LEVEL_INFO; else if (msg == "WARN") level = LOG_LEVEL_WARN; else if (msg == "ERROR") level = LOG_LEVEL_ERROR;
    setLogLevel(level);
    publishSelect(tLogLvlState);
  } else if (is(tRestartCmd)) {
    ESP.restart();
  } else if (is(tSeqCmd)) {
    extern StartupSequence startupSequence; startupSequence.start();
  } else if (is(tUpdateCmd)) {
    checkForFirmwareUpdate();
  }
}

static bool mqtt_connect() {
  if (mqtt.connected()) return true;
  if (WiFi.status() != WL_CONNECTED) return false;
  if (g_mqttCfg.host.length() == 0 || g_mqttCfg.port == 0) return false; // not configured yet

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
  publishDiscovery();

  // Subscriptions
  mqtt.subscribe(tLightSet.c_str());
  mqtt.subscribe(tClockSet.c_str());
  mqtt.subscribe(tAnimSet.c_str());
  mqtt.subscribe(tAutoUpdSet.c_str());
  mqtt.subscribe(tSellSet.c_str());
  mqtt.subscribe(tHetIsSet.c_str());
  mqtt.subscribe(tLogLvlSet.c_str());
  mqtt.subscribe(tRestartCmd.c_str());
  mqtt.subscribe(tSeqCmd.c_str());
  mqtt.subscribe(tUpdateCmd.c_str());

  mqtt_publish_state(true);
  g_connected = true;
  g_lastErr = "";
  return true;
}

void mqtt_begin() {
  // Load saved settings or fall back to compile-time defaults
  mqtt_settings_load(g_mqttCfg);
  mqtt.setServer(g_mqttCfg.host.c_str(), g_mqttCfg.port);
  mqtt.setCallback(handleMessage);
}

void mqtt_loop() {
  if (!mqtt.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 2000) {
      lastReconnectAttempt = now;
      if (!mqtt_connect()) {
        g_connected = false;
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
  lastReconnectAttempt = 0; // trigger immediate reconnect in loop
}
