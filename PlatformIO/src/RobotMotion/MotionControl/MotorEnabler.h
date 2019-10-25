
#include "Utils.h"

class MotorEnabler
{
public:
    static constexpr float stepDisableSecs_default = 60.0f;

    MotorEnabler()
    {
        // Stepper motor enablement
        _stepEnablePin = -1;
        _stepEnLev = true;
        _stepDisableSecs = 60.0;
        _motorEnLastMillis = 0;
        _motorEnLastUnixTime = 0;
    }
    ~MotorEnabler()
    {
        deinit();
    }
    void deinit()
    {
        // disable
        if (_stepEnablePin != -1)
            pinMode(_stepEnablePin, INPUT);
    }

    bool configure(const char *robotConfigJSON)
    {
        // Get motor enable info
        String stepEnablePinName = RdJson::getString("stepEnablePin", "-1", robotConfigJSON);
        _stepEnLev = RdJson::getLong("stepEnLev", 1, robotConfigJSON);
        _stepEnablePin = ConfigPinMap::getPinFromName(stepEnablePinName.c_str());
        _stepDisableSecs = float(RdJson::getDouble("stepDisableSecs", stepDisableSecs_default, robotConfigJSON));
        Log.notice("MotorEnabler: (pin %d, actLvl %d, disableAfter %Fs)\n", _stepEnablePin, _stepEnLev, _stepDisableSecs);

        // Enable pin - initially disable
        pinMode(_stepEnablePin, OUTPUT);
        digitalWrite(_stepEnablePin, !_stepEnLev);
        return true;
    }

    void enableMotors(bool en, bool timeout)
    {
        // Log.trace("Enable %d, disable level %d, disable after time %F\n",
        //							en, !_stepEnLev, _stepDisableSecs);
        if (en)
        {
            if (_stepEnablePin != -1)
            {
                if (!_motorsAreEnabled)
                    Log.notice("MotorEnabler: enabled, disable after idle %Fs\n", _stepDisableSecs);
                digitalWrite(_stepEnablePin, _stepEnLev);
            }
            _motorsAreEnabled = true;
            _motorEnLastMillis = millis();
            time(&_motorEnLastUnixTime);
        }
        else
        {
            if (_stepEnablePin != -1)
            {
                if (_motorsAreEnabled)
                    Log.notice("MotorEnabler: %smotors disabled by %s\n", timeout ? "timeout" : "command");
                digitalWrite(_stepEnablePin, !_stepEnLev);
            }
            _motorsAreEnabled = false;
        }
    }

    unsigned long getLastActiveUnixTime()
    {
        return _motorEnLastUnixTime;
    }

    void service()
    {
        // Check for motor enable timeout
        if (_motorsAreEnabled && Utils::isTimeout(millis(), _motorEnLastMillis,
                                                    (unsigned long)(_stepDisableSecs * 1000)))
            enableMotors(false, true);
    }

private:
    // Step enable
    int _stepEnablePin;
    bool _stepEnLev = true;
    // Motor enable
    float _stepDisableSecs;
    bool _motorsAreEnabled;
    unsigned long _motorEnLastMillis;
    time_t _motorEnLastUnixTime;

};
