# Wordclock Firmware

Firmware for an ESP32-based Wordclock with OTA updates, a web interface, and pixel-LED time display.

## Status

> Development in progress.  
> Functionality is incomplete and unstable.  
> Use at your own risk.

## Features

- WiFi setup via WiFiManager
- OTA firmware updates
- Web-based dashboard and admin interface
- Telnet logging
- NeoPixel LED word clock

## Installation & Hardware

See [docs/BuildInstruction.md](docs/BuildInstruction.md) for hardware connections and LED configuration.

Requirements:
- ESP32 microcontroller
- NeoPixel LED strip/matrix
- Power supply (VIN + GND)

## Configuration

1. Copy `include/secrets_template.h` to `include/secrets.h` and fill in your WiFi credentials.
2. Modify `upload_params_template.ini` if necessary and rename it to `upload_params.ini`.
3. Compile and upload the firmware using PlatformIO.

## Usage

After installation, the clock is accessible via the local network. Use the web dashboard for settings, updates, and status.

## Project Structure & Modules

The firmware is modularly built. Main modules include:

- `main.cpp`: Central setup and loop, invokes all modules.
- `network_init.h`: WiFi initialization via WiFiManager.
- `ota_init.h`: Over-the-air updates (OTA).
- `time_sync.h`: Time synchronization via NTP.
- `webserver_init.h`: Web server and route initialization.
- `mqtt_init.h`: MQTT initialization and event loop.
- `display_init.h`: LED and display settings.
- `startup_sequence_init.h`: Startup animation.
- `wordclock_main.h`: Main logic of the clock.
- `wordclock_system_init.h`: UI authentication and word clock setup.

Refer to the comments in each module file for explanations of functionality and usage.

## Documentation

- [BuildInstruction.md](docs/BuildInstruction.md): Hardware and LED setup
- [QuickStart.md](docs/QuickStart.md): Quick Start Guide
- [todo](docs/todo): Pending and completed tasks

## Todo's & Roadmap

See [docs/todo](docs/todo) for current and completed tasks.

## License

To be determined.