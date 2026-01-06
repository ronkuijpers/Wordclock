#pragma once
#include <Preferences.h>

#include "grid_layout.h"
#include "log.h"

constexpr GridVariant FIRMWARE_DEFAULT_GRID_VARIANT = GridVariant::NL_V4;
enum class WordAnimationMode : uint8_t { Classic = 0, Smart = 1 };
enum class AnimationSpeed : uint8_t {
  Slow = 0,      // 1000ms per frame
  Normal = 1,    // 500ms per frame (default)
  Fast = 2,      // 250ms per frame
  Custom = 3     // User-defined milliseconds
};
enum class AnimationDirection : uint8_t {
  LeftToRight = 0,   // Default
  RightToLeft = 1,
  TopToBottom = 2,
  BottomToTop = 3,
  CenterOut = 4,     // Spiral from center
  Random = 5
};

class DisplaySettings {
public:
  void begin() {
    prefs_.begin("wc_display", false);  // Note: renamed namespace for safety
    hetIsDurationSec_ = prefs_.getUShort("his_sec", 360); // default ALWAYS (360s)
    if (hetIsDurationSec_ > 360) hetIsDurationSec_ = 360;
    sellMode_ = prefs_.getBool("sell_on", false);
    animateWords_ = prefs_.getBool("anim_on", false); // default OFF unless enabled via UI
    uint8_t storedAnimMode = prefs_.getUChar("anim_mode", static_cast<uint8_t>(WordAnimationMode::Classic));
    if (storedAnimMode > static_cast<uint8_t>(WordAnimationMode::Smart)) {
      storedAnimMode = static_cast<uint8_t>(WordAnimationMode::Classic);
    }
    animationMode_ = static_cast<WordAnimationMode>(storedAnimMode);
    
    // Animation speed (new feature)
    uint8_t storedSpeed = prefs_.getUChar("anim_speed", static_cast<uint8_t>(AnimationSpeed::Normal));
    if (storedSpeed > static_cast<uint8_t>(AnimationSpeed::Custom)) {
      storedSpeed = static_cast<uint8_t>(AnimationSpeed::Normal);
    }
    animationSpeed_ = static_cast<AnimationSpeed>(storedSpeed);
    customSpeedMs_ = prefs_.getUShort("anim_speed_ms", 500);
    if (customSpeedMs_ < 100) customSpeedMs_ = 100;
    if (customSpeedMs_ > 2000) customSpeedMs_ = 2000;
    
    // Animation direction (new feature)
    uint8_t storedDir = prefs_.getUChar("anim_dir", static_cast<uint8_t>(AnimationDirection::LeftToRight));
    if (storedDir > static_cast<uint8_t>(AnimationDirection::Random)) {
      storedDir = static_cast<uint8_t>(AnimationDirection::LeftToRight);
    }
    animationDirection_ = static_cast<AnimationDirection>(storedDir);
    autoUpdate_ = prefs_.getBool("auto_upd", true);
    const uint8_t defaultVariantId = gridVariantToId(FIRMWARE_DEFAULT_GRID_VARIANT);
    const bool hasGridKey = prefs_.isKey("grid_id");
    // Track whether a grid variant was already stored so we can detect migrations
    if (!initialized_) {
      hasStoredVariant_ = hasGridKey;
    }
    uint8_t storedVariant = prefs_.getUChar("grid_id", defaultVariantId);
    if (!hasGridKey) {
      prefs_.putUChar("grid_id", defaultVariantId);
      storedVariant = defaultVariantId;
    }
    prefs_.end();

    gridVariant_ = gridVariantFromId(storedVariant);
    if (!setActiveGridVariant(gridVariant_)) {
      gridVariant_ = FIRMWARE_DEFAULT_GRID_VARIANT;
      setActiveGridVariant(gridVariant_);
      prefs_.begin("wc_display", false);
      prefs_.putUChar("grid_id", defaultVariantId);
      prefs_.end();
    }

    // Update channel (stable by default)
    prefs_.begin("wc_display", true);
    hasStoredUpdateChannel_ = prefs_.isKey("upd_ch");
    String ch = hasStoredUpdateChannel_ ? prefs_.getString("upd_ch", "stable") : String("stable");
    prefs_.end();
    ch.toLowerCase();
    if (ch != "stable" && ch != "early" && ch != "develop") ch = "stable";
    updateChannel_ = ch;
    if (updateChannel_ == "develop" && autoUpdate_) {
      autoUpdate_ = false;
      prefs_.begin("wc_display", false);
      prefs_.putBool("auto_upd", autoUpdate_);
      prefs_.end();
      logInfo("🔁 Automatic updates disabled for develop channel");
    }
    initialized_ = true;
    
    dirty_ = false;
    lastFlush_ = millis();
  }

