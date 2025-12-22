#pragma once
#include <Preferences.h>

#include "grid_layout.h"

constexpr GridVariant FIRMWARE_DEFAULT_GRID_VARIANT = GridVariant::NL_V4;

class DisplaySettings {
public:
  void begin() {
    prefs.begin("display", false);
    hetIsDurationSec = prefs.getUShort("his_sec", 360); // default ALWAYS (360s)
    if (hetIsDurationSec > 360) hetIsDurationSec = 360;
    sellMode = prefs.getBool("sell_on", false);
    animateWords = prefs.getBool("anim_on", false); // default OFF unless enabled via UI
    autoUpdate = prefs.getBool("auto_upd", true);
    updateChannel = prefs.getString("upd_ch", "stable");
    if (updateChannel != "early") updateChannel = "stable";
    const uint8_t defaultVariantId = gridVariantToId(FIRMWARE_DEFAULT_GRID_VARIANT);
    const bool hasGridKey = prefs.isKey("grid_id");
    // Track whether a grid variant was already stored so we can detect migrations
    if (!initialized) {
      hasStoredVariant = hasGridKey;
    }
    uint8_t storedVariant = prefs.getUChar("grid_id", defaultVariantId);
    if (!hasGridKey) {
      prefs.putUChar("grid_id", defaultVariantId);
      storedVariant = defaultVariantId;
    }
    prefs.end();

    gridVariant = gridVariantFromId(storedVariant);
    if (!setActiveGridVariant(gridVariant)) {
      gridVariant = FIRMWARE_DEFAULT_GRID_VARIANT;
      setActiveGridVariant(gridVariant);
      prefs.begin("display", false);
      prefs.putUChar("grid_id", defaultVariantId);
      prefs.end();
    }
    initialized = true;
  }

  uint16_t getHetIsDurationSec() const { return hetIsDurationSec; }
  bool isSellMode() const { return sellMode; }
  bool getAnimateWords() const { return animateWords; }
  bool getAutoUpdate() const { return autoUpdate; }
  String getUpdateChannel() const { return updateChannel; }
  GridVariant getGridVariant() const { return gridVariant; }
  uint8_t getGridVariantId() const { return gridVariantToId(gridVariant); }
  bool hasPersistedGridVariant() const { return hasStoredVariant; }

  void setHetIsDurationSec(uint16_t s) {
    if (s > 360) s = 360;
    hetIsDurationSec = s;
    prefs.begin("display", false);
    prefs.putUShort("his_sec", hetIsDurationSec);
    prefs.end();
  }
  void setSellMode(bool on) {
    sellMode = on;
    prefs.begin("display", false);
    prefs.putBool("sell_on", sellMode);
    prefs.end();
  }

  void setAnimateWords(bool on) {
    animateWords = on;
    prefs.begin("display", false);
    prefs.putBool("anim_on", animateWords);
    prefs.end();
  }

  void setAutoUpdate(bool on) {
    autoUpdate = on;
    prefs.begin("display", false);
    prefs.putBool("auto_upd", autoUpdate);
    prefs.end();
  }

  void setUpdateChannel(const String& ch) {
    String v = ch;
    v.toLowerCase();
    if (v != "early") v = "stable";
    updateChannel = v;
    prefs.begin("display", false);
    prefs.putString("upd_ch", updateChannel);
    prefs.end();
  }

  void setGridVariant(GridVariant variant) {
    if (!setActiveGridVariant(variant)) {
      return;
    }
    gridVariant = variant;
    prefs.begin("display", false);
    prefs.putUChar("grid_id", gridVariantToId(gridVariant));
    prefs.end();
  }

  void setGridVariantById(uint8_t id) {
    size_t count = 0;
    getGridVariantInfos(count);
    if (id >= count) {
      return;
    }
    setGridVariant(gridVariantFromId(id));
  }

private:
  uint16_t hetIsDurationSec = 360; // default ALWAYS
  bool sellMode = false;
  bool animateWords = false; // default OFF
  bool autoUpdate = true;    // default ON to keep current behavior
  String updateChannel = "stable"; // stable | early
  GridVariant gridVariant = FIRMWARE_DEFAULT_GRID_VARIANT;
  bool hasStoredVariant = false;
  bool initialized = false;
  Preferences prefs;
};

extern DisplaySettings displaySettings;
