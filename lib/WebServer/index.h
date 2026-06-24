#ifndef INDEX_H
#define INDEX_H

#include <Arduino.h>

const char index_html_content[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Plant Moisture Monitor</title>
<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Sans:wght@400;500;600&display=swap" rel="stylesheet">
<style>
:root {
  --background: #f4f4f4;
  --layer-01: #ffffff;
  --layer-02: #f4f4f4;
  --layer-hover-01: #e5e5e5;
  --field-01: #f4f4f4;
  --border-subtle-01: #e0e0e0;
  --border-strong-01: #8d8d8d;
  --text-primary: #161616;
  --text-secondary: #525252;
  --text-helper: #6f6f6f;
  --text-placeholder: rgba(22,22,22,0.55);
  --icon-primary: #161616;
  --interactive: #0f62fe;
  --interactive-hover: #0353e9;
  --interactive-active: #002d9c;
  --layer-selected-01: #e0e0e0;
  --focus: #0f62fe;
  --support-success: #24a148;
  --support-warning: #f1c21b;
  --support-error: #da1e28;
  --overlay: rgba(22,22,22,0.5);
  --shadow: 0 2px 6px rgba(0,0,0,0.15);
}
[data-theme="dark"] {
  --background: #161616;
  --layer-01: #262626;
  --layer-02: #393939;
  --layer-hover-01: #333333;
  --field-01: #393939;
  --border-subtle-01: #525252;
  --border-strong-01: #8d8d8d;
  --text-primary: #f4f4f4;
  --text-secondary: #c6c6c6;
  --text-helper: #a8a8a8;
  --text-placeholder: rgba(244,244,244,0.4);
  --icon-primary: #f4f4f4;
  --interactive: #0f62fe;
  --interactive-hover: #0353e9;
  --interactive-active: #002d9c;
  --layer-selected-01: #525252;
  --focus: #ffffff;
  --support-success: #42be65;
  --support-warning: #f1c21b;
  --support-error: #fa4d56;
  --overlay: rgba(0,0,0,0.6);
  --shadow: 0 2px 6px rgba(0,0,0,0.8);
}

* { margin: 0; padding: 0; box-sizing: border-box; }

body {
  background: var(--background);
  color: var(--text-primary);
  font-family: 'IBM Plex Sans', sans-serif;
  font-size: 16px;
  line-height: 1.5;
  min-height: 100vh;
  padding: 24px 16px 48px;
  transition: background 0.2s, color 0.2s;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
}

/* Header */
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 32px;
  padding: 0 8px;
}
.header h1 {
  font-size: 2rem;
  font-weight: 400;
  letter-spacing: -0.02em;
}
.header-right {
  display: flex;
  align-items: center;
  gap: 8px;
}

/* Tags / badges */
.tag {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  padding: 6px 12px;
  background: var(--layer-01);
  color: var(--text-secondary);
  font-size: 0.75rem;
  line-height: 1.333;
  border: 1px solid var(--border-subtle-01);
}
.tag.connected { color: var(--support-success); border-color: var(--support-success); }

/* Buttons */
button, .btn {
  font-family: inherit;
  font-size: 0.875rem;
  font-weight: 400;
  line-height: 1.285;
  padding: 11px 16px;
  border: 1px solid transparent;
  border-radius: 0;
  cursor: pointer;
  transition: background 0.15s, color 0.15s, border-color 0.15s;
  outline: none;
}
button:focus-visible, .btn:focus-visible { outline: 2px solid var(--focus); outline-offset: -2px; }
.btn-primary {
  background: var(--interactive);
  color: #ffffff;
  border-color: var(--interactive);
}
.btn-primary:hover { background: var(--interactive-hover); border-color: var(--interactive-hover); }
.btn-primary:active { background: var(--interactive-active); border-color: var(--interactive-active); }
.btn-secondary {
  background: var(--layer-02);
  color: var(--text-primary);
  border-color: var(--border-subtle-01);
}
.btn-secondary:hover { background: var(--layer-hover-01); }
.btn-ghost {
  background: transparent;
  color: var(--interactive);
  border-color: transparent;
}
.btn-ghost:hover { background: var(--layer-hover-01); }
.btn-danger {
  background: var(--support-error);
  color: #ffffff;
  border-color: var(--support-error);
}
.icon-btn {
  width: 40px;
  height: 40px;
  padding: 0;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  background: var(--layer-01);
  color: var(--icon-primary);
  border: 1px solid var(--border-subtle-01);
}
.icon-btn:hover { background: var(--layer-hover-01); }
.icon-btn svg { width: 16px; height: 16px; }

/* Tabs */
.tabs {
  display: flex;
  gap: 0;
  margin-bottom: 32px;
  border-bottom: 1px solid var(--border-subtle-01);
}
.tab {
  padding: 12px 16px;
  border: none;
  background: transparent;
  color: var(--text-secondary);
  font-family: inherit;
  font-size: 0.875rem;
  font-weight: 400;
  cursor: pointer;
  border-bottom: 2px solid transparent;
  transition: color 0.15s, border-color 0.15s, background 0.15s;
}
.tab:hover { color: var(--text-primary); background: var(--layer-hover-01); }
.tab.active { color: var(--interactive); border-bottom-color: var(--interactive); }
.tab-content { display: none; }
.tab-content.active { display: block; }

