# UI-Only Update Systeem Voorstel

## Huidige Situatie

- Er is al een `/syncUI` endpoint dat UI files sync van het manifest
- Dit vereist admin auth
- Het manifest systeem ondersteunt al UI files via `files` array
- UI files worden automatisch gesynct bij firmware updates

## Probleem

- Geen eenvoudige manier om alleen UI updates te distribueren zonder firmware update
- Admin auth vereist voor `/syncUI`
- Geen manier om een specifieke UI versie te targeten

## Voorgestelde Oplossingen

### Optie 1: UI-Only Manifest (Aanbevolen) ⭐

**Concept:**
- Maak een aparte `ui-manifest.json` die alleen UI files bevat
- Voeg een nieuw endpoint toe dat deze manifest gebruikt
- Maak het toegankelijk zonder admin auth (of met UI auth)

**Voordelen:**
- Consistent met bestaand systeem
- Eenvoudig te onderhouden
- Kan versioning hebben
- Ondersteunt channels (stable/early/develop)

**Implementatie:**
```cpp
// Nieuwe functie in ota_updater.cpp
void syncUIFromManifest(const String& manifestUrl) {
  // Download manifest
  // Parse UI files
  // Download en update files
  // Update UI version
}

// Nieuw endpoint in web_routes.h
server.on("/checkUIUpdate", HTTP_ANY, []() {
  if (!ensureUiAuth()) return;
  logInfo("🔍 Checking for UI update...");
  server.send(200, "text/plain", "UI update check started");
  delay(100);
  syncUIFromManifest(UI_MANIFEST_URL); // Nieuwe constante
});
```

**Manifest Structuur:**
```json
{
  "ui_version": "2025.12.30",
  "released": "2025-12-30T12:00:00Z",
  "channels": {
    "stable": {
      "ui_version": "2025.12.30",
      "files": [
        { "path": "/dashboard.html", "url": "...", "sha256": "..." },
        ...
      ],
      "release_notes": "UI update: improved dashboard loading"
    }
  }
}
```

### Optie 2: Directe URL Update

**Concept:**
- Endpoint dat een directe URL accepteert voor een specifiek UI bestand
- Of een lijst van URLs voor meerdere bestanden

**Voordelen:**
- Flexibel
- Geen manifest nodig
- Kan gebruikt worden voor testing

**Nadelen:**
- Minder gestructureerd
- Geen versioning
- Moeilijker te beheren

**Implementatie:**
```cpp
server.on("/updateUIFile", HTTP_POST, []() {
  if (!ensureUiAuth()) return;
  if (!server.hasArg("url") || !server.hasArg("path")) {
    server.send(400, "text/plain", "Missing url or path");
    return;
  }
  String url = server.arg("url");
  String path = server.arg("path");
  // Download en update file
});
```

### Optie 3: GitHub Tag/Release Based

**Concept:**
- Download UI files van een specifieke GitHub tag of release
- Gebruik GitHub API of raw.githubusercontent.com

**Voordelen:**
- Gebruikt bestaande GitHub infrastructuur
- Eenvoudig te gebruiken
- Ondersteunt versioning via tags

**Nadelen:**
- Afhankelijk van GitHub
- Minder flexibel dan manifest

**Implementatie:**
```cpp
server.on("/updateUIFromTag", HTTP_POST, []() {
  if (!ensureUiAuth()) return;
  String tag = server.hasArg("tag") ? server.arg("tag") : "latest";
  // Download files van GitHub tag
  syncUiFilesFromVersion(tag); // Bestaande functie aanpassen
});
```

### Optie 4: ZIP File Upload

**Concept:**
- Upload een ZIP file met alle UI files
- Extract en plaats in filesystem

**Voordelen:**
- Volledige controle
- Kan lokaal worden voorbereid
- Geen internet nodig

**Nadelen:**
- Vereist ZIP library
- Meer complex
- Minder automatisch

## Aanbevolen Implementatie: Optie 1 + Optie 3 Hybrid

Combineer de beste aspecten:

1. **UI-Only Manifest** voor gestructureerde updates
2. **GitHub Tag Support** voor flexibiliteit
3. **Nieuwe endpoints** die toegankelijk zijn zonder admin auth

### Nieuwe Endpoints

