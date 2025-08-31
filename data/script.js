// script.js
function showTab(id) {
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('nav button').forEach(b => b.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  document.getElementById('btn-' + id).classList.add('active');
}

function toggleClock(cb) {
  fetch('/toggle?state=' + (cb.checked ? 'on' : 'off'));
  document.getElementById('clockStatusText').innerText = cb.checked ? 'Wordclock aan' : 'Wordclock uit';
}

function updateStatusAndLog() {
  fetch('/status')
    .then(r => r.text())
    .then(state => {
      const on = (state === 'on');
      document.getElementById('clockToggle').checked = on;
      document.getElementById('clockStatusText').innerText = on ? 'Wordclock aan' : 'Wordclock uit';
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
