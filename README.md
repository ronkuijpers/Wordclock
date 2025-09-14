
# Wordclock Firmware

Firmware voor een ESP32-gebaseerde Wordclock met OTA-updates, webinterface en pixel-LED tijdsweergave.

## Status

> Ontwikkeling in progress.  
> Functionaliteit is incompleet en instabiel.  
> Gebruik op eigen risico.

## Features

- WiFi setup via WiFiManager
- OTA firmware updates
- Web-based dashboard en admin interface
- Telnet logging
- NeoPixel LED woordklok

## Installatie & Hardware

Zie [docs/BuildInstruction.md](docs/BuildInstruction.md) voor hardware-aansluitingen en led-configuratie.

Benodigdheden:
- ESP32 microcontroller
- NeoPixel LED strip/matrix
- Voeding (VIN + GND)

## Configuratie

1. Kopieer `include/secrets_template.h` naar `include/secrets.h` en vul je WiFi-gegevens in.
2. Pas indien nodig `upload_params_template.ini` aan en hernoem naar `upload_params.ini`.
3. Compileer en upload de firmware via PlatformIO.

## Gebruik

Na installatie is de klok bereikbaar via het lokale netwerk. Gebruik het webdashboard voor instellingen, updates en status.

## Documentatie

- [BuildInstruction.md](docs/BuildInstruction.md): Hardware en led-opbouw
- [QuickStart.md](docs/QuickStart.md): Snelstartgids
- [todo](docs/todo): Openstaande en afgeronde taken

## Todo's & Roadmap

Zie [docs/todo](docs/todo) voor actuele en afgeronde taken.

## License

To be determined.
