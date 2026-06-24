#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

#define MAX_SENSORS 5

struct SensorConfig {
    char name[32];
    uint8_t pin;
    int dryRaw;
    int wetRaw;
    bool enabled;
};

#endif // TYPES_H
