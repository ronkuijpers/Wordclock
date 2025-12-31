# Dashboard Verbeteringsplan

## Huidige Problemen

1. **Sequentieel laden**: Alle API calls worden direct na elkaar gedaan zonder prioritering
2. **Geen error handling**: Als één call faalt, is er geen goede feedback
3. **Geen loading states**: Gebruiker ziet niet welke data nog aan het laden is
4. **Geen retry mechanisme**: Bij tijdelijke fouten wordt niet opnieuw geprobeerd
5. **Afhankelijkheden**: Sommige calls kunnen elkaar blokkeren
6. **Geen timeout handling**: Calls kunnen oneindig hangen

## Verbeteringsplan

### 1. Data Loading Manager Systeem

Creëer een centraal systeem dat alle data loading beheert:

```javascript
class DataLoader {
  constructor() {
    this.loaders = new Map();
    this.retryDelays = [1000, 2000, 5000]; // Progressive backoff
  }

  // Registreer een data loader
  register(name, loaderFn, options = {}) {
    this.loaders.set(name, {
      fn: loaderFn,
      loading: false,
      lastError: null,
      retryCount: 0,
      interval: options.interval || null,
      priority: options.priority || 0, // Hoger = belangrijker
      timeout: options.timeout || 5000,
      onSuccess: options.onSuccess || null,
      onError: options.onError || null,
    });
  }

  // Laad één specifieke data source
  async load(name) {
    const loader = this.loaders.get(name);
    if (!loader || loader.loading) return;

    loader.loading = true;
    loader.lastError = null;

    try {
      const result = await Promise.race([
        loader.fn(),
        new Promise((_, reject) => 
          setTimeout(() => reject(new Error('Timeout')), loader.timeout)
        )
      ]);

      loader.retryCount = 0;
      if (loader.onSuccess) loader.onSuccess(result);
      return result;
    } catch (error) {
      loader.lastError = error;
      if (loader.onError) loader.onError(error);
      
      // Retry logic
      if (loader.retryCount < this.retryDelays.length) {
        const delay = this.retryDelays[loader.retryCount];
        loader.retryCount++;
        setTimeout(() => this.load(name), delay);
      }
    } finally {
      loader.loading = false;
    }
  }

  // Laad alle data sources (geprioriteerd)
  async loadAll() {
    const sorted = Array.from(this.loaders.entries())
      .sort((a, b) => b[1].priority - a[1].priority);
    
    // Laad eerst hoge prioriteit items
    const highPriority = sorted.filter(([_, l]) => l.priority >= 5);
    await Promise.allSettled(highPriority.map(([name]) => this.load(name)));
    
    // Laad dan rest parallel
    const rest = sorted.filter(([_, l]) => l.priority < 5);
    await Promise.allSettled(rest.map(([name]) => this.load(name)));
  }

  // Start interval loading voor specifieke loader
  startInterval(name) {
    const loader = this.loaders.get(name);
    if (!loader || !loader.interval) return;
    
    setInterval(() => this.load(name), loader.interval);
  }
}
```

### 2. Loading States per Component

Voeg visuele feedback toe voor elke data source:

```javascript
function setLoadingState(elementId, loading) {
  const el = document.getElementById(elementId);
  if (!el) return;
  
  if (loading) {
    el.classList.add('opacity-50');
    el.setAttribute('data-loading', 'true');
    // Voeg spinner toe indien nodig
  } else {
    el.classList.remove('opacity-50');
    el.removeAttribute('data-loading');
  }
}

function setErrorState(elementId, error) {
  const el = document.getElementById(elementId);
  if (!el) return;
  
  el.classList.add('text-red-600');
  el.setAttribute('title', `Error: ${error.message}`);
  el.textContent = 'Error';
}
```

### 3. Onafhankelijke Data Loaders

Elke data source krijgt zijn eigen loader met error handling:

```javascript
const dataLoader = new DataLoader();

// Status (hoge prioriteit, interval)
dataLoader.register('status', async () => {
  setLoadingState('statusIndicator', true);
  const res = await fetch('/status');
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.text();
}, {
  priority: 10,
  interval: 2000,
  onSuccess: (status) => {
    setLoadingState('statusIndicator', false);
    const on = status === 'on';
    document.getElementById('statusIndicator').className =
      'inline-block w-4 h-4 rounded-full ' + (on ? 'bg-green-500' : 'bg-red-500');
    document.getElementById('statusText').textContent = 
      on ? 'Wordclock on' : 'Wordclock off';
  },
  onError: (error) => {
    setLoadingState('statusIndicator', false);
    setErrorState('statusText', error);
  }
});

// Brightness (normale prioriteit)
dataLoader.register('brightness', async () => {
  setLoadingState('brightnessSlider', true);
  const res = await fetch('/getBrightness');
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.text();
}, {
  priority: 5,
  onSuccess: (val) => {
    setLoadingState('brightnessSlider', false);
    const s = document.getElementById('brightnessSlider');
    const i = document.getElementById('brightnessInput');
    const lbl = document.getElementById('brightnessValue2');
    if (s) s.value = val;
    if (i) i.value = val;
    if (lbl) lbl.textContent = val;
  },
  onError: (error) => {
    setLoadingState('brightnessSlider', false);
    setErrorState('brightnessValue2', error);
  }
});

// Versions (lage prioriteit, één keer)
dataLoader.register('versions', async () => {
  const [fwRes, uiRes] = await Promise.all([
    fetch('/version'),
    fetch('/uiversion')
  ]);
  if (!fwRes.ok || !uiRes.ok) throw new Error('Version fetch failed');
  return {
    firmware: await fwRes.text(),
    ui: await uiRes.text()
  };
}, {
  priority: 1,
  onSuccess: ({firmware, ui}) => {
    const fwEl = document.getElementById('version');
    const uiEl = document.getElementById('uiVersion');
    if (fwEl) fwEl.textContent = firmware;
    if (uiEl) uiEl.textContent = ui;
  },
  onError: (error) => {
    setErrorState('version', error);
    setErrorState('uiVersion', error);
  }
});

// Grid Variant (normale prioriteit)
dataLoader.register('gridVariant', async () => {
  const res = await fetch('/getGridVariant');
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}, {
  priority: 5,
  onSuccess: (v) => {
    const el = document.getElementById('gridVariantLabel');
    if (el) el.textContent = v.label || v.key || ('ID ' + v.id);
  },
  onError: (error) => {
    const el = document.getElementById('gridVariantLabel');
    if (el) el.textContent = 'n.v.t.';
  }
});

// Color (normale prioriteit)
dataLoader.register('color', async () => {
  const res = await fetch('/getColor');
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.text();
}, {
  priority: 5,
  onSuccess: (hex) => {
    const cp = document.getElementById('colorPicker');
    if (cp && hex && hex.length === 6) {
      cp.value = '#' + hex;
    }
  },
  onError: (error) => {
    console.warn('Failed to load color:', error);
  }
});

// Logs (lage prioriteit, interval)
dataLoader.register('logs', async () => {
  const res = await fetch('/log');
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.text();
}, {
  priority: 2,
  interval: 5000,
  onSuccess: (text) => {
    // Update log display
    updateLogDisplay(text);
  },
  onError: (error) => {
    console.warn('Failed to load logs:', error);
  }
});

// Log Summary (lage prioriteit)
dataLoader.register('logSummary', async () => {
  const res = await fetch('/api/logs/summary');
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}, {
  priority: 1,
  onSuccess: (j) => {
    const total = j.total_bytes || 0;
    const el = document.getElementById('logSummary');
    if (el) {
      el.textContent = `${formatBytes(total)} (${j.count || 0} files)`;
      setLogSummaryColor(total);
    }
  },
  onError: (error) => {
    const el = document.getElementById('logSummary');
    if (el) el.textContent = '-';
  }
});
```

### 4. Initialisatie

```javascript
// Bij page load
document.addEventListener('DOMContentLoaded', () => {
  // Laad alle data
  dataLoader.loadAll();
  
  // Start interval loaders
  dataLoader.startInterval('status');
  dataLoader.startInterval('logs');
  
  // Setup event listeners (onafhankelijk van data loading)
  setupEventListeners();
});
```

### 5. Voordelen van deze Aanpak

1. **Onafhankelijkheid**: Elke data source laadt onafhankelijk
2. **Fouttolerantie**: Als één call faalt, blijven anderen werken
3. **Visuele feedback**: Gebruiker ziet welke data nog laadt
4. **Retry mechanisme**: Automatische retry bij tijdelijke fouten
5. **Prioritering**: Belangrijke data laadt eerst
6. **Timeout handling**: Calls kunnen niet oneindig hangen
7. **Interval support**: Automatische refresh voor dynamische data
8. **Betere UX**: Gebruiker ziet direct wat werkt en wat niet

### 6. Implementatie Stappen

1. ✅ Creëer DataLoader class
2. ✅ Voeg loading states toe aan HTML (data-loading attributen)
3. ✅ Registreer alle data loaders
4. ✅ Update initialisatie code
5. ✅ Test error scenarios
6. ✅ Voeg retry feedback toe (optioneel: "Retrying...")
7. ✅ Voeg manual refresh buttons toe per sectie

### 7. Optionele Verbeteringen

- **Cache**: Cache data lokaal om onnodige calls te voorkomen
- **Batch API**: Creëer een `/api/dashboard/state` endpoint die alle data in één keer ophaalt
- **WebSocket**: Voor real-time updates in plaats van polling
- **Offline support**: Toon cached data als device offline is
- **Loading skeleton**: Toon placeholder content tijdens loading

## Prioriteit Implementatie

1. **Hoog**: DataLoader systeem + basis error handling
2. **Medium**: Loading states + retry mechanisme
3. **Laag**: Cache + batch API + WebSocket