  uint16_t getHetIsDurationSec() const { return hetIsDurationSec_; }
  bool isSellMode() const { return sellMode_; }
  bool getAnimateWords() const { return animateWords_; }
  WordAnimationMode getAnimationMode() const { return animationMode_; }
  uint8_t getAnimationModeId() const { return static_cast<uint8_t>(animationMode_); }
  AnimationSpeed getAnimationSpeed() const { return animationSpeed_; }
  uint8_t getAnimationSpeedId() const { return static_cast<uint8_t>(animationSpeed_); }
  uint16_t getAnimationSpeedMs() const {
    switch (animationSpeed_) {
      case AnimationSpeed::Slow: return 1000;
      case AnimationSpeed::Normal: return 500;
      case AnimationSpeed::Fast: return 250;
      case AnimationSpeed::Custom: return customSpeedMs_;
      default: return 500;
    }
  }
  uint16_t getCustomSpeedMs() const { return customSpeedMs_; }
  AnimationDirection getAnimationDirection() const { return animationDirection_; }
  uint8_t getAnimationDirectionId() const { return static_cast<uint8_t>(animationDirection_); }
  bool getAutoUpdate() const { return autoUpdate_; }
  String getUpdateChannel() const { return updateChannel_; }
  bool hasStoredChannel() const { return hasStoredUpdateChannel_; }
  GridVariant getGridVariant() const { return gridVariant_; }
  uint8_t getGridVariantId() const { return gridVariantToId(gridVariant_); }
  bool hasPersistedGridVariant() const { return hasStoredVariant_; }

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

  void setAnimateWords(bool on) {
    if (animateWords_ == on) return;
    animateWords_ = on;
    markDirty();
  }

  void setAnimationMode(WordAnimationMode mode) {
    if (animationMode_ == mode) return;
    animationMode_ = mode;
    markDirty();
  }

  void setAnimationModeById(uint8_t id) {
    if (id > static_cast<uint8_t>(WordAnimationMode::Smart)) {
      id = static_cast<uint8_t>(WordAnimationMode::Classic);
    }
    setAnimationMode(static_cast<WordAnimationMode>(id));
  }

  void setAnimationSpeed(AnimationSpeed speed) {
    if (animationSpeed_ == speed) return;
    animationSpeed_ = speed;
    markDirty();
  }

  void setAnimationSpeedById(uint8_t id) {
    if (id > static_cast<uint8_t>(AnimationSpeed::Custom)) {
      id = static_cast<uint8_t>(AnimationSpeed::Normal);
    }
    setAnimationSpeed(static_cast<AnimationSpeed>(id));
  }

  void setCustomSpeedMs(uint16_t ms) {
    if (ms < 100) ms = 100;
    if (ms > 2000) ms = 2000;
    if (customSpeedMs_ == ms) return;
    customSpeedMs_ = ms;
    markDirty();
  }

  void setAnimationDirection(AnimationDirection direction) {
    if (animationDirection_ == direction) return;
    animationDirection_ = direction;
    markDirty();
  }