```cpp
// Check voor UI update (gebruikt UI manifest)
server.on("/checkUIUpdate", HTTP_ANY, []() {
  if (!ensureUiAuth()) return;
  logInfo("🔍 Checking for UI update...");
  server.send(200, "text/plain", "UI update check started");
  delay(100);
  syncUIFromManifest(UI_MANIFEST_URL);
});

// Update UI van specifieke GitHub tag
server.on("/updateUIFromTag", HTTP_POST, []() {
  if (!ensureUiAuth()) return;
  String tag = server.hasArg("tag") ? server.arg("tag") : "";
  if (tag.isEmpty()) {
    server.send(400, "text/plain", "Missing tag parameter");
    return;
  }
  logInfo("🔍 Updating UI from tag: " + tag);
  server.send(200, "text/plain", "UI update from tag started");
  delay(100);
  syncUiFilesFromVersion(tag);
});

// Update UI van specifieke manifest URL
server.on("/updateUIFromManifest", HTTP_POST, []() {
  if (!ensureUiAuth()) return;
  String manifestUrl = server.hasArg("url") ? server.arg("url") : "";
  if (manifestUrl.isEmpty()) {
    server.send(400, "text/plain", "Missing url parameter");
    return;
  }
  logInfo("🔍 Updating UI from manifest: " + manifestUrl);
  server.send(200, "text/plain", "UI update from manifest started");
  delay(100);
  syncUIFromManifest(manifestUrl);
});
```

### Config Toevoegingen

```cpp
// In config.h of secrets.h
#define UI_MANIFEST_URL "https://raw.githubusercontent.com/ronkuijpers/Wordclock/main/ui-manifest.json"
```

### UI Manifest Voorbeeld

```json
{
  "ui_version": "2025.12.30",
  "released": "2025-12-30T12:00:00Z",
  "description": "UI-only update: improved dashboard performance",
  
  "channels": {
    "stable": {
      "ui_version": "2025.12.30",
      "released": "2025-12-30T12:00:00Z",
      "files": [
        { 
          "path": "/dashboard.html", 
          "url": "https://raw.githubusercontent.com/ronkuijpers/Wordclock/v2025.12.30/data/dashboard.html",
          "sha256": "abc123..."
        },
        { 
          "path": "/admin.html", 
          "url": "https://raw.githubusercontent.com/ronkuijpers/Wordclock/v2025.12.30/data/admin.html",
          "sha256": "def456..."
        }
      ],
      "release_notes": "Dashboard improvements: better error handling and loading states"
    },
    "early": {
      "ui_version": "2025.12.29",
      "files": [...]
    }
  }
}
```

### Dashboard Integratie

Voeg toe aan `update.html` of `admin.html`:

```html
<section>
  <h3>UI Update</h3>
  <button id="checkUIUpdateBtn">Check for UI Update</button>
  <span id="uiUpdateStatus"></span>
  
  <h4>Manual UI Update</h4>
  <input type="text" id="uiTagInput" placeholder="GitHub tag (e.g. v2025.12.30)">
  <button id="updateUIFromTagBtn">Update from Tag</button>
  
  <input type="text" id="uiManifestInput" placeholder="Manifest URL">
  <button id="updateUIFromManifestBtn">Update from Manifest</button>
</section>
```

## Implementatie Stappen

1. ✅ Creëer `ui-manifest.json` template
2. ✅ Voeg `UI_MANIFEST_URL` constante toe
3. ✅ Implementeer `syncUIFromManifest()` functie
4. ✅ Voeg nieuwe endpoints toe
5. ✅ Update dashboard met UI update controls
6. ✅ Test met verschillende scenarios

## Voordelen van deze Aanpak

- **Flexibel**: Ondersteunt zowel manifest als directe tag updates
- **Toegankelijk**: Geen admin auth nodig (alleen UI auth)
- **Consistent**: Gebruikt bestaande infrastructuur
- **Versioned**: Ondersteunt versioning en channels
- **Eenvoudig**: Eenvoudig te gebruiken via dashboard of curl

## Gebruik

### Via Dashboard
- Klik op "Check for UI Update" button
- Of voer handmatig een tag/manifest URL in

### Via curl
```bash
# Check voor UI update
curl -X POST http://wordclock.local/checkUIUpdate

# Update van specifieke tag
curl -X POST "http://wordclock.local/updateUIFromTag?tag=v2025.12.30"

# Update van custom manifest
curl -X POST "http://wordclock.local/updateUIFromManifest?url=https://..."
```

## Veiligheid

- UI auth vereist (niet publiek)
- SHA256 checksums voor file verificatie (optioneel)
- HTTPS downloads
- Validatie van file paths (voorkomt directory traversal)


