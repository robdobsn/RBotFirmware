// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "RdJson.h"
#include "ConfigPinMap.h"

class EndStop
{
  private:
    int _pin;
    bool _actLvl;
    int _inputType;

  public:
    EndStop(int axisIdx, int endStopIdx, const char *endStopJSON)
    {
        // Get end stop
        String pinName = RdJson::getString("sensePin", "-1", endStopJSON);
        long actLvl = RdJson::getLong("actLvl", 1, endStopJSON);
        String inputTypeStr = RdJson::getString("inputType", "", endStopJSON);
        long pinId = ConfigPinMap::getPinFromName(pinName.c_str());
        int inputType = ConfigPinMap::getInputType(inputTypeStr.c_str());
        Log.notice("Axis%dEndStop%d (sense %d, level %d, type %d)\n", axisIdx, endStopIdx, pinId,
                   actLvl, inputType);
        setConfig(pinId, actLvl, inputType);
    }
    ~EndStop()
    {
        // Restore pin to input (may have had pullup)
        if (_pin != -1)
            pinMode(_pin, INPUT);
    }
    void setConfig(int pin, bool actLvl, int inputType)
    {
        _pin = pin;
        _actLvl = actLvl;
        _inputType = inputType;
        if (_pin != -1)
        {
            pinMode(_pin, _inputType);
        }
        // Log.notice("EndStop %d, %d, %d\n", _pin, _actLvl, _inputType);
    }
    bool isAtEndStop()
    {
        if (_pin != -1)
        {
            bool val = digitalRead(_pin);
            return val == _actLvl;
        }
        return true;
    }
    void getPins(int &sensePin, bool &actLvl)
    {
        sensePin = _pin;
        actLvl = _actLvl;
    }
};
