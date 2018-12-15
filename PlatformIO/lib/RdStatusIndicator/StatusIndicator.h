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
    StatusIndicator();
    void setup(ConfigBase* pConfig, const char* ledName);
    void setCode(int code);
    void service();

private:
    void configChanged();

private:
    ConfigBase* _pConfig;
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