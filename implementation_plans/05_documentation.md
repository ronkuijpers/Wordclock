# Implementation Plan: Add Comprehensive Documentation

**Priority:** MEDIUM  
**Estimated Effort:** 20 hours  
**Complexity:** Low  
**Risk:** Low  
**Impact:** MEDIUM - Improves onboarding, maintainability, and community adoption

---

## Problem Statement

The codebase currently lacks comprehensive documentation, creating barriers for:

1. **New Contributors:** Steep learning curve without architectural overview
2. **Maintenance:** Difficult to understand complex interactions
3. **Deployment:** No clear setup/configuration guide
4. **Community Adoption:** Harder for users to self-serve
5. **API Understanding:** No reference for function contracts

**Current State:**
- ❌ No architecture documentation
- ❌ Mixed language comments (Dutch/English)
- ❌ No API reference (Doxygen)
- ❌ Limited inline documentation
- ❌ No developer onboarding guide
- ❌ No deployment/troubleshooting guide

**Target State:**
- ✅ Complete architecture documentation
- ✅ Standardized English documentation
- ✅ Generated API reference (Doxygen)
- ✅ Comprehensive inline comments
- ✅ Developer setup guide
- ✅ User deployment guide
- ✅ Troubleshooting reference

---

## Documentation Structure

```
workspace/
├── README.md                        # Project overview (enhanced)
├── docs/
│   ├── ARCHITECTURE.md             # System architecture
│   ├── API_REFERENCE.md            # Generated API docs link
│   ├── DEVELOPMENT.md              # Developer guide
│   ├── DEPLOYMENT.md               # User setup guide
│   ├── MQTT_INTEGRATION.md         # Home Assistant setup
│   ├── LED_MAPPING.md              # Grid variant system
│   ├── TROUBLESHOOTING.md          # Common issues
│   ├── CONTRIBUTING.md             # Contribution guidelines
│   ├── CHANGELOG.md                # Release notes
│   ├── images/                     # Diagrams and screenshots
│   │   ├── architecture.png
│   │   ├── state_machine.png
│   │   ├── led_mapping.png
│   │   └── ...
│   └── api/                        # Generated Doxygen output
│       └── html/
├── CODE_REVIEW.md                  # This review document
├── implementation_plans/           # Implementation plans
│   ├── 01_mqtt_reconnect_fix.md
│   ├── 02_preferences_optimization.md
│   ├── 03_unit_testing.md
│   ├── 04_function_refactoring.md
│   └── 05_documentation.md
├── Doxyfile                        # Doxygen configuration
└── .github/
    ├── ISSUE_TEMPLATE/
    │   ├── bug_report.md
    │   ├── feature_request.md
    │   └── question.md
    └── PULL_REQUEST_TEMPLATE.md
```

---

## Phase 1: Enhanced README

**File:** `README.md` (update)

```markdown
# Word Clock Firmware

[![Build Status](https://github.com/user/wordclock/workflows/build/badge.svg)](https://github.com/user/wordclock/actions)
[![Tests](https://github.com/user/wordclock/workflows/tests/badge.svg)](https://github.com/user/wordclock/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-26.1.5--rc.1-green.svg)](https://github.com/user/wordclock/releases)

ESP32-based word clock firmware with extensive IoT capabilities.

![Word Clock Demo](docs/images/wordclock_demo.jpg)

## Features

### Display
- 🌐 **Multi-language support:** Dutch, English (extensible)
- 📐 **Multiple grid variants:** 10x10, 20x20, 50x50
- 🎨 **RGBW LEDs:** Full color customization
- ✨ **Smooth animations:** Classic and smart animation modes
- 🌙 **Night mode:** Scheduled dimming or display off

### Connectivity
- 📡 **WiFi:** WPA2 with configuration portal
- 🏠 **Home Assistant:** Full MQTT discovery integration
- 🔄 **OTA Updates:** Wireless firmware updates
- 🌐 **Web UI:** Complete configuration interface
- 📝 **Logging:** File-based logs with retention

### Advanced
- ⏰ **NTP Time Sync:** Automatic timezone support
- 🔐 **Authentication:** Web UI and MQTT security
- 🎯 **Sell Mode:** Demo display (10:47)
- 📊 **Diagnostics:** System health monitoring
- 🔧 **Customizable:** Extensive configuration options

## Quick Start

### Hardware Requirements
- ESP32 development board
- WS2812B or SK6812 RGBW LED strip
- 5V power supply (3-5A depending on LED count)
- Optional: Light sensor for auto-brightness

### Software Requirements
- [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
- USB cable for initial programming

### Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/user/wordclock.git
   cd wordclock
   ```

