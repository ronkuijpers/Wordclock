# Wordclock Firmware

Firmware for an ESP32-based word clock with OTA updates, a browser dashboard, and addressable LED output.

## TL;DR

1. Download the latest release binary from the [GitHub releases page](https://github.com/ronkuijpers/Wordclock/releases).
2. Flash the ESP32, for example:  
   `esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash -z 0x1000 wordclock-v0.12.bin`
3. Power-cycle the clock and connect to the temporary Wi-Fi network `Wordclock_AP` to finish setup.

## Project Status

- Actively developed; expect breaking changes between releases.
- Firmware auto-updates nightly when connected to the internet.
- Use on production hardware only after validating with your setup.

## Highlights

- Guided Wi-Fi onboarding via WiFiManager captive portal.
- Automatic and manual OTA firmware updates, configurable update channel via `firmware.json`.
- Responsive web dashboard (`/dashboard`) and admin panel (`/admin`) served from SPIFFS.
- MQTT integration for home automation platforms.
- Multiple letter-grid layouts with persistent storage in NVS.
- Telnet logging and serial diagnostics for development.

## Hardware & Wiring

Refer to [docs/BuildInstruction.md](docs/BuildInstruction.md) for wiring notes. At minimum you need:
- ESP32 DevKit or equivalent (4 MB flash recommended)
- 5 V power supply tied to `VIN` and `GND`
- NeoPixel/WS2812 LED matrix connected to GPIO `D4`

Mount the LEDs starting at the upper-right corner, skip the first LED (index 0), and leave four LED gaps when turning a corner.

## Firmware Options

- **Pre-built binary**: Each tagged release publishes `wordclock-vX.Y.bin` plus a `firmware.json` manifest that OTA clients consume.
- **Local build**: Use PlatformIO (`platformio run -t upload`) after cloning the repository and configuring secrets (see below).
- **OTA bundles**: Static dashboard assets live under `data/`; they are uploaded with `platformio run -t uploadfs` or via the web-based upgrader.

## Configuration

1. Copy `include/secrets_template.h` to `include/secrets.h` and set Wi-Fi credentials and optional MQTT details.
2. Duplicate `upload_params_template.ini` as `upload_params.ini` for per-device OTA and serial upload parameters.
3. Adjust grid layout defaults in `src/wordclock_system_init.cpp` if you ship with a non-default language layout.

## Admin & Daily Use

- Access the dashboard at `http://wordclock.local` or by IP to adjust brightness, animations, and scheduling.
- The admin UI exposes Wi-Fi reset, password management, manual firmware upload, and live logs.
- Time synchronizes automatically via NTP; no RTC battery is required.
- Nightly OTA checks occur at 02:00 local device time. Override the manifest URL in `src/config.h` if you host your own updates.

## Development Notes

| Path                     | Purpose                                                                    |
|--------------------------|----------------------------------------------------------------------------|
| `src/`                   | Core firmware modules (`main.cpp`, subsystems, grid layouts)               |
| `data/`                  | Web dashboard and admin static assets (served from SPIFFS)                 |
| `include/`               | Public headers, secrets template, feature flags                            |
| `lib/`                   | External and custom reusable libraries                                     |
| `tools/`                 | Utility scripts such as OTA deployment helpers                             |
| `docs/`                  | Build instructions, quick start sheet, task list                           |

Key modules to inspect first:
- `src/main.cpp` bootstraps networking, OTA, web, and display subsystems.
- `src/grid_layout.cpp` contains the available word-clock layouts and helper lookups.
- `src/wordclock_system_init.cpp` wires settings, authentication, and web routes together.

## Documentation

- [QuickStart.md](docs/QuickStart.md): End-user setup walkthrough.
- [BuildInstruction.md](docs/BuildInstruction.md): Physical assembly notes.
- [todo](docs/todo): Backlog of open and completed tasks.

## Support & Contributions

Bug reports and feature requests are welcome in the issue tracker. Please include device details, firmware version, and logs from `/logs.html` where possible. Pull requests should target the `main` branch and include relevant tests or reproduction steps.

## License

To be determined. Until then, assume the code is proprietary to the project maintainers.
