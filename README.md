
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


## Projectstructuur & Modules

De firmware is modulair opgebouwd. Belangrijkste modules:

- `main.cpp`: Centrale setup en loop, roept alle modules aan.
- `network_init.h`: WiFi-initialisatie via WiFiManager.
- `ota_init.h`: Over-the-air updates (OTA).
- `time_sync.h`: Tijdsynchronisatie via NTP.
- `webserver_init.h`: Webserver en route-initialisatie.
- `mqtt_init.h`: MQTT-initialisatie en event loop.
- `display_init.h`: LED- en display-instellingen.
- `startup_sequence_init.h`: Opstartanimatie.
- `wordclock_main.h`: Hoofdlogica van de klok.
- `wordclock_system_init.h`: UI-authenticatie en wordclock setup.

Zie de comments in elk modulebestand voor uitleg over functionaliteit en gebruik.

## Documentatie

- [BuildInstruction.md](docs/BuildInstruction.md): Hardware en led-opbouw
- [QuickStart.md](docs/QuickStart.md): Snelstartgids
- [todo](docs/todo): Openstaande en afgeronde taken

## Todo's & Roadmap

Zie [docs/todo](docs/todo) voor actuele en afgeronde taken.

## License

To be determined.
