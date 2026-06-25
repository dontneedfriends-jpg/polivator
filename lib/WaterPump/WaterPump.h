#ifndef WATER_PUMP_H
#define WATER_PUMP_H

#include <Arduino.h>
#include <Preferences.h>
#include "../Common/Types.h"

class SensorManager;
class Calibration;

class WaterPump {
public:
    WaterPump();
    // GPIO пина реле (active HIGH по умолчанию).
    void begin(uint8_t relayPin, bool activeHigh = true);
    // Включить мотор на durationMs миллисекунд. 0 = выключить сейчас.
    // Возвращает true, если команда принята.
    bool run(uint32_t durationMs);
    void stop();
    // Текущее состояние
    bool isRunning() const;
    uint32_t remainingMs() const;
    // Настройки калибровки (мл/сек)
    float getFlowRate() const;
    void setFlowRate(float mlPerSec);
    // Порог автополива (влажность в %)
    int getAutoThreshold() const;
    void setAutoThreshold(int percent);
    bool isAutoModeEnabled() const;
    void setAutoModeEnabled(bool enabled);
    // Вызывать из loop() — обслуживает таймеры и автополив
    void tick(SensorManager* sensors, Calibration* calibration);
    // Persist on every setter
    void save();

private:
    Preferences prefs;
    uint8_t m_pin;
    bool m_activeHigh;
    bool m_running;
    uint32_t m_stopAt;
    bool m_autoMode;
    int m_autoThreshold;     // процент, ниже которого включаем
    float m_flowRate;        // мл в секунду (для отображения объёма)
    void switchOn();
    void switchOff();
};

#endif // WATER_PUMP_H
