// Status LED
// Rob Dobson 2018

#pragma once

#include "Arduino.h"
#include "Utils.h"
#include "ConfigBase.h"
#include "ConfigPinMap.h"

class StatusLed
{
  public:
    StatusLed()
    {
        _isSetup = false;
    }
    void setup(ConfigBase& hwConfig, const char* ledName)
    {
        ConfigBase ledConfig(hwConfig.getString(ledName, "").c_str());
        String pinStr = ledConfig.getString("ledPin", "");
        int ledPin = 0;
        if (pinStr.length() != 0)
            ledPin = ConfigPinMap::getPinFromName(pinStr.c_str());
        _ledOnMs = ledConfig.getLong("ledOnMs", 500);
        _ledShortOffMs = ledConfig.getLong("ledShortOffMs", 500);
        _ledLongOffMs = ledConfig.getLong("ledLongOffMs", 1000);
        Log.notice("StatusLed: Wifi pin %d on %l short %l long %l\n", ledPin, _ledOnMs, _ledShortOffMs, _ledLongOffMs);
        // Check if this is a change
        if (_isSetup)
        {
            if (ledPin != _ledPin)
            {
                pinMode(_ledPin, INPUT);
            }
            else
            {
                // No change so nothing to do
                Log.notice("StatusLed: No change\n");
                return;
            }
        }
        // Setup the pin
        _ledPin = ledPin;
        pinMode(_ledPin, OUTPUT);
        digitalWrite(_ledPin, 0);
        _ledIsOn = false;
        _curCodePos = 0;
        _ledCode = 0;
        _isSetup = true;
        _ledLastMs = millis();
    }

    void setCode(int code)
    {
        if (_ledCode == code)
            return;
        _ledCode = code;
        _curCodePos = 0;
        _ledLastMs = millis();
        digitalWrite(_ledPin, 0);
        _ledIsOn = false;
    }

    void service()
    {
        // Check if active
        if (!_isSetup)
            return;

        // Code 0 means stay off
        if (_ledCode == 0)
            return;

        // Handle the code
        if (_ledIsOn)
        {
            if (Utils::isTimeout(millis(), _ledLastMs, _ledOnMs))
            {
                _ledIsOn = false;
                digitalWrite(_ledPin, 0);
                _ledLastMs = millis();
            }
        }
        else
        {
            if (Utils::isTimeout(millis(), _ledLastMs, 
                    (_curCodePos == _ledCode-1) ? _ledLongOffMs : _ledShortOffMs))
            {
                _ledIsOn = true;
                digitalWrite(_ledPin, 1);
                _ledLastMs = millis();
                _curCodePos++;
                if (_curCodePos >= _ledCode)
                    _curCodePos = 0;
            }
        }
    }

  private:
    bool _isSetup;
    int _ledPin;
    int _ledCode;
    int _curCodePos;
    bool _ledIsOn;
    uint32_t _ledLastMs;
    uint32_t _ledOnMs;
    uint32_t _ledLongOffMs;
    uint32_t _ledShortOffMs;
};