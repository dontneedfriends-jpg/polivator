#include "WaterPump.h"
#include "Sensor.h"
#include "Calibration.h"

WaterPump::WaterPump()
  : m_pin(38),
    m_activeHigh(true),
    m_running(false),
    m_stopAt(0),
    m_autoMode(false),
    m_autoThreshold(30),
    m_flowRate(1.0f) {} // ~60 ml/min default for KPHM100-style 12V dosing pump

void WaterPump::begin(uint8_t relayPin, bool activeHigh) {
    m_pin = relayPin;
    m_activeHigh = activeHigh;
    pinMode(m_pin, OUTPUT);
    switchOff();

    prefs.begin("pump", false);
    m_autoMode = prefs.getBool("auto", false);
    m_autoThreshold = prefs.getInt("threshold", 30);
    if (m_autoThreshold < 0) m_autoThreshold = 0;
    if (m_autoThreshold > 100) m_autoThreshold = 100;
    m_flowRate = prefs.getFloat("flow", 1.0f);
    if (m_flowRate < 0.05f) m_flowRate = 0.05f;
    if (m_flowRate > 50.0f) m_flowRate = 50.0f;
}

void WaterPump::save() {
    prefs.putBool("auto", m_autoMode);
    prefs.putInt("threshold", m_autoThreshold);
    prefs.putFloat("flow", m_flowRate);
}

bool WaterPump::run(uint32_t durationMs) {
    if (durationMs == 0) {
        stop();
        return true;
    }
    m_stopAt = millis() + durationMs;
    switchOn();
    m_running = true;
    return true;
}

void WaterPump::stop() {
    m_running = false;
    m_stopAt = 0;
    switchOff();
}

bool WaterPump::isRunning() const {
    return m_running;
}

uint32_t WaterPump::remainingMs() const {
    if (!m_running) return 0;
    uint32_t now = millis();
    if (m_stopAt <= now) return 0;
    return m_stopAt - now;
}

float WaterPump::getFlowRate() const { return m_flowRate; }
void WaterPump::setFlowRate(float mlPerSec) {
    if (mlPerSec < 0.05f) mlPerSec = 0.05f;
    if (mlPerSec > 50.0f) mlPerSec = 50.0f;
    m_flowRate = mlPerSec;
    save();
}

int WaterPump::getAutoThreshold() const { return m_autoThreshold; }
void WaterPump::setAutoThreshold(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    m_autoThreshold = percent;
    save();
}

bool WaterPump::isAutoModeEnabled() const { return m_autoMode; }
void WaterPump::setAutoModeEnabled(bool enabled) {
    m_autoMode = enabled;
    save();
}

void WaterPump::tick(SensorManager* sensors, Calibration* calibration) {
    uint32_t now = millis();

    // 1. Следим за таймером ручного режима
    if (m_running && m_stopAt != 0 && now >= m_stopAt) {
        stop();
    }

    // 2. Автополив: если ни один датчик ниже порога — включаем на 5 секунд,
    //    если все выше порога — выключаем.
    if (m_autoMode && sensors && calibration) {
        const SensorConfig* configs = calibration->getConfigs();
        bool needsWater = false;
        bool anyEnabled = false;
        for (uint8_t i = 0; i < MAX_SENSORS; i++) {
            if (!configs[i].enabled) continue;
            anyEnabled = true;
            int pct = sensors->getPercent(i);
            if (pct < m_autoThreshold) {
                needsWater = true;
                break;
            }
        }
        if (anyEnabled && needsWater && !m_running) {
            // Объём ~5 мл за раз; чем суше, тем больше.
            // При 60 мл/мин (1 мл/с) это ~5 с — комфортно для горшка.
            float volumeMl = 5.0f;
            if (m_autoThreshold < 20) volumeMl = 10.0f;
            if (m_autoThreshold < 10) volumeMl = 15.0f;
            uint32_t duration = (uint32_t)((volumeMl / m_flowRate) * 1000.0f);
            if (duration < 1000UL) duration = 1000UL;
            if (duration > 30000UL) duration = 30000UL;
            run(duration);
        }
    }
}

void WaterPump::switchOn() {
    digitalWrite(m_pin, m_activeHigh ? HIGH : LOW);
}

void WaterPump::switchOff() {
    digitalWrite(m_pin, m_activeHigh ? LOW : HIGH);
}
