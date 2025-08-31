#pragma once

#include "config.h"
#include <Arduino.h>

String getDashboardHTML(String logContent) {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Wordclock Dashboard</title>
  <style>
    body {
      margin: 0;
      font-family: sans-serif;
      background-color: #1A1F71;
      color: white;
    }
    nav {
      display: flex;
      background-color: #F7B600;
      overflow-x: auto;
    }
    nav button {
      flex: 1;
      padding: 1rem;
      background: none;
      border: none;
      font-size: 1rem;
      cursor: pointer;
      color: #1A1F71;
      font-weight: bold;
    }
    nav button.active {
      background-color: white;
      color: #1A1F71;
    }
    .tab {
      display: none;
      padding: 1rem;
    }
    .tab.active {
      display: block;
    }
    label, input, button, select {
      margin: 0.5rem 0;
      display: block;
    }
    input[type="color"] {
      padding: 0;
      border: none;
      width: 100%;
      height: 2rem;
    }
    input[type="range"] {
      width: 100%;
    }
    pre {
      background: #000;
      color: #0f0;
      padding: 1rem;
      overflow: auto;
      max-height: 300px;
    }
    @media (min-width: 600px) {
      .grid {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 1rem;
      }
    }
  </style>
  <script>
    function showTab(id) {
      document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
      document.querySelectorAll('nav button').forEach(b => b.classList.remove('active'));
      document.getElementById(id).classList.add('active');
      document.getElementById('btn-' + id).classList.add('active');
    }

    function updateStatusAndLog() {
      fetch('/status')
        .then(r => r.text())
        .then(state => {
          document.getElementById('clockToggle').checked = (state === 'on');
        });
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
            document.getElementById("status").innerText = "Update gestart. ESP herstart...";
            setTimeout(() => location.reload(), 10000);
          } else {
            alert("Update mislukt");
          }
        });
    }

    function updateBrightness(val) {
      document.getElementById("brightnessValue").innerText = val;
      fetch(`/setBrightness?level=${val}`);
    }

    window.addEventListener('DOMContentLoaded', () => {
      showTab('control');
      updateStatusAndLog();
      setInterval(updateStatusAndLog, 5000);

      fetch('/getBrightness')
        .then(resp => resp.text())
        .then(val => {
          document.getElementById("brightnessSlider").value = val;
          document.getElementById("brightnessValue").innerText = val;
        });

      document.getElementById('colorPicker').addEventListener('input', () => {
        const color = document.getElementById('colorPicker').value.substring(1);
        fetch(`/setColor?color=${color}`);
      });
    });
  </script>
</head>
<body>
  <nav>
    <button id="btn-control" onclick="showTab('control')">Bediening</button>
    <button id="btn-log" onclick="showTab('log')">Log</button>
    <button id="btn-update" onclick="showTab('update')">Instellingen</button>
  </nav>

  <div id="control" class="tab">
    <label><input type='checkbox' id='clockToggle' onchange='toggleClock(this)'> Wordclock Aan/Uit</label>
    <div class="grid">
      <div>
        <label for="colorPicker">LED kleur:</label>
        <input type="color" id="colorPicker" value="#ffffff">
      </div>
      <div>
        <label for="brightnessSlider">Helderheid: <span id="brightnessValue">?</span></label>
        <input type="range" id="brightnessSlider" min="0" max="255" value="128" oninput="updateBrightness(this.value)">
      </div>
    </div>
  </div>

  <div id="log" class="tab">
    <pre id="logBox">
)rawliteral";
  html += logContent;
  html += R"rawliteral(
    </pre>
  </div>

  <div id="update" class="tab">
    <button onclick="if(confirm('Herstart Wordclock?')) location.href='/restart';">Herstart Wordclock</button>
    <button onclick="if(confirm('WiFi resetten?')) location.href='/resetwifi';">Reset WiFi</button>
    <button onclick="if(confirm('Sequence starten?')) fetch('/startSequence');">Start LED Sequence</button>
    <button onclick="checkForUpdate()">Check for updates</button>
    <p id="status"></p>
    <h2>Firmware uploaden</h2>
    <form action="/uploadFirmware" method="POST" enctype="multipart/form-data">
      <input type="file" name="firmwareFile" accept=".bin" required>
      <button type="submit">Upload</button>
    </form>
    <p><strong>Build versie:</strong> )rawliteral";
  html += FIRMWARE_VERSION;
  html += R"rawliteral(</p>
  </div>
</body>
</html>
)rawliteral";
  return html;
}