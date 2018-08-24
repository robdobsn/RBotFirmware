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

    static int getPinFromName(const char *pinName)
    {
        // Check "other" for full match to pin name
        for (int i = 0; i < _pinMapOtherLen; i++)
        {
            if (strcmp(_pinMapOtherStr[i], pinName) == 0)
                return _pinMapOtherPin[i];
        }
        // Check for D# and A# pins
        switch (*pinName)
        {
        case 'D':
        {
            int pinIdx = (int)strtol(pinName + 1, NULL, 10);
            if (pinIdx >= 0 && pinIdx < _pinMapDLen)
                return _pinMapD[pinIdx];
            return -1;
        }
        case 'A':
        {
            int pinIdx = (int)strtol(pinName + 1, NULL, 10);
            if (pinIdx >= 0 && pinIdx < _pinMapALen)
                return _pinMapA[pinIdx];
            return -1;
        }
        }
        if (strlen(pinName) == 0)
            return -1;
        return (int)strtol(pinName, NULL, 10);
    }

    // Conversion from strings like:
    // PULLUP, PULLDOWN to INPUT_PULLUP, etc
    static int getInputType(const char *inputTypeStr)
    {
        if (strcasecmp(inputTypeStr, "INPUT_PULLUP") == 0)
            return INPUT_PULLUP;
        if (strcasecmp(inputTypeStr, "INPUT_PULLDOWN") == 0)
            return INPUT_PULLDOWN;
        return INPUT;
    }
};
