#pragma once

#include "config.h"
#include <Arduino.h>

String getDashboardHTML(String logContent) {
  String html = R"rawliteral(
<html>
<head>
  <script>
    function toggleClock(cb) {
      fetch('/toggle?state=' + (cb.checked ? 'on' : 'off'));
    }

    function updateStatusAndLog() {
      // Update checkbox status
      fetch('/status')
        .then(r => r.text())
        .then(state => {
          document.getElementById('clockToggle').checked = (state === 'on');
        });

      // Update log content
      fetch('/log')
        .then(r => r.text())
        .then(content => {
          document.getElementById('logBox').textContent = content;
        });
    }

    function checkForUpdate() {
      fetch('/checkForUpdate')
        .then(response => {
          if (response.ok) {
            console.log("Update started...");
            // Geef gebruiker feedback:
            document.getElementById("status").innerText = "Update gestart. ESP herstart...";
            // Wacht 10 seconden en probeer dan opnieuw te laden
            setTimeout(() => {
              location.reload();
            }, 10000);
          } else {
            console.error("Update mislukt");
          }
        })
        .catch(err => console.error("Fout:", err));
    }
    
    const picker = document.getElementById('colorPicker');
    picker.addEventListener('input', () => {
      // stuur gekozen hex kleur naar ESP32
      fetch(`/setColor?color=${picker.value.substring(1)}`)
        .then(resp => {
          if (!resp.ok) console.error('Kon kleur niet opslaan');
        });
    });

    function updateBrightness(val) {
      document.getElementById("brightnessValue").innerText = val;
      fetch(`/setBrightness?level=${val}`);
    }

    window.addEventListener('DOMContentLoaded', () => {
      fetch('/getBrightness')
        .then(resp => resp.text())
        .then(val => {
          document.getElementById("brightnessSlider").value = val;
          document.getElementById("brightnessValue").innerText = val;
        })
        .catch(err => console.error("Kan helderheid niet ophalen:", err));
    });

    setInterval(updateStatusAndLog, 2000);
    window.onload = updateStatusAndLog;
  </script>
</head>
<body>
  <h1>Wordclock Log Dashboard</h1>
  <label>
    <input type='checkbox' id='clockToggle' onchange='toggleClock(this)'>
    Wordclock Aan/Uit
  </label>
  <br><br>
  <button onclick="if(confirm('Weet je zeker dat je de klok wilt herstarten?')) location.href='/restart';">
    Herstart Wordclock
  </button>
  <br><br>
  <button onclick="if(confirm('Weet je zeker dat je de WiFi wilt resetten?')) location.href='/resetwifi';">
  Reset WiFi
  </button>
  <br><br>
  <button onclick="if(confirm('Sequence starten?')) fetch('/startSequence');">
  Start LED Sequence
  </button>
  <br><br>
  <button onclick="checkForUpdate()">Check for updates</button>
  <p id="status"></p>
  <br><br>
  <div><strong>Build versie:</strong> )rawliteral";
  html += FIRMWARE_VERSION;
  html += R"rawliteral(</div>
  <br><br>
  <label for="colorPicker">LED kleur:</label>
  <input type="color" id="colorPicker" name="colorPicker" value="#ffffff">
  <br><br>
  <label for="brightnessSlider">Helderheid: <span id="brightnessValue">?</span></label>
  <input type="range" id="brightnessSlider" min="0" max="255" value="128" oninput="updateBrightness(this.value)">
  <br><br>
  <pre id='logBox'></pre>
  <h2>Upload New Firmware</h2>
  <form action="/uploadFirmware" method="POST" enctype="multipart/form-data">
    <label for="firmwareFile">Choose Firmware File:</label>
    <input type="file" id="firmwareFile" name="firmwareFile" accept=".bin" required>
    <br><br>
    <button type="submit">Upload</button>
  </form>
</body>
</html>
)rawliteral";

  return html;
}