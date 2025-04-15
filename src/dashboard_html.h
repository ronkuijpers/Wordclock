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
  <div><strong>Build versie:</strong> )rawliteral";
  html += BUILD_VERSION;
  html += R"rawliteral(</div>
  <pre id='logBox'></pre>
</body>
</html>
)rawliteral";

  return html;
}