/* Dashboard */
.sensors-wrapper {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(256px, 1fr));
  gap: 16px;
}
.card {
  background: var(--layer-01);
  border: 1px solid var(--border-subtle-01);
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 16px;
  transition: background 0.2s, border-color 0.2s;
}
.card:hover { border-color: var(--border-strong-01); }
.card.disabled { opacity: 0.6; }
.card-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
}
.card-title {
  font-size: 0.875rem;
  font-weight: 600;
  line-height: 1.285;
  color: var(--text-primary);
}
.card-subtitle {
  font-size: 0.75rem;
  color: var(--text-secondary);
  margin-top: 4px;
}

/* Inputs */
.cds-input {
  width: 100%;
  background: var(--field-01);
  color: var(--text-primary);
  border: none;
  border-bottom: 1px solid var(--border-strong-01);
  padding: 11px 16px;
  font-family: inherit;
  font-size: 0.875rem;
  line-height: 1.285;
  outline: none;
  transition: background 0.15s, border-color 0.15s;
}
.cds-input::placeholder { color: var(--text-placeholder); }
.cds-input:hover { background: var(--layer-hover-01); }
.cds-input:focus { border-bottom-color: var(--interactive); outline: 2px solid var(--focus); outline-offset: -2px; }
.cds-input.pin-input { width: 72px; padding: 8px 12px; text-align: center; }

/* Gauge */
.gauge-wrapper {
  display: flex;
  flex-direction: column;
  align-items: center;
  width: 100%;
  padding: 8px 0;
}
.gauge {
  width: 120px;
  height: 120px;
  position: relative;
}
.gauge svg { width: 100%; height: 100%; transform: rotate(-90deg); }
.gauge circle { fill: none; stroke-width: 6; }
.gauge .bg { stroke: var(--border-subtle-01); }
.gauge .fg {
  stroke: var(--interactive);
  stroke-linecap: butt;
  transition: stroke-dashoffset 0.4s ease;
}
.gauge-value {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  font-size: 1.5rem;
  font-weight: 300;
  color: var(--text-primary);
}
.gauge-label {
  margin-top: 8px;
  font-size: 0.75rem;
  color: var(--text-secondary);
  text-transform: uppercase;
  letter-spacing: 0.32px;
}

/* Metrics */
.metrics {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 8px;
  width: 100%;
}
.calibration-metrics {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
  width: 100%;
}
.metric.metric-wide {
  grid-column: span 1;
}
.metric {
  background: var(--layer-02);
  padding: 12px;
  text-align: left;
}
.metric .label {
  display: block;
  font-size: 0.75rem;
  color: var(--text-secondary);
  text-transform: uppercase;
  letter-spacing: 0.32px;
  margin-bottom: 4px;
}
.metric .value {
  font-size: 0.875rem;
  font-weight: 400;
  color: var(--text-primary);
}

/* Button group */
.btn-group {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}
.btn-group .btn {
  flex: 1 1 auto;
  min-width: 64px;
  padding: 8px 12px;
  font-size: 0.75rem;
  text-transform: none;
}

/* Inline status message */
.message {
  min-height: 1.5em;
  font-size: 0.875rem;
  color: var(--text-secondary);
  margin-top: 24px;
  padding: 0 8px;
}

/* Pinout / Settings cards */
.content-card {
  background: var(--layer-01);
  border: 1px solid var(--border-subtle-01);
  padding: 24px;
  max-width: 720px;
}
.content-card h2, .content-card h3 {
  font-size: 1.25rem;
  font-weight: 400;
  margin-bottom: 24px;
}
.content-card h4 {
  font-size: 0.875rem;
  font-weight: 600;
  color: var(--interactive);
  margin-bottom: 12px;
  text-transform: uppercase;
  letter-spacing: 0.32px;
}

/* Data table */
.cds-table {
  width: 100%;
  border-collapse: collapse;
  margin-bottom: 24px;
  font-size: 0.875rem;
}
.cds-table th, .cds-table td {
  text-align: left;
  padding: 12px 16px;
  border-bottom: 1px solid var(--border-subtle-01);
}
.cds-table th {
  background: var(--layer-02);
  color: var(--text-secondary);
  font-weight: 600;
  font-size: 0.75rem;
  text-transform: uppercase;
  letter-spacing: 0.32px;
}
.cds-table tr:hover td { background: var(--layer-hover-01); }

/* Form */
.form-group {
  margin-bottom: 24px;
}
.form-group label {
  display: block;
  font-size: 0.75rem;
  color: var(--text-secondary);
  margin-bottom: 8px;
}
.form-actions {
  display: flex;
  gap: 16px;
  margin-top: 32px;
}
.form-actions button { min-width: 128px; }

.help-text {
  font-size: 0.75rem;
  color: var(--text-helper);
  margin-top: 4px;
}

.guide-list {
  margin: 12px 0 12px 24px;
  font-size: 0.875rem;
  line-height: 1.5;
  color: var(--text-secondary);
}
.guide-list li { margin-bottom: 8px; }
.guide-list strong { color: var(--text-primary); }

/* Modal */
.modal-overlay {
  display: none;
  position: fixed;
  z-index: 100;
  left: 0;
  top: 0;
  width: 100%;
  height: 100%;
  background: var(--overlay);
  justify-content: center;
  align-items: flex-start;
  padding-top: 80px;
}
.modal-content {
  background: var(--layer-01);
  border: 1px solid var(--border-subtle-01);
  padding: 24px;
  max-width: 384px;
  width: 90%;
  box-shadow: var(--shadow);
}
.modal-content h3 {
  font-size: 1.25rem;
  font-weight: 400;
  margin-bottom: 24px;
}
.modal-status {
  margin-top: 16px;
  font-size: 0.875rem;
  color: var(--text-secondary);
  min-height: 1.5em;
}

