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
  <form action="/check_update" method="post">
  <button type="submit">Check for Update</button>
  </form>
  <br><br>
  <div><strong>Build versie:</strong> )rawliteral";
  html += FIRMWARE_VERSION;
  html += R"rawliteral(</div>
  <br><br>
  <label for="colorPicker">LED kleur:</label>
  <input type="color" id="colorPicker" name="colorPicker" value="#ffffff">

  <script>
    const picker = document.getElementById('colorPicker');
    picker.addEventListener('input', () => {
      // stuur gekozen hex kleur naar ESP32
      fetch(`/setColor?color=${picker.value.substring(1)}`)
        .then(resp => {
          if (!resp.ok) console.error('Kon kleur niet opslaan');
        });
    });
  </script>
  <br><br>
  <label for="brightnessSlider">Helderheid:</label>
  <input type="range" id="brightnessSlider" min="0" max="255" value="128" oninput="updateBrightness(this.value)">
  <script>
    function updateBrightness(val) {
      fetch(`/setBrightness?level=${val}`);
    }
  </script>
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