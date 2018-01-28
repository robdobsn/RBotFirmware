// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "RdJson.h"
#include "ConfigPinMap.h"

class EndStop
{
private:
  int _pin;
  bool _activeLevel;
  int _inputType;

public:
  EndStop(int axisIdx, int endStopIdx, const char* endStopJSON)
  {
    // Get end stop
    String pinName      = RdJson::getString("sensePin", "-1", endStopJSON);
    long   activeLevel  = RdJson::getLong("activeLevel", 1, endStopJSON);
    String inputTypeStr = RdJson::getString("inputType", "", endStopJSON);
    long   pinId        = ConfigPinMap::getPinFromName(pinName.c_str());
    int    inputType    = ConfigPinMap::getInputType(inputTypeStr.c_str());
    Log.info("Axis%dEndStop%d (sense %ld, level %ld, type %d)", axisIdx, endStopIdx, pinId,
             activeLevel, inputType);
    setConfig(pinId, activeLevel, inputType);
  }
  ~EndStop()
  {
    // Restore pin to input (may have had pullup)
    if (_pin != -1)
      pinMode(_pin, INPUT);
  }
  void setConfig(int pin, bool activeLevel, int inputType)
  {
    _pin         = pin;
    _activeLevel = activeLevel;
    _inputType   = inputType;
    if (_pin != -1)
    {
      pinMode(_pin, (PinMode) _inputType);
    }
    // Log.info("EndStop %d, %d, %d", _pin, _activeLevel, _inputType);
  }
  bool isAtEndStop()
  {
    if (_pin != -1)
    {
      bool val = digitalRead(_pin);
      return val == _activeLevel;
    }
    return true;
  }
  void getPins(int& sensePin, bool& activeLevel)
  {
    sensePin    = _pin;
    activeLevel = _activeLevel;
  }
};