.form-row {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
  gap: 24px;
}
.settings-section {
  margin-bottom: 32px;
}
.settings-section h3 {
  font-size: 1rem;
  font-weight: 600;
  margin-bottom: 8px;
  color: var(--text-primary);
}
.checkbox-group {
  display: flex;
  flex-direction: column;
}
.checkbox-label {
  display: inline-flex;
  align-items: center;
  gap: 12px;
  font-size: 0.875rem;
  color: var(--text-primary);
  cursor: pointer;
}
.checkbox-label input[type="checkbox"] {
  width: 18px;
  height: 18px;
  accent-color: var(--interactive);
}
select.cds-input {
  appearance: none;
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='16' height='16' viewBox='0 0 16 16'%3E%3Cpath fill='%23c6c6c6' d='M8 11 3 6h10z'/%3E%3C/svg%3E");
  background-repeat: no-repeat;
  background-position: right 12px center;
  padding-right: 40px;
}
select.cds-input option {
  background: var(--field-01);
  color: var(--text-primary);
}

#otaInput { display: none; }
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <h1>Moisture Monitor</h1>
    <div class="header-right">
      <div class="tag" id="wifiBadge">
        <span id="wifiStatus">--</span>
      </div>
      <button class="icon-btn" id="themeToggle" aria-label="Toggle theme" title="Toggle theme">
        <span id="themeIcon">☀️</span>
      </button>
      <button class="icon-btn" id="otaBtn" aria-label="OTA Update" title="OTA Update">
        <span>⬆️</span>
      </button>
      <button class="btn-secondary" onclick="openModal()">Settings</button>
    </div>
  </div>

  <div class="tabs">
    <button class="tab active" data-tab="dashboard" onclick="switchTab('dashboard')">Dashboard</button>
    <button class="tab" data-tab="pinout" onclick="switchTab('pinout')">Pinout</button>
    <button class="tab" data-tab="settings" onclick="switchTab('settings')">Settings</button>
  </div>

  <div id="tab-dashboard" class="tab-content active">
    <div class="sensors-wrapper" id="sensorContainer"></div>
    <div class="message" id="message"></div>
  </div>

  <div id="tab-pinout" class="tab-content">
    <div class="content-card">
      <h3>Hardware Pinout</h3>
      <h4>E-Ink Display (WeActStudio 2.13" tri-color)</h4>
      <table class="cds-table">
        <thead><tr><th>Pin</th><th>GPIO</th></tr></thead>
        <tbody>
          <tr><td>BUSY</td><td>GPIO4</td></tr>
          <tr><td>RES / RST</td><td>GPIO16</td></tr>
          <tr><td>D/C</td><td>GPIO17</td></tr>
          <tr><td>CS</td><td>GPIO5</td></tr>
          <tr><td>SCK / SCL</td><td>GPIO18</td></tr>
          <tr><td>MOSI / SDA</td><td>GPIO21</td></tr>
          <tr><td>GND</td><td>GND</td></tr>
          <tr><td>VCC</td><td>3.3V</td></tr>
        </tbody>
      </table>
      <h4>Soil Moisture Sensors</h4>
      <table class="cds-table">
        <thead><tr><th>Sensor</th><th>Signal Pin</th><th>VCC</th><th>GND</th></tr></thead>
        <tbody>
          <tr><td>Sensor 0</td><td>GPIO6</td><td>3.3V</td><td>GND</td></tr>
          <tr><td>Sensor 1</td><td>GPIO7</td><td>3.3V</td><td>GND</td></tr>
          <tr><td>Sensor 2</td><td>GPIO8</td><td>3.3V</td><td>GND</td></tr>
          <tr><td>Sensor 3</td><td>GPIO9</td><td>3.3V</td><td>GND</td></tr>
          <tr><td>Sensor 4</td><td>GPIO10</td><td>3.3V</td><td>GND</td></tr>
        </tbody>
      </table>
      <p class="help-text">Note: Default pins are GPIO6–10. Use the Dashboard to reassign pins if needed.</p>
    </div>
  </div>

  <div id="tab-settings" class="tab-content">
    <div class="content-card">
      <h2>Settings</h2>

      <section class="settings-section">
        <h3>WiFi</h3>
        <p class="help-text">Configure the wireless network the device connects to. Changes require a reboot.</p>
        <div class="form-group">
          <label for="wifiSsidTab">WiFi SSID</label>
          <input type="text" id="wifiSsidTab" class="cds-input" placeholder="MyHomeNetwork" />
        </div>
        <div class="form-group">
          <label for="wifiPassTab">WiFi Password</label>
          <input type="password" id="wifiPassTab" class="cds-input" placeholder="Leave empty for open networks" />
          <p class="help-text">Password is stored on the device and never displayed again.</p>
        </div>
      </section>

      <section class="settings-section">
        <h3>Sensor Reading</h3>
        <p class="help-text">Control how often and how accurately the soil moisture sensors are sampled.</p>
        <div class="form-row">
          <div class="form-group">
            <label for="readIntervalTab">Read Interval (seconds)</label>
            <input type="number" id="readIntervalTab" class="cds-input" min="1" max="3600" value="2" />
            <p class="help-text">How often ADC values are read. 1–3600 s.</p>
          </div>
          <div class="form-group">
            <label for="adcSamplesTab">ADC Samples</label>
            <input type="number" id="adcSamplesTab" class="cds-input" min="1" max="100" value="10" />
            <p class="help-text">Samples averaged per reading. More samples reduce noise but take longer.</p>
          </div>
        </div>
      </section>

      <section class="settings-section">
        <h3>Web Interface</h3>
        <p class="help-text">Adjust how frequently the web dashboard receives live updates.</p>
        <div class="form-row">
          <div class="form-group">
            <label for="webIntervalTab">Web Update Interval (seconds)</label>
            <input type="number" id="webIntervalTab" class="cds-input" min="1" max="3600" value="2" />
            <p class="help-text">SSE message interval for the dashboard.</p>
          </div>
          <div class="form-group">
            <label for="displayIntervalTab">Display Refresh Interval (seconds)</label>
            <input type="number" id="displayIntervalTab" class="cds-input" min="1" max="3600" value="30" />
            <p class="help-text">How often the e-ink screen is fully redrawn.</p>
          </div>
        </div>

        <div class="form-group">
          <label for="updatePriorityTab">Update Priority</label>
          <select id="updatePriorityTab" class="cds-input">
            <option value="0">Time priority — update on fixed interval</option>
            <option value="1">Change priority — update only when values change significantly</option>
            <option value="2">Balanced — interval updates with change detection</option>
          </select>
          <p class="help-text">Strategy for refreshing the screen and sending web events.</p>
        </div>
      </section>

      <section class="settings-section">
        <h3>Display</h3>
        <div class="form-row">
          <div class="form-group">
            <label for="displayUnitTab">Default Display Unit</label>
            <select id="displayUnitTab" class="cds-input">
              <option value="0">Percentage (%)</option>
              <option value="1">Raw ADC value (0–4095)</option>
              <option value="2">Voltage (V)</option>
            </select>
          </div>
        </div>
      </section>

      <section class="settings-section">
        <h3>Device Behavior</h3>
        <div class="form-row">
          <div class="form-group">
            <label for="sleepModeTab">Sleep Mode</label>
            <select id="sleepModeTab" class="cds-input">
              <option value="0">Disabled — always active</option>
              <option value="1">Light sleep — CPU paused between readings</option>
              <option value="2">Deep sleep — wake on timer (web UI unavailable)</option>
            </select>
            <p class="help-text">Power saving mode. Deep sleep disables the web interface.</p>
          </div>
        </div>

        <div class="form-row">
          <div class="form-group checkbox-group">
            <label class="checkbox-label">
              <input type="checkbox" id="otaEnabledTab" />
              <span>Enable OTA firmware updates</span>
            </label>
            <p class="help-text">Allow uploading new firmware through the web interface.</p>
          </div>

          <div class="form-group checkbox-group">
            <label class="checkbox-label">
              <input type="checkbox" id="debugEnabledTab" />
              <span>Enable serial debug output</span>
            </label>
            <p class="help-text">Send diagnostic messages to the serial port.</p>
          </div>
        </div>
      </section>

      <div class="form-actions">
        <button class="btn-secondary" onclick="resetSettingsDefaults()">Reset to Defaults</button>
        <button class="btn-primary" onclick="saveSettingsTab()">Save Settings</button>
      </div>
      <div class="modal-status" id="settingsStatusTab"></div>

      <section class="settings-section" style="margin-top: 48px; padding-top: 24px; border-top: 1px solid var(--border-subtle-01);">
        <h3>GPIO Pin Guide</h3>
        <p class="help-text" style="margin-bottom: 12px;">Sensor pins must be ADC-capable GPIO. Avoid pins already used by other peripherals.</p>
        <ul class="guide-list">
          <li><strong>GPIO1, GPIO3:</strong> used by Serial/UART (unless USB-CDC only).</li>
          <li><strong>GPIO4, GPIO5:</strong> used by E-Ink display (BUSY, CS).</li>
          <li><strong>GPIO6–GPIO10:</strong> internal flash, do not use.</li>
          <li><strong>GPIO11–GPIO20:</strong> ADC2 pins. Reading them while WiFi is active may fail.</li>
          <li><strong>Recommended:</strong> GPIO2, GPIO12–GPIO15, GPIO38–GPIO48 (if available and free).</li>
        </ul>
        <p class="help-text">Set the GPIO pin for each sensor in its card on the Dashboard tab.</p>
      </section>
    </div>
  </div>
</div>

<input type="file" id="otaInput" accept=".bin,.bin.gz" />

<div id="settingsModal" class="modal-overlay">
  <div class="modal-content" style="max-width: 640px; max-height: 90vh; overflow-y: auto;">
    <h3>Settings</h3>

    <section class="settings-section">
      <h3>WiFi</h3>
      <p class="help-text">Configure the wireless network the device connects to. Changes require a reboot.</p>
      <div class="form-row">
        <div class="form-group">
          <label for="wifiSsid">WiFi SSID</label>
          <input type="text" id="wifiSsid" class="cds-input" placeholder="MyHomeNetwork">
        </div>
        <div class="form-group">
          <label for="wifiPass">WiFi Password</label>
          <input type="password" id="wifiPass" class="cds-input" placeholder="Leave empty for open networks">
          <p class="help-text">Password is stored on the device and never displayed again.</p>
        </div>
      </div>
    </section>

    <section class="settings-section">
      <h3>Sensor Reading</h3>
      <p class="help-text">Control how often and how accurately the soil moisture sensors are sampled.</p>
      <div class="form-row">
        <div class="form-group">
          <label for="readInterval">Read Interval (seconds)</label>
          <input type="number" id="readInterval" class="cds-input" min="1" max="3600" value="2">
          <p class="help-text">How often ADC values are read. 1–3600 s.</p>
        </div>
        <div class="form-group">
          <label for="adcSamples">ADC Samples</label>
          <input type="number" id="adcSamples" class="cds-input" min="1" max="100" value="10">
          <p class="help-text">Samples averaged per reading. More samples reduce noise but take longer.</p>
        </div>
      </div>
    </section>

    <section class="settings-section">
      <h3>Web Interface</h3>
      <p class="help-text">Adjust how frequently the web dashboard receives live updates.</p>
      <div class="form-row">
        <div class="form-group">
          <label for="webInterval">Web Update Interval (seconds)</label>
          <input type="number" id="webInterval" class="cds-input" min="1" max="3600" value="2">
          <p class="help-text">SSE message interval for the dashboard.</p>
        </div>
        <div class="form-group">
          <label for="displayInterval">Display Refresh Interval (seconds)</label>
          <input type="number" id="displayInterval" class="cds-input" min="1" max="3600" value="30">
          <p class="help-text">How often the e-ink screen is fully redrawn.</p>
        </div>
      </div>

      <div class="form-group">
        <label for="updatePriority">Update Priority</label>
        <select id="updatePriority" class="cds-input">
          <option value="0">Time priority — update on fixed interval</option>
          <option value="1">Change priority — update only when values change significantly</option>
          <option value="2">Balanced — interval updates with change detection</option>
        </select>
        <p class="help-text">Strategy for refreshing the screen and sending web events.</p>
      </div>
    </section>

    <section class="settings-section">
      <h3>Display</h3>
      <div class="form-row">
        <div class="form-group">
          <label for="displayUnit">Default Display Unit</label>
          <select id="displayUnit" class="cds-input">
            <option value="0">Percentage (%)</option>
            <option value="1">Raw ADC value (0–4095)</option>
            <option value="2">Voltage (V)</option>
          </select>
        </div>
      </div>
    </section>

    <section class="settings-section">
      <h3>Device Behavior</h3>
      <div class="form-row">
        <div class="form-group">
          <label for="sleepMode">Sleep Mode</label>
          <select id="sleepMode" class="cds-input">
            <option value="0">Disabled — always active</option>
            <option value="1">Light sleep — CPU paused between readings</option>
            <option value="2">Deep sleep — wake on timer (web UI unavailable)</option>
          </select>
          <p class="help-text">Power saving mode. Deep sleep disables the web interface.</p>
        </div>
      </div>

      <div class="form-row">
        <div class="form-group checkbox-group">
          <label class="checkbox-label">
            <input type="checkbox" id="otaEnabled" />
            <span>Enable OTA firmware updates</span>
          </label>
          <p class="help-text">Allow uploading new firmware through the web interface.</p>
        </div>

        <div class="form-group checkbox-group">
          <label class="checkbox-label">
            <input type="checkbox" id="debugEnabled" />
            <span>Enable serial debug output</span>
          </label>
          <p class="help-text">Send diagnostic messages to the serial port.</p>
        </div>
      </div>
    </section>

    <div class="form-actions">
      <button class="btn-secondary" onclick="resetSettingsModalDefaults()">Reset to Defaults</button>
      <button class="btn-primary" onclick="saveSettings()">Save Settings</button>
    </div>
    <div class="modal-status" id="wifiStatusModal"></div>
  </div>
</div>

<script>
(function(){
  const circumference = 2 * Math.PI * 54;
  const escapeHtml = (unsafe) => {
    return unsafe.replace(/[&<>"']/g, function(m) {
      if (m === '&') return '&amp;';
      if (m === '<') return '&lt;';
      if (m === '>') return '&gt;';
      if (m === '"') return '&quot;';
      if (m === "'") return '&#039;';
    });
  };
  const messageDiv = document.getElementById('message');
  const sensorContainer = document.getElementById('sensorContainer');
  const wifiStatus = document.getElementById('wifiStatus');
  const wifiBadge = document.getElementById('wifiBadge');
  const cards = {};

  function switchTab(tabName) {
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.tab-content').forEach(tc => tc.classList.remove('active'));
    document.querySelector(`.tab[data-tab="${tabName}"]`).classList.add('active');
    document.getElementById('tab-' + tabName).classList.add('active');
  }
  window.switchTab = switchTab;

  function createOrUpdateSensorCard(index, data) {
    let card = cards[index];
    if (!card) {
      card = document.createElement('div');
      card.className = 'card';
      card.innerHTML = `
        <div class="card-header">
          <div>
            <input class="cds-input" id="name-${index}" value="${escapeHtml(data.name || '')}" />
            <div class="card-subtitle">Sensor ${index}</div>
          </div>
          <input class="cds-input pin-input" id="pin-${index}" type="number" min="0" max="48" value="${escapeHtml(String(data.pin || ''))}" title="GPIO pin" />
        </div>
        <div class="gauge-wrapper">
          <div class="gauge">
            <svg viewBox="0 0 120 120">
              <circle class="bg" cx="60" cy="60" r="54"/>
              <circle class="fg" id="gauge-${index}" cx="60" cy="60" r="54" stroke-dasharray="${circumference}" stroke-dashoffset="${circumference}"/>
            </svg>
            <div class="gauge-value" id="gauge-value-${index}">0%</div>
          </div>
          <div class="gauge-label">Moisture</div>
        </div>
        <div class="metrics">
          <div class="metric"><span class="label">Raw</span><span class="value" id="raw-${index}">${escapeHtml(String(data.raw ?? ''))}</span></div>
          <div class="metric"><span class="label">Voltage</span><span class="value" id="voltage-${index}">${escapeHtml(String(data.voltage ?? ''))}V</span></div>
        </div>
        <div class="calibration-metrics">
          <div class="metric"><span class="label">Dry</span><span class="value" id="dry-${index}">${escapeHtml(String(data.dry ?? ''))}</span></div>
          <div class="metric"><span class="label">Wet</span><span class="value" id="wet-${index}">${escapeHtml(String(data.wet ?? ''))}</span></div>
          <div class="metric metric-wide"><span class="label">GPIO</span><span class="value" id="pinval-${index}">${escapeHtml(String(data.pin ?? ''))}</span></div>
        </div>
        <div class="btn-group" id="btns-${index}"></div>
      `;
      sensorContainer.appendChild(card);
      cards[index] = card;
      const nameInput = card.querySelector(`#name-${index}`);
      nameInput.addEventListener('change', function() {
        const newName = this.value.trim();
        if (!newName) return;
        fetch(`/api/config/sensor?index=${index}`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({name: newName})
        }).then(() => {
          messageDiv.textContent = 'Name updated for sensor ' + index;
          setTimeout(() => { messageDiv.textContent = ''; }, 2000);
        }).catch(() => { messageDiv.textContent = 'Error updating name'; });
      });
      const pinInput = card.querySelector(`#pin-${index}`);
      pinInput.addEventListener('change', function() {
        const newPin = parseInt(this.value);
        if (isNaN(newPin) || newPin < 0 || newPin > 48) return;
        fetch(`/api/config/sensor?index=${index}`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({pin: newPin})
        }).then(() => {
          messageDiv.textContent = 'Pin updated for sensor ' + index;
          setTimeout(() => { messageDiv.textContent = ''; }, 2000);
        }).catch(() => { messageDiv.textContent = 'Error updating pin'; });
      });
    }
    const fgCircle = card.querySelector(`#gauge-${index}`);
    const gaugeValue = card.querySelector(`#gauge-value-${index}`);
    const rawSpan = card.querySelector(`#raw-${index}`);
    const voltageSpan = card.querySelector(`#voltage-${index}`);
    const drySpan = card.querySelector(`#dry-${index}`);
    const wetSpan = card.querySelector(`#wet-${index}`);
    const pinValSpan = card.querySelector(`#pinval-${index}`);
    const nameInput = card.querySelector(`#name-${index}`);
    const pinInput = card.querySelector(`#pin-${index}`);
    const btnGroup = card.querySelector(`#btns-${index}`);

    card.classList.toggle('disabled', !data.enabled);
    const pct = typeof data.percentage === 'number' ? data.percentage : (parseFloat(data.percentage) || 0);
    const raw = typeof data.raw === 'number' ? data.raw : (parseInt(data.raw) || 0);
    const voltage = typeof data.voltage === 'number' ? data.voltage : (parseFloat(data.voltage) || 0);
    const dry = typeof data.dry === 'number' ? data.dry : (parseInt(data.dry) || 0);
    const wet = typeof data.wet === 'number' ? data.wet : (parseInt(data.wet) || 0);

    const offset = circumference - (pct / 100) * circumference;
    fgCircle.style.strokeDashoffset = offset;
    gaugeValue.textContent = Math.round(pct) + '%';
    rawSpan.textContent = raw;
    voltageSpan.textContent = voltage.toFixed(2) + 'V';
    if (drySpan) drySpan.textContent = dry;
    if (wetSpan) wetSpan.textContent = wet;
    if (pinValSpan) pinValSpan.textContent = data.pin !== undefined ? data.pin : '-';
    if (data.name && nameInput.value !== data.name) nameInput.value = data.name;
    if (data.pin !== undefined && pinInput && parseInt(pinInput.value) !== data.pin) pinInput.value = data.pin;

    btnGroup.innerHTML = '';
    if (data.enabled) {
      const dryBtn = document.createElement('button');
      dryBtn.className = 'btn btn-secondary';
      dryBtn.textContent = 'Dry';
      dryBtn.onclick = function() { calibrateDry(index); };
      btnGroup.appendChild(dryBtn);
      const wetBtn = document.createElement('button');
      wetBtn.className = 'btn btn-secondary';
      wetBtn.textContent = 'Wet';
      wetBtn.onclick = function() { calibrateWet(index); };
      btnGroup.appendChild(wetBtn);
      const resetBtn = document.createElement('button');
      resetBtn.className = 'btn btn-secondary';
      resetBtn.textContent = 'Reset';
      resetBtn.onclick = function() { calibrateReset(index); };
      btnGroup.appendChild(resetBtn);
      const delBtn = document.createElement('button');
      delBtn.className = 'btn btn-danger';
      delBtn.textContent = 'Delete';
      delBtn.onclick = function() { deleteSensor(index); };
      btnGroup.appendChild(delBtn);
    } else {
      const restoreBtn = document.createElement('button');
      restoreBtn.className = 'btn btn-primary';
      restoreBtn.textContent = 'Restore';
      restoreBtn.onclick = function() { enableSensor(index); };
      btnGroup.appendChild(restoreBtn);
    }
  }

  // SSE
  let eventSource = null;
  function connectSSE() {
    if (eventSource) eventSource.close();
    eventSource = new EventSource('/events');
    eventSource.addEventListener('status', function(e) {
      try {
        const data = JSON.parse(e.data);
        if (data.sensors) {
          data.sensors.forEach((sensor, index) => {
            if (sensor && typeof sensor === 'object') {
              createOrUpdateSensorCard(index, sensor);
            }
          });
        }
        if (data.wifi) {
          wifiStatus.textContent = data.wifi;
          wifiBadge.className = 'tag';
          if (data.wifi === 'connected') {
            wifiBadge.classList.add('connected');
            wifiStatus.textContent = 'Connected';
          } else if (data.wifi === 'ap') {
            wifiBadge.classList.add('connected');
            wifiStatus.textContent = 'AP Mode';
          } else {
            wifiStatus.textContent = 'Disconnected';
          }
        }
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

  function calibrateDry(index) {
    messageDiv.textContent = 'Setting dry for sensor ' + index + '...';
    fetch(`/api/calibrate/dry?index=${index}`, { method: 'POST' })
      .then(async response => { messageDiv.textContent = await response.text(); setTimeout(() => { messageDiv.textContent = ''; }, 2000); })
      .catch(err => { messageDiv.textContent = 'Error: ' + err; });
  }
  window.calibrateDry = calibrateDry;

  function calibrateWet(index) {
    messageDiv.textContent = 'Setting wet for sensor ' + index + '...';
    fetch(`/api/calibrate/wet?index=${index}`, { method: 'POST' })
      .then(async response => { messageDiv.textContent = await response.text(); setTimeout(() => { messageDiv.textContent = ''; }, 2000); })
      .catch(err => { messageDiv.textContent = 'Error: ' + err; });
  }
  window.calibrateWet = calibrateWet;

  function calibrateReset(index) {
    messageDiv.textContent = 'Resetting calibration for sensor ' + index + '...';
    fetch(`/api/calibrate/reset?index=${index}`, { method: 'POST' })
      .then(async response => { messageDiv.textContent = await response.text(); setTimeout(() => { messageDiv.textContent = ''; }, 2000); })
      .catch(err => { messageDiv.textContent = 'Error: ' + err; });
  }
  window.calibrateReset = calibrateReset;

  function deleteSensor(index) {
    messageDiv.textContent = 'Deleting sensor ' + index + '...';
    fetch(`/api/sensor?index=${index}`, { method: 'DELETE' })
      .then(async response => { messageDiv.textContent = await response.text(); setTimeout(() => { messageDiv.textContent = ''; }, 2000); })
      .catch(err => { messageDiv.textContent = 'Error: ' + err; });
  }
  window.deleteSensor = deleteSensor;

  function enableSensor(index) {
    messageDiv.textContent = 'Enabling sensor ' + index + '...';
    fetch(`/api/sensor/enable?index=${index}`, { method: 'POST' })
      .then(async response => { messageDiv.textContent = await response.text(); setTimeout(() => { messageDiv.textContent = ''; }, 2000); })
      .catch(err => { messageDiv.textContent = 'Error: ' + err; });
  }
  window.enableSensor = enableSensor;

  window.openModal = function() {
    document.getElementById('settingsModal').style.display = 'flex';
    fetch('/api/wifi')
      .then(response => response.json())
      .then(data => { document.getElementById('wifiSsid').value = data.ssid || ''; })
      .catch(() => {});
    fetch('/api/settings')
      .then(response => response.json())
      .then(data => {
        if (data.readInterval !== undefined) document.getElementById('readInterval').value = data.readInterval;
        if (data.displayInterval !== undefined) document.getElementById('displayInterval').value = data.displayInterval;
        if (data.webInterval !== undefined) document.getElementById('webInterval').value = data.webInterval;
        if (data.updatePriority !== undefined) document.getElementById('updatePriority').value = data.updatePriority;
        if (data.sleepMode !== undefined) document.getElementById('sleepMode').value = data.sleepMode;
        if (data.otaEnabled !== undefined) document.getElementById('otaEnabled').checked = data.otaEnabled;
        if (data.debugEnabled !== undefined) document.getElementById('debugEnabled').checked = data.debugEnabled;
        if (data.displayUnit !== undefined) document.getElementById('displayUnit').value = data.displayUnit;
        if (data.adcSamples !== undefined) document.getElementById('adcSamples').value = data.adcSamples;
      })
      .catch(() => {});
  };
  window.closeModal = function() {
    document.getElementById('settingsModal').style.display = 'none';
  };
  window.saveSettings = function() {
    const ssid = document.getElementById('wifiSsid').value.trim();
    const pass = document.getElementById('wifiPass').value.trim();
    const readInterval = document.getElementById('readInterval').value.trim();
    const displayInterval = document.getElementById('displayInterval').value.trim();
    const webInterval = document.getElementById('webInterval').value.trim();
    const updatePriority = document.getElementById('updatePriority').value;
    const sleepMode = document.getElementById('sleepMode').value;
    const otaEnabled = document.getElementById('otaEnabled').checked;
    const debugEnabled = document.getElementById('debugEnabled').checked;
    const displayUnit = document.getElementById('displayUnit').value;
    const adcSamples = document.getElementById('adcSamples').value.trim();
    const statusEl = document.getElementById('wifiStatusModal');
    if (ssid) {
      const wifiFormData = new URLSearchParams();
      wifiFormData.append('ssid', ssid);
      if (pass) wifiFormData.append('pass', pass);
      fetch('/api/config/wifi', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: wifiFormData.toString()
      }).then(async r => { statusEl.textContent = await r.text(); });
    }
    fetch('/api/settings', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({
        readInterval: parseInt(readInterval) || 2,
        displayInterval: parseInt(displayInterval) || 30,
        webInterval: parseInt(webInterval) || 2,
        updatePriority: parseInt(updatePriority) || 0,
        sleepMode: parseInt(sleepMode) || 0,
        otaEnabled: !!otaEnabled,
        debugEnabled: !!debugEnabled,
        displayUnit: parseInt(displayUnit) || 0,
        adcSamples: parseInt(adcSamples) || 10
      })
    }).then(() => {
      if (!ssid) statusEl.textContent = 'Settings saved';
    }).catch(() => { statusEl.textContent = 'Error saving settings'; });
  };

  window.resetSettingsModalDefaults = function() {
    document.getElementById('readInterval').value = 2;
    document.getElementById('displayInterval').value = 30;
    document.getElementById('webInterval').value = 2;
    document.getElementById('updatePriority').value = 0;
    document.getElementById('sleepMode').value = 0;
    document.getElementById('otaEnabled').checked = true;
    document.getElementById('debugEnabled').checked = true;
    document.getElementById('displayUnit').value = 0;
    document.getElementById('adcSamples').value = 10;
    document.getElementById('wifiStatusModal').textContent = 'Defaults restored. Click Save to apply.';
  };

  window.saveSettingsTab = function() {
    const ssid = document.getElementById('wifiSsidTab').value.trim();
    const pass = document.getElementById('wifiPassTab').value.trim();
    const readInterval = document.getElementById('readIntervalTab').value.trim();
    const displayInterval = document.getElementById('displayIntervalTab').value.trim();
    const webInterval = document.getElementById('webIntervalTab').value.trim();
    const updatePriority = document.getElementById('updatePriorityTab').value;
    const sleepMode = document.getElementById('sleepModeTab').value;
    const otaEnabled = document.getElementById('otaEnabledTab').checked;
    const debugEnabled = document.getElementById('debugEnabledTab').checked;
    const displayUnit = document.getElementById('displayUnitTab').value;
    const adcSamples = document.getElementById('adcSamplesTab').value.trim();
    const statusEl = document.getElementById('settingsStatusTab');
    if (ssid) {
      const wifiFormData = new URLSearchParams();
      wifiFormData.append('ssid', ssid);
      if (pass) wifiFormData.append('pass', pass);
      fetch('/api/config/wifi', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: wifiFormData.toString()
      }).then(async r => { statusEl.textContent = await r.text(); });
    }
    fetch('/api/settings', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({
        readInterval: parseInt(readInterval) || 2,
        displayInterval: parseInt(displayInterval) || 30,
        webInterval: parseInt(webInterval) || 2,
        updatePriority: parseInt(updatePriority) || 0,
        sleepMode: parseInt(sleepMode) || 0,
        otaEnabled: !!otaEnabled,
        debugEnabled: !!debugEnabled,
        displayUnit: parseInt(displayUnit) || 0,
        adcSamples: parseInt(adcSamples) || 10
      })
    }).then(() => {
      if (!ssid) statusEl.textContent = 'Settings saved';
    }).catch(() => { statusEl.textContent = 'Error'; });
  };

  window.resetSettingsDefaults = function() {
    document.getElementById('readIntervalTab').value = 2;
    document.getElementById('displayIntervalTab').value = 30;
    document.getElementById('webIntervalTab').value = 2;
    document.getElementById('updatePriorityTab').value = 0;
    document.getElementById('sleepModeTab').value = 0;
    document.getElementById('otaEnabledTab').checked = true;
    document.getElementById('debugEnabledTab').checked = true;
    document.getElementById('displayUnitTab').value = 0;
    document.getElementById('adcSamplesTab').value = 10;
    document.getElementById('settingsStatusTab').textContent = 'Defaults restored. Click Save to apply.';
  };

  function loadSettings() {
    fetch('/api/settings')
      .then(response => response.json())
      .then(data => {
        if (data.readInterval !== undefined) document.getElementById('readIntervalTab').value = data.readInterval;
        if (data.displayInterval !== undefined) document.getElementById('displayIntervalTab').value = data.displayInterval;
        if (data.webInterval !== undefined) document.getElementById('webIntervalTab').value = data.webInterval;
        if (data.updatePriority !== undefined) document.getElementById('updatePriorityTab').value = data.updatePriority;
        if (data.sleepMode !== undefined) document.getElementById('sleepModeTab').value = data.sleepMode;
        if (data.otaEnabled !== undefined) document.getElementById('otaEnabledTab').checked = data.otaEnabled;
        if (data.debugEnabled !== undefined) document.getElementById('debugEnabledTab').checked = data.debugEnabled;
        if (data.displayUnit !== undefined) document.getElementById('displayUnitTab').value = data.displayUnit;
        if (data.adcSamples !== undefined) document.getElementById('adcSamplesTab').value = data.adcSamples;
      })
      .catch(() => {});
  }
  loadSettings();

  const themeToggle = document.getElementById('themeToggle');
  const themeIcon = document.getElementById('themeIcon');
  const currentTheme = localStorage.getItem('theme') || 'dark';
  document.documentElement.setAttribute('data-theme', currentTheme);
  themeIcon.textContent = currentTheme === 'dark' ? '☀️' : '🌙';
  themeToggle.addEventListener('click', function() {
    const newTheme = document.documentElement.getAttribute('data-theme') === 'dark' ? 'light' : 'dark';
    document.documentElement.setAttribute('data-theme', newTheme);
    localStorage.setItem('theme', newTheme);
    themeIcon.textContent = newTheme === 'dark' ? '☀️' : '🌙';
  });

  const otaBtn = document.getElementById('otaBtn');
  const otaInput = document.getElementById('otaInput');
  otaBtn.addEventListener('click', function() { otaInput.click(); });
  otaInput.addEventListener('change', function() {
    const file = this.files[0];
    if (!file) return;
    const formData = new FormData();
    formData.append('firmware', file);
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/update', true);
    xhr.upload.onprogress = function(e) {
      if (e.lengthComputable) {
        messageDiv.textContent = 'Uploading: ' + Math.round((e.loaded / e.total) * 100) + '%';
      }
    };
    xhr.onload = function() {
      messageDiv.textContent = 'Update result: ' + xhr.responseText;
      if (xhr.status === 200) setTimeout(() => { location.reload(); }, 5000);
    };
    xhr.send(formData);
  });
})();
</script>
</body>
</html>
)rawliteral";

#endif
