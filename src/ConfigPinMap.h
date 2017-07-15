// RBotFirmware
// Rob Dobson 2016

#pragma once

class ConfigPinMap
{
public:
    static int _pinMapD[];
    static int _pinMapDLen;
    static int _pinMapA[];
    static int _pinMapALen;

    static int getPinFromName(const char* pinName)
    {
        switch(*pinName)
        {
            case 'D':
            {
                int pinIdx = (int)strtol(pinName+1, NULL, 10);
                if (pinIdx >= 0 && pinIdx < _pinMapDLen)
                    return _pinMapD[pinIdx];
                return -1;
            }
            case 'A':
            {
                int pinIdx = (int)strtol(pinName+1, NULL, 10);
                if (pinIdx >= 0 && pinIdx < _pinMapALen)
                    return _pinMapA[pinIdx];
                return -1;
            }
        }
        return (int)strtol(pinName, NULL, 10);
    }

    // Conversion from strings like:
    // PULLUP, PULLDOWN to INPUT_PULLUP, etc
    static int getInputType(const char* inputTypeStr)
    {
        if (strcasecmp(inputTypeStr, "INPUT_PULLUP") == 0)
            return INPUT_PULLUP;
        if (strcasecmp(inputTypeStr, "INPUT_PULLDOWN") == 0)
            return INPUT_PULLDOWN;
        return INPUT;
    }
};