2. **Configure:**
   ```bash
   # Copy example configuration
   cp src/secrets.h.example src/secrets.h
   # Edit secrets.h with your settings
   ```

3. **Build and upload:**
   ```bash
   # Using PlatformIO
   pio run -t upload
   
   # Upload filesystem
   pio run -t uploadfs
   ```

4. **Connect:**
   - Connect to `Wordclock_AP` WiFi network
   - Navigate to http://192.168.4.1
   - Configure WiFi credentials
   - Clock will restart and connect

5. **Access:**
   - Web UI: http://wordclock.local
   - MQTT: Configure in web UI settings

For detailed setup instructions, see [DEPLOYMENT.md](docs/DEPLOYMENT.md).

## Documentation

- 📚 [Architecture Overview](docs/ARCHITECTURE.md) - System design and components
- 🛠️ [Development Guide](docs/DEVELOPMENT.md) - Building and contributing
- 🚀 [Deployment Guide](docs/DEPLOYMENT.md) - Installation and configuration
- 🏠 [Home Assistant Integration](docs/MQTT_INTEGRATION.md) - MQTT setup
- 💡 [LED Mapping](docs/LED_MAPPING.md) - Grid variant system
- 🔍 [Troubleshooting](docs/TROUBLESHOOTING.md) - Common issues
- 📖 [API Reference](docs/api/html/index.html) - Code documentation

## Project Structure

```
wordclock/
├── src/                # Production source code
├── include/            # Header files
├── test/               # Unit and integration tests
├── data/               # Web UI files (HTML/CSS/JS)
├── tools/              # Build scripts
└── docs/               # Documentation
```

## Configuration

Key settings in `src/config.h`:
- `FIRMWARE_VERSION` - Current version
- `DATA_PIN` - LED strip data pin
- `DEFAULT_BRIGHTNESS` - Initial brightness (0-255)
- `CLOCK_NAME` - Device name
- `TZ_INFO` - Timezone string

See [docs/DEPLOYMENT.md](docs/DEPLOYMENT.md) for full configuration reference.

## Development

### Building from Source
```bash
# Install dependencies
pio lib install

# Build
pio run

# Run tests
pio test -e native
```

### Contributing
We welcome contributions! Please see [CONTRIBUTING.md](docs/CONTRIBUTING.md) for:
- Code style guidelines
- Pull request process
- Development workflow

### Testing
```bash
# Unit tests (native)
pio test -e native

# Integration tests (on-device)
pio test -e esp32_test

# All tests
pio test
```

## Home Assistant Integration

The word clock provides full MQTT discovery for Home Assistant:

**Entities:**
- Light control (brightness, color)
- Animation toggle
- Night mode settings
- System sensors (version, IP, RSSI)
- Diagnostic buttons

See [MQTT_INTEGRATION.md](docs/MQTT_INTEGRATION.md) for setup guide.

## Troubleshooting

### Common Issues

**Clock not connecting to WiFi:**
- Hold reset for 10 seconds to clear credentials
- Connect to `Wordclock_AP` network
- Reconfigure WiFi

**LEDs not lighting:**
- Check `DATA_PIN` in config.h
- Verify power supply voltage (5V)
- Test with single LED first

**MQTT not connecting:**
- Verify broker settings in web UI
- Check network connectivity
- Review logs at http://wordclock.local/logs

For more issues, see [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md).

## Versioning

