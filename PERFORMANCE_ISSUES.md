# Performance Issues - Traag Worden / Bricken

## Mogelijke Oorzaken

### 1. **Log Buffer Heap Fragmentatie** ⚠️ KRITIEK
**Probleem:**
- `String logBuffer[LOG_BUFFER_SIZE]` met 150 entries
- Elke String allocatie veroorzaakt heap fragmentatie
- Bij veel logging (vooral DEBUG level) wordt heap snel gefragmenteerd
- ESP32 heeft beperkte heap (typisch 200-300KB)

**Symptomen:**
- Device wordt langzamer na verloop van tijd
- Heap free neemt af
- Device crasht of herstart spontaan
- Memory allocation failures

**Oplossing:**
- Gebruik fixed-size char arrays in plaats van String objecten
- Of verklein LOG_BUFFER_SIZE
- Of gebruik een circular buffer met char pointers

### 2. **String Concatenatie Zonder Reserve** ⚠️ BELANGRIJK
**Probleem:**
- Veel `String + String` operaties zonder `.reserve()`
- Elke concatenatie kan nieuwe heap allocatie veroorzaken
- Heap fragmentatie neemt toe

**Locaties:**
- `log.cpp`: `makeLogPrefix() + msg`
- `web_routes.h`: Veel String concatenaties
- `mqtt_client.cpp`: String concatenaties

**Oplossing:**
- Gebruik `.reserve()` voor String concatenaties
- Of gebruik `snprintf()` met char buffers
- Minimaliseer String objecten

### 3. **Log Bestanden Te Groot** ⚠️ BELANGRIJK
**Probleem:**
- Log bestanden kunnen onbeperkt groeien
- Filesystem raakt vol
- File operations worden trager
- Device kan crashen bij vol filesystem

**Oplossing:**
- Implementeer log rotation (max bestandsgrootte)
- Verwijder oude log bestanden automatisch
- Limiteer totale log directory grootte

### 4. **Te Veel Debug Logging** ⚠️ BELANGRIJK
**Probleem:**
- Bij DEBUG level worden ALLE log messages opgeslagen
- Log buffer vult snel
- Veel String allocaties
- Performance impact

**Oplossing:**
- Zet log level op INFO of WARN in productie
- Debug logging alleen tijdens ontwikkeling
- Gebruik conditional compilation voor debug logs

### 5. **File System Operaties** ⚠️ MATIG
**Probleem:**
- `ensureLogFile()` wordt vaak aangeroepen
- File operations zijn traag op SPIFFS/LittleFS
- Directory scanning bij elke log file switch

**Oplossing:**
- Cache log file handle
- Minder vaak file operations
- Batch file operations

### 6. **MQTT String Concatenatie** ⚠️ MATIG
**Probleem:**
- String concatenatie in `mqtt_client.cpp`
- Elke MQTT publish kan String allocaties veroorzaken

**Oplossing:**
- Gebruik char buffers met snprintf
- Minimaliseer String objecten

### 7. **Web Server Memory** ⚠️ MATIG
**Probleem:**
- Web server kan veel memory gebruiken
- JSON serialization kan grote buffers alloceren
- Client connections niet altijd goed afgesloten

**Oplossing:**
- Limiteer JSON response grootte
- Zorg voor proper connection cleanup
- Limiteer aantal gelijktijdige connections

## Diagnose Stappen

### 1. Check Heap Status
```bash
# Via API
curl http://wordclock.local/api/device/info | grep heap

# Of via dashboard
# Kijk naar "heap_free" en "heap_min_free"
```

### 2. Check Log Level
```bash
curl http://wordclock.local/getLogLevel
```

### 3. Check Log Bestanden
```bash
curl http://wordclock.local/api/logs/summary
```

### 4. Monitor Heap Over Tijd
- Check heap_free regelmatig
- Als heap_free blijft dalen = memory leak
- Als heap_min_free laag is = fragmentatie

## Snelle Oplossingen

### 1. Zet Log Level op INFO
```bash
curl -X POST "http://wordclock.local/setLogLevel?level=INFO"
```

### 2. Verwijder Oude Logs
```bash
# Via restart (wist alle logs)
curl http://wordclock.local/restart
```

### 3. Herstart Device
```bash
curl http://wordclock.local/restart
```

### 4. Check Filesystem Vrij
- Via dashboard: kijk naar log summary
- Als > 500KB, overweeg logs te wissen

## Langetermijn Oplossingen

### 1. Fix Log Buffer
- Vervang String array met char array
- Of gebruik een betere buffer implementatie

### 2. Fix String Concatenatie
- Gebruik `.reserve()` voor alle String concatenaties
- Of vervang met char buffers + snprintf

### 3. Implementeer Log Rotation
- Max bestandsgrootte per log file
- Automatische cleanup van oude logs
- Limiteer totale log directory grootte

### 4. Optimaliseer Logging
- Minder debug logging in productie
- Gebruik conditional compilation
- Log alleen belangrijke events

## Monitoring

### Heap Monitoring
Voeg toe aan main loop:
```cpp
static unsigned long lastHeapLog = 0;
if (millis() - lastHeapLog > 60000) { // Elke minuut
  logInfo("Heap: " + String(ESP.getFreeHeap()) + " free, " + 
          String(ESP.getMinFreeHeap()) + " min");
  lastHeapLog = millis();
}
```

### Log File Size Monitoring
Check regelmatig log file grootte en roteer indien nodig.

## Prioriteit

1. **HOOG**: Log buffer String array → char array
2. **HOOG**: Log level op INFO zetten
3. **MEDIUM**: String concatenatie optimaliseren
4. **MEDIUM**: Log rotation implementeren
5. **LAAG**: File operations optimaliseren


