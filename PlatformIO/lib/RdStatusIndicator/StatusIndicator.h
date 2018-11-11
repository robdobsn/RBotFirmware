// StatusIndicator
// Rob Dobson 2018

#pragma once

#include <Arduino.h>
#include "Utils.h"
#include "ConfigBase.h"
#include "ConfigPinMap.h"

class StatusIndicator
{
public:
    StatusIndicator()
    {
        _isSetup = false;
        _hwPin = -1;
        _onLevel = 1;
    }
    void setup(ConfigBase& hwConfig, const char* ledName);
    void setCode(int code);
    void service();

private:
    String _name;
    bool _isSetup;
    int _hwPin;
    int _onLevel;
    int _curCode;
    int _curCodePos;
    bool _isOn;
    uint32_t _changeLastMs;
    uint32_t _onMs;
    uint32_t _longOffMs;
    uint32_t _shortOffMs;
};