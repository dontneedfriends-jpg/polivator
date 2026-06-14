#ifndef INDEX_H
#define INDEX_H

#include <Arduino.h>

const char index_html_content[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Plant Moisture Monitor</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
  background: #1a1a2e;
  color: #eee;
  font-family: 'Segoe UI', sans-serif;
  display: flex;
  justify-content: center;
  align-items: flex-start;
  min-height: 100vh;
  padding: 20px;
}
.container {
  max-width: 480px;
  width: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 20px;
}
.card {
  background: #16213e;
  border-radius: 16px;
  padding: 20px;
  width: 100%;
  box-shadow: 0 4px 15px rgba(0,0,0,0.4);
}
.gauge-wrapper {
  display: flex;
  flex-direction: column;
  align-items: center;
}
.gauge {
  width: 200px;
  height: 200px;
  position: relative;
}
.gauge svg {
  width: 100%;
  height: 100%;
  transform: rotate(-90deg);
}
.gauge circle {
  fill: none;
  stroke-width: 12;
}
.gauge .bg {
  stroke: #0f3460;
}
.gauge .fg {
  stroke: #00bcd4;
  stroke-linecap: round;
  transition: stroke-dashoffset 0.5s ease;
}
.gauge-value {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  font-size: 2.2rem;
  font-weight: bold;
  color: #00bcd4;
}
.metrics {
  display: flex;
  flex-wrap: wrap;
  justify-content: space-around;
  gap: 10px;
}
.metric {
  text-align: center;
  min-width: 80px;
}
.metric .label {
  font-size: 0.75rem;
  color: #7f8c8d;
  text-transform: uppercase;
  letter-spacing: 1px;
}
.metric .value {
  font-size: 1.2rem;
  font-weight: bold;
  color: #00bcd4;
}
.btn-group {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  justify-content: center;
}
.btn {
  background: #0f3460;
  color: #ddd;
  border: none;
  padding: 10px 16px;
  border-radius: 8px;
  cursor: pointer;
  font-size: 0.9rem;
  transition: background 0.2s;
}
.btn:hover {
  background: #1a5276;
}
.btn:active {
  background: #00bcd4;
  color: #1a1a2e;
}
.message {
  font-size: 0.85rem;
  color: #00bcd4;
  min-height: 1.2em;
}
canvas {
  display: block;
  margin: 0 auto;
  background: #0d1b2a;
  border-radius: 8px;
  width: 100%;
  height: auto;
}
</style>
</head>
<body>
<div class="container">
  <div class="card gauge-wrapper">
    <div class="gauge">
      <svg viewBox="0 0 200 200">
        <circle class="bg" cx="100" cy="100" r="88"/>
        <circle class="fg" cx="100" cy="100" r="88" stroke-dasharray="553.1 553.1" stroke-dashoffset="553.1"/>
      </svg>
      <div class="gauge-value">0%</div>
    </div>
    <div class="moisture-text" style="font-size:1.5rem; margin-top:10px;">Moisture: 0%</div>
  </div>

  <div class="card">
    <div class="metrics" id="metrics">
      <div class="metric"><span class="label">Raw ADC</span><span class="value" id="rawAdc">--</span></div>
      <div class="metric"><span class="label">Voltage</span><span class="value" id="voltage">--</span></div>
      <div class="metric"><span class="label">Dry Thresh</span><span class="value" id="dryThresh">--</span></div>
      <div class="metric"><span class="label">Wet Thresh</span><span class="value" id="wetThresh">--</span></div>
      <div class="metric"><span class="label">WiFi</span><span class="value" id="wifiStatus">--</span></div>
    </div>
  </div>

  <div class="card btn-group">
    <button class="btn" id="btnDry">Calibrate Dry</button>
    <button class="btn" id="btnWet">Calibrate Wet</button>
    <button class="btn" id="btnReset">Reset Calibration</button>
  </div>
  <div class="message" id="message"></div>

  <div class="card">
    <canvas id="chart" width="300" height="150"></canvas>
  </div>
</div>

<script>
(function(){
  const circumference = 2 * Math.PI * 88; // ≈ 552.92
  const fgCircle = document.querySelector('.gauge .fg');
  const gaugeValue = document.querySelector('.gauge-value');
  const moistureText = document.querySelector('.moisture-text');
  const chartCanvas = document.getElementById('chart');
  const ctx = chartCanvas.getContext('2d');
  const messageDiv = document.getElementById('message');
  
  const data = [];
  const maxPoints = 60;

  function setGauge(percent) {
    const offset = circumference - (percent / 100) * circumference;
    fgCircle.style.strokeDashoffset = offset;
    gaugeValue.textContent = Math.round(percent) + '%';
    moistureText.textContent = 'Moisture: ' + Math.round(percent) + '%';
  }

  function updateMetrics(status) {
    document.getElementById('rawAdc').textContent = status.raw;
    document.getElementById('voltage').textContent = status.voltage.toFixed(2) + 'V';
    document.getElementById('dryThresh').textContent = status.dryThreshold;
    document.getElementById('wetThresh').textContent = status.wetThreshold;
    document.getElementById('wifiStatus').textContent = status.wifiStatus;
  }

  function drawChart() {
    ctx.clearRect(0, 0, 300, 150);
    if (data.length === 0) return;
    const min = 0;
    const max = 100;
    const range = max - min;
    const stepX = 300 / (maxPoints - 1); 
    ctx.strokeStyle = '#00bcd4';
    ctx.lineWidth = 2;
    ctx.beginPath();
    for (let i = 0; i < data.length; i++) {
      const x = i * stepX;
      const y = 150 - ( (data[i] - min) / range ) * 140; // leave 5px margin
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }

  function addDataPoint(value) {
    data.push(value);
    if (data.length > maxPoints) data.shift();
    drawChart();
  }

  // SSE connection
  let eventSource = null;
  function connectSSE() {
    if (eventSource) eventSource.close();
    eventSource = new EventSource('/events');
    eventSource.addEventListener('status', function(e) {
      try {
        const status = JSON.parse(e.data);
        if (status.moisture !== undefined) {
          setGauge(status.moisture);
          addDataPoint(status.moisture);
        }
        updateMetrics(status);
      } catch(err) {
        console.error('SSE parse error', err);
      }
    });
    eventSource.onerror = function() {
      eventSource.close();
      setTimeout(connectSSE, 3000);
    };
  }
  connectSSE();

  // Buttons
  function postAndMessage(url, msg) {
    fetch(url, { method: 'POST' })
      .then(res => {
        messageDiv.textContent = msg;
        setTimeout(() => { messageDiv.textContent = ''; }, 3000);
      })
      .catch(err => {
        messageDiv.textContent = 'Error: ' + err;
      });
  }
  document.getElementById('btnDry').addEventListener('click', function() {
    postAndMessage('/calibrate-dry', 'Dry threshold set!');
  });
  document.getElementById('btnWet').addEventListener('click', function() {
    postAndMessage('/calibrate-wet', 'Wet threshold set!');
  });
  document.getElementById('btnReset').addEventListener('click', function() {
    postAndMessage('/reset-calibration', 'Calibration reset!');
  });

  // Initialize chart with empty data
  drawChart();
})();
</script>
</body>
</html>
)rawliteral";

#endif
