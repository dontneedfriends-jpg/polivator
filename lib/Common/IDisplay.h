#ifndef IDISPLAY_H
#define IDISPLAY_H

#include <Arduino.h>
#include "Types.h"

/// Abstract interface for the display (e-ink or TFT).
/// Lets the rest of the firmware drive the screen without knowing the technology.
class IDisplay {
public:
    virtual ~IDisplay() = default;

    /// Hardware init. Called once at boot.
    virtual void begin() = 0;

    /// Queue a status dashboard update (async if a display task is running).
    virtual void showStatus(uint8_t sensorCount,
                            const SensorConfig* configs,
                            const int* percentages,
                            const int* rawValues,
                            const char* wifiStatus) = 0;

    /// Show a full-screen calibration message.
    virtual void showCalibrationScreen(const char* step = "") = 0;

    /// Show a full-screen text message.
    virtual void showMessage(const char* msg) = 0;

    /// Put the display into low-power mode.
    virtual void deepSleep() = 0;

    /// Force a full refresh on the next update (for e-ink: clear ghosting).
    virtual void forceFullRefresh() = 0;

    /// Start a dedicated FreeRTOS task for display updates.
    virtual void startTask() = 0;

    /// Stop the display task.
    virtual void stopTask() = 0;

    /// True while a render is in progress (task busy).
    virtual bool isBusy() const = 0;

    /// True if the display task was started and is running.
    virtual bool isTaskRunning() const = 0;
};

#endif // IDISPLAY_H
