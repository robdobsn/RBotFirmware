// Config Pin Map (maps as string to a pin on a processor)
// Rob Dobson 2016-2018

#pragma once

#include <Arduino.h>

class ConfigPinMap
{
public:
    static const char *_pinMapOtherStr[];
    static int _pinMapOtherPin[];
    static int _pinMapOtherLen;
    static int _pinMapD[];
    static int _pinMapDLen;
    static int _pinMapA[];
    static int _pinMapALen;

    static int getPinFromName(const char *pinName);

    // Conversion from strings like:
    // PULLUP, PULLDOWN to INPUT_PULLUP, etc
    static int getInputType(const char *inputTypeStr);
};
