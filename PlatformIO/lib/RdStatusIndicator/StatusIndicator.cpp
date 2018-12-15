// StatusIndicator
// Rob Dobson 2018

#include "StatusIndicator.h"

static const char* MODULE_PREFIX = "StatusIndicator: ";

StatusIndicator::StatusIndicator()
{
    _isSetup = false;
    _hwPin = -1;
    _onLevel = 1;
    _pConfig = NULL;
}

void StatusIndicator::setup(ConfigBase* pConfig, const char* ledName)
{
    _name = ledName;
    // Save config and register callback on config changed
    if (_pConfig == NULL)
    {
        _pConfig = pConfig;
        _pConfig->registerChangeCallback(std::bind(&StatusIndicator::configChanged, this));
    }
    // Get LED config
    ConfigBase ledConfig(pConfig->getString(ledName, "").c_str());
    Log.trace("%ssetup name %s configStr %s\n", MODULE_PREFIX, _name.c_str(),
                    ledConfig.getConfigCStrPtr());
    String pinStr = ledConfig.getString("hwPin", "");
    int ledPin = -1;
    if (pinStr.length() != 0)
        ledPin = ConfigPinMap::getPinFromName(pinStr.c_str());
    _onLevel = ledConfig.getLong("onLevel", 1) ? 1 : 0;

    // Blink info
    _onMs = ledConfig.getLong("onMs", 500);
    _shortOffMs = ledConfig.getLong("shortOffMs", 500);
    _longOffMs = ledConfig.getLong("longOffMs", 1000);
    Log.notice("%s%s pin %d(%s) onLevel %d onMs %l shortMs %l longMs %l\n", MODULE_PREFIX,
                ledName, ledPin, pinStr.c_str(), _onLevel, _onMs, _shortOffMs, _longOffMs);
    if (ledPin == -1)
        return;
    // Check if this is a change
    if (_isSetup)
    {
        if (ledPin != _hwPin)
        {
            pinMode(_hwPin, INPUT);
        }
        else
        {
            // No change so nothing to do
            Log.trace("%s%sNo change\n", MODULE_PREFIX, _name.c_str());
            return;
        }
    }
    // Setup the pin
    _hwPin = ledPin;
    pinMode(_hwPin, OUTPUT);
    digitalWrite(_hwPin, !_onLevel);
    _isOn = false;
    _curCodePos = 0;
    _curCode = 0;
    _isSetup = true;
    _changeLastMs = millis();
}

void StatusIndicator::setCode(int code)
{
    // Log.verbose("%ssetCode %d curCode %d isSetup %d\n", MODULE_PREFIX, code, _curCode, _isSetup);
    if ((_curCode == code) || (!_isSetup))
        return;
    _curCode = code;
    _curCodePos = 0;
    _changeLastMs = millis();
    if (code == 0)
    {
        digitalWrite(_hwPin, !_onLevel);
        _isOn = false;
    }
    else
    {
        digitalWrite(_hwPin, _onLevel);
        _isOn = true;
    }
}

void StatusIndicator::service()
{
    // Check if active
    if (!_isSetup)
        return;

    // Code 0 means stay off
    if (_curCode == 0)
        return;

    // Handle the code
    if (_isOn)
    {
        if (Utils::isTimeout(millis(), _changeLastMs, _onMs))
        {
            _isOn = false;
            digitalWrite(_hwPin, !_onLevel);
            _changeLastMs = millis();
        }
    }
    else
    {
        if (Utils::isTimeout(millis(), _changeLastMs, 
                (_curCodePos == _curCode-1) ? _longOffMs : _shortOffMs))
        {
            _isOn = true;
            digitalWrite(_hwPin, _onLevel);
            _changeLastMs = millis();
            _curCodePos++;
            if (_curCodePos >= _curCode)
                _curCodePos = 0;
        }
    }
}

void StatusIndicator::configChanged()
{
    // Reset config
    Log.trace("%sconfigChanged\n", MODULE_PREFIX);
    setup(_pConfig, _name.c_str());
}