  void setAnimationDirectionById(uint8_t id) {
    if (id > static_cast<uint8_t>(AnimationDirection::Random)) {
      id = static_cast<uint8_t>(AnimationDirection::LeftToRight);
    }
    setAnimationDirection(static_cast<AnimationDirection>(id));
  }

  void setAutoUpdate(bool on) {
    if (autoUpdate_ == on) return;
    autoUpdate_ = on;
    markDirty();
  }

  void setUpdateChannel(const String& channel) {
    String ch = channel;
    ch.toLowerCase();
    if (ch != "stable" && ch != "early" && ch != "develop") ch = "stable"; // default/fallback
    if (ch == updateChannel_) return;
    updateChannel_ = ch;
    markDirty();
    logInfo(String("🔀 Update channel set to ") + updateChannel_);
    if (updateChannel_ == "develop" && autoUpdate_) {
      autoUpdate_ = false;
      markDirty();
      logInfo("🔁 Automatic updates disabled for develop channel");
    }
  }

  void resetUpdateChannel() {
    setUpdateChannel("stable");
    hasStoredUpdateChannel_ = false;
  }

  void setGridVariant(GridVariant variant) {
    if (!setActiveGridVariant(variant)) {
      return;
    }
    if (gridVariant_ == variant) return;
    gridVariant_ = variant;
    markDirty();
  }

  void setGridVariantById(uint8_t id) {
    size_t count = 0;
    getGridVariantInfos(count);
    if (id >= count) {
      return;
    }
    setGridVariant(gridVariantFromId(id));
  }

  /**
   * @brief Force immediate write to persistent storage
   * @note Call before critical operations (OTA, deep sleep, restart)
   */
  void flush() {
    if (!dirty_) return;
    
    prefs_.begin("wc_display", false);
    // Batch write all settings
    prefs_.putUShort("his_sec", hetIsDurationSec_);
    prefs_.putBool("sell_on", sellMode_);
    prefs_.putBool("anim_on", animateWords_);
    prefs_.putUChar("anim_mode", static_cast<uint8_t>(animationMode_));
    prefs_.putUChar("anim_speed", static_cast<uint8_t>(animationSpeed_));
    prefs_.putUShort("anim_speed_ms", customSpeedMs_);
    prefs_.putUChar("anim_dir", static_cast<uint8_t>(animationDirection_));
    prefs_.putBool("auto_upd", autoUpdate_);
    prefs_.putString("upd_ch", updateChannel_);
    prefs_.putUChar("grid_id", gridVariantToId(gridVariant_));
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

  // Query persistence state
  bool isDirty() const { return dirty_; }
  unsigned long millisSinceLastFlush() const { 
    return millis() - lastFlush_; 
  }

private:
  void markDirty() {
    if (!dirty_) {
      dirty_ = true;
      lastFlush_ = millis();
    }
  }

  uint16_t hetIsDurationSec_ = 360; // default ALWAYS
  bool sellMode_ = false;
  bool animateWords_ = false; // default OFF
  WordAnimationMode animationMode_ = WordAnimationMode::Classic;
  AnimationSpeed animationSpeed_ = AnimationSpeed::Normal; // default Normal (500ms)
  uint16_t customSpeedMs_ = 500; // For Custom speed mode
  AnimationDirection animationDirection_ = AnimationDirection::LeftToRight; // default Left-to-Right
  bool autoUpdate_ = true;    // default ON to keep current behavior
  String updateChannel_ = "stable";
  GridVariant gridVariant_ = FIRMWARE_DEFAULT_GRID_VARIANT;
  bool hasStoredVariant_ = false;
  bool hasStoredUpdateChannel_ = false;
  bool initialized_ = false;
  bool dirty_ = false;
  unsigned long lastFlush_ = 0;
  
  Preferences prefs_;
  
  static const unsigned long AUTO_FLUSH_DELAY_MS = 5000;  // 5 seconds
};

extern DisplaySettings displaySettings;
