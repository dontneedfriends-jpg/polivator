#ifndef IANALOG_READER_H
#define IANALOG_READER_H

#include <Arduino.h>

class IAnalogReader {
public:
    virtual ~IAnalogReader() = default;
    virtual int readRaw(uint8_t pin) = 0;
};

#endif