We use [Semantic Versioning](https://semver.org/):
- **Major:** Breaking changes
- **Minor:** New features (backwards compatible)
- **Patch:** Bug fixes

See [CHANGELOG.md](docs/CHANGELOG.md) for release history.

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file.

## Acknowledgments

- ESP32 Arduino Core
- WiFiManager by tzapu
- ArduinoJson by Benoit Blanchon
- Adafruit NeoPixel Library
- Home Assistant Community

## Support

- 🐛 [Report a bug](https://github.com/user/wordclock/issues/new?template=bug_report.md)
- 💡 [Request a feature](https://github.com/user/wordclock/issues/new?template=feature_request.md)
- ❓ [Ask a question](https://github.com/user/wordclock/discussions)
- 💬 [Discord Community](#) (if applicable)

## Author

**Your Name** - [GitHub](https://github.com/user)

---

**Made with ❤️ for the DIY community**
```

---

## Phase 2: Architecture Documentation

**File:** `docs/ARCHITECTURE.md` (new)

```markdown
# Word Clock Firmware Architecture

## Overview

The Word Clock firmware is a modular ESP32 application built on the Arduino framework, implementing a networked word clock with extensive IoT integration.

## System Architecture

### High-Level Components

```
┌─────────────────────────────────────────────────────────┐
│                     User Interfaces                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────────┐ │
│  │ Web UI   │  │ MQTT     │  │ Physical │  │ OTA     │ │
│  └────┬─────┘  └────┬─────┘  │ Controls │  └────┬────┘ │
│       │             │         └────┬─────┘       │      │
└───────┼─────────────┼──────────────┼─────────────┼──────┘
        │             │              │             │
┌───────┴─────────────┴──────────────┴─────────────┴──────┐
│                   Application Layer                      │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Clock Display Controller                         │   │
│  │  - Time synchronization                          │   │
│  │  - LED animation                                 │   │
│  │  - Night mode                                    │   │
│  └──────────────────────────────────────────────────┘   │
│                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ Settings     │  │ Grid         │  │ LED          │  │
│  │ Manager      │  │ Variants     │  │ Controller   │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└──────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────┴────────────────────────────────┐
│                   Infrastructure Layer                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────────┐ │
│  │ Network  │  │ MQTT     │  │ FileSystem│  │ Logging │ │
│  │ WiFi Mgr │  │ Client   │  │ SPIFFS   │  │ System  │ │
│  └──────────┘  └──────────┘  └──────────┘  └─────────┘ │
└──────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────┴────────────────────────────────┐
│                     Hardware Layer                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────────┐ │
│  │ ESP32    │  │ WS2812B  │  │ NVS      │  │ RTC     │ │
│  │ MCU      │  │ LED Strip│  │ Flash    │  │ Time    │ │
│  └──────────┘  └──────────┘  └──────────┘  └─────────┘ │
└──────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Clock Display System

**Purpose:** Manage time display and LED animations

**Key Modules:**
- `wordclock.cpp` - Main display logic
- `time_mapper.cpp` - Time to word conversion
- `led_controller.cpp` - LED hardware interface
- `grid_layout.cpp` - Grid variant management

**State Machine:**
```
┌──────────────┐
│ Initializing │
└──────┬───────┘
       │
       ▼
┌──────────────┐     ┌────────────────┐
│ No Time      │────▶│ Time Synced    │
│ (Indicator)  │     │                │
└──────────────┘     └────────┬───────┘
                              │
                              ▼
                     ┌────────────────┐
                     │ Static Display │◀───┐
                     └────────┬───────┘    │
                              │            │
         ┌────────────────────┴──────┐     │
         │ Minute Change Detected    │     │
         └────────────┬──────────────┘     │
                      │                    │
                      ▼                    │
              ┌───────────────┐            │
              │  Animating    │────────────┘
              │  (Frame Loop) │
              └───────────────┘
```

### 2. Network System

**Purpose:** WiFi connectivity and configuration

**Key Modules:**
- `network.cpp` - WiFi management
- `WiFiManager` - Configuration portal

**Connection Flow:**
```
Boot
 │
 ├─ Credentials Saved?
 │   ├─ Yes: Connect to WiFi
 │   │   ├─ Success → Connected State
 │   │   └─ Fail → Config Portal (15s retry)
 │   └─ No: Start Config Portal
 │
 └─ Config Portal Active
     ├─ User configures WiFi
     ├─ Save credentials
     └─ Restart → Connect
```

### 3. MQTT Integration

**Purpose:** Home Assistant and automation integration

**Key Modules:**
- `mqtt_client.cpp` - MQTT protocol
- `mqtt_settings.cpp` - Configuration
- Home Assistant Discovery

**Message Flow:**
```
Device                     MQTT Broker            Home Assistant
  │                            │                        │
  ├─ Connect ─────────────────▶│                        │
  │                            │                        │
  ├─ Publish Discovery ───────▶│───── Subscribe ───────▶│
  │  (homeassistant/*/config)  │                        │
  │                            │                        │
  ├─ Publish State ───────────▶│───── Process ─────────▶│
  │  (wordclock/*/state)       │                        │
  │                            │                        │
  │◀─── Command ───────────────│◀───── Control ─────────│
  │  (wordclock/*/set)         │                        │
  │                            │                        │
  ├─ Update State ────────────▶│───── Update UI ───────▶│
  │                            │                        │
```

### 4. Settings Persistence

**Purpose:** Save configuration across reboots

**Key Modules:**
- `display_settings.h` - Display preferences
- `night_mode.cpp` - Night mode config
- `led_state.h` - LED color/brightness
- ESP32 Preferences (NVS)

**Storage Architecture:**
```
ESP32 NVS Flash
├── Namespace: "wc_led"
│   ├── r, g, b, w (color values)
│   └── br (brightness)
├── Namespace: "wc_display"
│   ├── his_sec (HET IS duration)
│   ├── anim_on (animation enabled)
│   ├── grid_id (grid variant)
│   └── upd_ch (update channel)
├── Namespace: "wc_night"
│   ├── enabled (night mode on/off)
│   ├── effect (DIM/OFF)
│   ├── start, end (schedule)
│   └── dim_pct (dim percentage)
└── Namespace: "wc_log"
    ├── level (log level)
    └── retention (days)
```

### 5. OTA Update System

**Purpose:** Wireless firmware and UI updates

**Key Modules:**
- `ota_updater.cpp` - Update logic
- `ArduinoOTA` - Network OTA
- GitHub integration

**Update Flow:**
```
1. Check Version
   │
   ├─ Fetch manifest.json from GitHub
   ├─ Compare versions (current vs. remote)
   │
   └─ New version available?
       ├─ No → Done
       └─ Yes ↓

2. Download Firmware
   │
   ├─ HTTPS GET firmware.bin
   ├─ Stream to Update partition
   │
   └─ Verify ↓

3. Install
   │
   ├─ Verify integrity
   ├─ Set boot partition
   └─ Restart ↓

4. First Boot
   │
   ├─ Verify new firmware
   ├─ Download UI files if needed
   └─ Normal operation
```

## Data Flow

### Time Display Update

```
NTP Server
    │
    ▼
[ESP32 RTC]
    │
    ▼
getLocalTime()
    │
    ▼
time_mapper.cpp
    │
    ├─ Round to 5min
    ├─ Map to words
    ├─ Get LED indices
    │
    ▼
Animation System
    │
    ├─ Build frames
    ├─ Execute steps
    │
    ▼
LED Controller
    │
    ├─ Apply color
    ├─ Apply brightness
    ├─ Apply night mode
    │
    ▼
[WS2812B Strip]
```

### Settings Change via MQTT

```
Home Assistant
    │
    ▼
MQTT Command
(wordclock/light/set)
    │
    ▼
handleMessage()
    │
    ├─ Parse JSON
    ├─ Validate
    │
    ▼
Settings Update
    │
    ├─ ledState.setRGB()
    ├─ Mark dirty
    │
    ▼
Display Update
    │
    ├─ showLeds()
    │
    ▼
MQTT State Publish
(wordclock/light/state)
    │
    ▼
Home Assistant UI Update
```

## Threading Model

ESP32 uses FreeRTOS, but this firmware primarily uses:

**Main Loop (setup/loop):**
- Priority: 1 (default)
- Core: 1 (Arduino core)
- Stack: 8KB

**WiFi Task:**
- Priority: 23
- Core: 0
- Stack: 4KB (ESP-IDF managed)

**Key Considerations:**
- No explicit threading in application code
- All logic in main loop (cooperative multitasking)
- Avoid blocking operations (> 100ms)
- Use async patterns for I/O

## Memory Management

### RAM Usage (typical)

| Component | Usage | Notes |
|-----------|-------|-------|
| ESP32 System | ~40KB | WiFi, BT stack |
| Arduino Framework | ~20KB | Core libraries |
| Application Code | ~30KB | Our firmware |
| LED Buffers | ~5KB | Frame buffers |
| MQTT Buffer | ~1KB | Message queue |
| **Total** | **~96KB** | of 320KB available |

### Flash Usage

| Partition | Size | Usage |
|-----------|------|-------|
| Bootloader | 32KB | ESP32 bootloader |
| Partition Table | 4KB | Partition definitions |
| App0 (firmware) | 1.5MB | Running firmware |
| App1 (OTA) | 1.5MB | Update partition |
| SPIFFS (data) | 4MB | Web UI + logs |
| **Total** | **~7MB** | of 8MB flash |

## Error Handling

### Strategy
1. **Fail Gracefully:** Continue operation when possible
2. **Log Everything:** Persistent logging for debugging
3. **User Feedback:** Visual indicators (LEDs)
4. **Recovery:** Automatic reconnection and retry

### Error Categories

| Category | Example | Handling |
|----------|---------|----------|
| Critical | Flash corruption | Log + restart |
| Network | WiFi disconnected | Auto-reconnect with backoff |
| Time Sync | NTP unreachable | Show indicator, keep trying |
| MQTT | Broker offline | Exponential backoff |
| Display | Invalid time | Show last known good |

## Performance Considerations

### Target Metrics
- **Loop time:** < 10ms (100Hz)
- **Animation frame rate:** 2 FPS (500ms/frame)
- **MQTT latency:** < 100ms
- **Boot time:** < 5 seconds (with WiFi)

### Optimization Techniques
1. **Deferred Persistence:** Batch flash writes
2. **Time Caching:** Fetch NTP once per second
3. **MQTT Batching:** State updates every 30s
4. **LED Optimization:** Update only on change

## Security

### Current Implementation
- ✅ HTTPS for OTA (certificate validation bypassed)
- ✅ MQTT authentication (username/password)
- ✅ Web UI authentication (optional)
- ❌ WiFi credentials stored in plaintext
- ❌ No firmware signing

### Recommendations
1. Enable web UI authentication by default
2. Encrypt WiFi credentials in NVS
3. Implement firmware signing
4. Add rate limiting to web endpoints

## Extensibility

### Adding New Grid Variants
1. Create `grid_variants/xx_vN.h` and `.cpp`
2. Define word positions and LED mappings
3. Register in `grid_layout.cpp`
4. Add to `GridVariant` enum

### Adding New Languages
1. Define word mappings in new grid variant
2. Add time-to-word logic in `time_mapper.cpp`
3. Test all times (00:00 - 23:59)
4. Document in `LED_MAPPING.md`

### Adding MQTT Entities
1. Define topic strings in `mqtt_client.cpp`
2. Add discovery configuration in `publishDiscovery()`
3. Implement command handler
4. Add state publishing

## References

- [ESP32 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)
- [Home Assistant MQTT Discovery](https://www.home-assistant.io/docs/mqtt/discovery/)
- [WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)

---

**Document Version:** 1.0  
**Last Updated:** January 4, 2026  
**Maintainer:** [Your Name]
```

---

## Phase 3: API Reference (Doxygen)

### Doxygen Configuration

**File:** `Doxyfile` (new)

```doxyfile
# Project configuration
PROJECT_NAME           = "Word Clock Firmware"
PROJECT_NUMBER         = 26.1.5-rc.1
PROJECT_BRIEF          = "ESP32 Word Clock with IoT Integration"
PROJECT_LOGO           = docs/images/logo.png

# Input configuration
INPUT                  = src/ include/ README.md
RECURSIVE              = YES
EXCLUDE                = src/grid_variants/
FILE_PATTERNS          = *.cpp *.h *.md
EXTENSION_MAPPING      = ino=C++

# Output configuration
OUTPUT_DIRECTORY       = docs/api
GENERATE_HTML          = YES
GENERATE_LATEX         = NO
HTML_OUTPUT            = html
HTML_FILE_EXTENSION    = .html
HTML_COLORSTYLE_HUE    = 220
HTML_COLORSTYLE_SAT    = 100
HTML_COLORSTYLE_GAMMA  = 80

# Documentation extraction
EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = NO
EXTRACT_STATIC         = YES
EXTRACT_LOCAL_CLASSES  = YES

# Diagrams
HAVE_DOT               = YES
CALL_GRAPH             = YES
CALLER_GRAPH           = YES
DOT_IMAGE_FORMAT       = svg
INTERACTIVE_SVG        = YES

# Other options
JAVADOC_AUTOBRIEF      = YES
QT_AUTOBRIEF           = YES
OPTIMIZE_OUTPUT_FOR_C  = NO
BUILTIN_STL_SUPPORT    = YES
```

### Add Doxygen Comments to Code

**Example for `time_mapper.cpp`:**

```cpp
/**
 * @file time_mapper.cpp
 * @brief Time-to-LED mapping logic for word clock
 * 
 * This module provides functions to convert time values into LED indices
 * that represent the time in words on the word clock grid.
 * 
 * @author Your Name
 * @date 2026-01-04
 */

/**
 * @brief Get LED indices for specific time
 * 
 * Converts a time structure into a vector of LED indices that should
 * be illuminated to display that time in words.
 * 
 * @param timeinfo Pointer to tm structure containing time to display
 * @return std::vector<uint16_t> LED indices to illuminate
 * 
 * @note Minutes are rounded down to nearest 5-minute interval
 * @note Extra minutes (1-4) are shown as corner LEDs
 * 
 * @example
 * ```cpp
 * struct tm time = {.tm_hour = 12, .tm_min = 34};
 * auto leds = get_led_indices_for_time(&time);
 * showLeds(leds);  // Display "half een" + 4 corner LEDs
 * ```
 */
std::vector<uint16_t> get_led_indices_for_time(struct tm* timeinfo);
```

### Generate Documentation

Add to CI workflow:

```yaml
- name: Generate API Documentation
  run: |
    sudo apt-get install -y doxygen graphviz
    doxygen Doxyfile
    
- name: Deploy Documentation
  uses: peaceiris/actions-gh-pages@v3
  with:
    github_token: ${{ secrets.GITHUB_TOKEN }}
    publish_dir: ./docs/api/html
```

---

## Phase 4: Developer Guide

**File:** `docs/DEVELOPMENT.md` (new)

```markdown
# Development Guide

## Prerequisites

### Required Tools
- [PlatformIO](https://platformio.org/) 5.0+ (recommended)
  OR
- [Arduino IDE](https://www.arduino.cc/) 1.8.19+
- [Git](https://git-scm.com/) for version control
- USB cable for ESP32 programming

### Optional Tools
- [VSCode](https://code.visualstudio.com/) with PlatformIO extension
- [MQTT Explorer](http://mqtt-explorer.com/) for MQTT debugging
- Logic analyzer for LED debugging
- Serial monitor (built into PlatformIO/Arduino IDE)

## Development Environment Setup

### 1. Clone Repository
```bash
git clone https://github.com/user/wordclock.git
cd wordclock
```

### 2. Install Dependencies
```bash
# PlatformIO will auto-install on first build
pio lib install

# Or manually:
pio lib install "adafruit/Adafruit NeoPixel@^1.12.1"
pio lib install "bblanchon/ArduinoJson@^7.4.1"
# ... etc
```

### 3. Create Configuration Files
```bash
# Copy example secrets file
cp src/secrets.h.example src/secrets.h

# Edit with your values
nano src/secrets.h
```

Example `secrets.h`:
```cpp
#pragma once

// Optional: Pre-configure MQTT (can also be done via web UI)
#define MQTT_HOST "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USER "wordclock"
#define MQTT_PASS "password"

// Optional: Pre-configure WiFi for AP mode password
#define AP_PASSWORD "wordclock123"
```

### 4. Build Project
```bash
# Build for ESP32
pio run -e esp32dev

# Build specific environment
pio run -e esp32dev --target clean
pio run -e esp32dev

# Verbose output
pio run -e esp32dev -v
```

## Project Structure

```
wordclock/
├── src/                        # Source files
│   ├── main.cpp               # Entry point
│   ├── wordclock.cpp          # Clock display logic
│   ├── time_mapper.cpp        # Time-to-LED mapping
│   ├── led_controller.cpp     # LED hardware control
│   ├── mqtt_client.cpp        # MQTT integration
│   ├── network.cpp            # WiFi management
│   ├── night_mode.cpp         # Night mode logic
│   ├── grid_variants/         # Language/size variants
│   └── ...
├── include/                    # Headers (if not in src/)
├── test/                       # Unit tests
│   ├── test_time_mapper/
│   ├── test_led_controller/
│   └── ...
├── data/                       # Web UI files (SPIFFS)
│   ├── dashboard.html
│   ├── admin.html
│   └── ...
├── tools/                      # Build scripts
│   ├── full_upload.py
│   └── generate_build_info.py
└── platformio.ini             # PlatformIO config
```

## Building and Uploading

### Full Build and Upload
```bash
# Build, upload firmware, and upload filesystem
pio run -e esp32dev -t upload -t uploadfs
```

### Firmware Only
```bash
pio run -e esp32dev -t upload
```

### Filesystem Only
```bash
pio run -e esp32dev -t uploadfs
```

### Monitor Serial Output
```bash
pio device monitor -e esp32dev
# or
pio run -e esp32dev -t monitor
```

## Testing

### Unit Tests (Native)
```bash
# Run all tests on PC
pio test -e native

# Run specific test
pio test -e native -f test_time_mapper

# Verbose output
pio test -e native -v
```

### Integration Tests (On-Device)
```bash
# Run tests on actual ESP32
pio test -e esp32_test
```

### Manual Testing Checklist
- [ ] Boot and WiFi connection
- [ ] Web UI accessible
- [ ] Time syncs via NTP
- [ ] Clock displays correctly
- [ ] Animations work (if enabled)
- [ ] Night mode activates
- [ ] MQTT publishes state
- [ ] MQTT commands work
- [ ] OTA update completes
- [ ] Settings persist after reboot

## Debugging

### Serial Debug Output
```bash
# Set log level in config.h
#define DEFAULT_LOG_LEVEL LOG_LEVEL_DEBUG

# Monitor output
pio device monitor
```

### MQTT Debugging
```bash
# Subscribe to all topics
mosquitto_sub -h localhost -t 'wordclock/#' -v

# Publish test command
mosquitto_pub -h localhost -t 'wordclock/light/set' -m '{"state":"ON"}'
```

### LED Debugging
- Use single LED test pattern
- Check DATA_PIN definition
- Verify 5V power supply
- Measure voltage at strip
- Check ground connections

### Common Issues

**Compilation fails:**
```bash
# Clean and rebuild
pio run -e esp32dev --target clean
pio run -e esp32dev
```

**Upload fails:**
```bash
# Check USB connection
ls /dev/tty.* # macOS
ls /dev/ttyUSB* # Linux

# Hold BOOT button during upload

# Check permissions (Linux)
sudo usermod -a -G dialout $USER
# Log out and back in
```

**LEDs don't work:**
- Verify DATA_PIN matches hardware
- Check power supply (5V, 3-5A)
- Test with single LED
- Check LED strip type (WS2812B vs SK6812)

## Code Style Guidelines

### C++ Style
```cpp
// Class names: PascalCase
class ClockDisplay {
public:
    // Public methods: camelCase
    void updateDisplay();
    
private:
    // Private members: camelCase with trailing underscore
    bool isActive_;
    int currentStep_;
};

// Functions: camelCase or snake_case (be consistent)
void initializeHardware();
bool get_led_indices();

// Constants: UPPER_SNAKE_CASE
const int MAX_BRIGHTNESS = 255;
#define DEFAULT_TIMEOUT_MS 5000

// Enums: PascalCase for type, UPPER_CASE for values
enum class DisplayMode {
    STATIC,
    ANIMATED
};
```

### File Organization
```cpp
// 1. License header (if applicable)
// 2. File comment
/**
 * @file example.cpp
 * @brief Brief description
 */

// 3. Includes (grouped)
#include <Arduino.h>      // System includes
#include <vector>

#include "config.h"       // Project includes
#include "example.h"

// 4. Namespace (if used)
namespace wordclock {

// 5. Constants and statics
static const int BUFFER_SIZE = 256;

// 6. Helper functions (file-scope)
static void helperFunction() {
    // ...
}

// 7. Public functions
void publicFunction() {
    // ...
}

} // namespace
```

### Documentation
```cpp
/**
 * @brief Short one-line description
 * 
 * Longer description with details about what the function does,
 * any important notes, and usage examples.
 * 
 * @param param1 Description of param1
 * @param param2 Description of param2
 * @return Description of return value
 * 
 * @note Important notes
 * @warning Warnings about usage
 * 
 * @example
 * ```cpp
 * int result = exampleFunction(10, 20);
 * ```
 */
int exampleFunction(int param1, int param2);
```

## Git Workflow

### Branch Strategy
```
main                  # Stable releases
  └─ develop          # Integration branch
      ├─ feature/*    # New features
      ├─ fix/*        # Bug fixes
      └─ refactor/*   # Code improvements
```

### Commit Messages
```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Formatting
- `refactor`: Code restructuring
- `test`: Adding tests
- `chore`: Maintenance

**Examples:**
```
feat(mqtt): add support for custom discovery prefix

Add configuration option for custom Home Assistant
discovery prefix instead of hardcoded "homeassistant".

Closes #42
```

```
fix(time): resolve race condition in time cache

Time cache could become stale if NTP sync failed silently.
Added staleness check and proper invalidation.

Fixes #89
```

### Pull Request Process
1. Create feature branch from `develop`
2. Make changes and commit
3. Push to GitHub
4. Open PR to `develop`
5. Ensure CI passes
6. Request review
7. Address feedback
8. Merge when approved

## Release Process

### Version Numbering
```
MAJOR.MINOR.PATCH-PRERELEASE

Examples:
- 26.1.5 (stable release)
- 26.2.0-rc.1 (release candidate)
- 26.2.0-beta.1 (beta)
- 26.2.0-develop (development)
```

### Creating a Release
1. Update `FIRMWARE_VERSION` in `config.h`
2. Update `CHANGELOG.md`
3. Commit: `chore: bump version to X.Y.Z`
4. Tag: `git tag -a vX.Y.Z -m "Release X.Y.Z"`
5. Push: `git push origin vX.Y.Z`
6. GitHub Actions builds and publishes

## Resources

- [ESP32 Arduino Core Docs](https://docs.espressif.com/projects/arduino-esp32/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [Home Assistant MQTT](https://www.home-assistant.io/docs/mqtt/)
- [WS2812B Guide](https://learn.adafruit.com/adafruit-neopixel-uberguide)

## Getting Help

- Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
- Search [existing issues](https://github.com/user/wordclock/issues)
- Ask in [Discussions](https://github.com/user/wordclock/discussions)
- Join Discord (if applicable)

---

**Happy Coding! 🚀**
```

---

## Implementation Timeline

### Week 1: Core Documentation
- **Day 1:** Enhanced README
- **Day 2:** Architecture documentation
- **Day 3:** Doxygen setup and configuration
- **Day 4:** Add Doxygen comments to major files
- **Day 5:** Generate and review API docs

### Week 2: Guides and References
- **Day 1:** Development guide
- **Day 2:** Deployment guide
- **Day 3:** MQTT integration guide
- **Day 4:** Troubleshooting guide
- **Day 5:** Contributing guidelines

### Week 3: Polish and Review
- **Day 1:** LED mapping documentation
- **Day 2:** Create diagrams and images
- **Day 3:** Review all documentation
- **Day 4:** Team review and feedback
- **Day 5:** Final updates and publish

---

## Success Criteria

- ✅ All major components documented
- ✅ API reference complete (Doxygen)
- ✅ Setup guides tested by new user
- ✅ Troubleshooting covers common issues
- ✅ Contributing guidelines clear
- ✅ Documentation accessible and well-organized
- ✅ Positive feedback from team/community

---

**Plan Version:** 1.0  
**Last Updated:** January 4, 2026  
**Status:** Ready for Implementation
