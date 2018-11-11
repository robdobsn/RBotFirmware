// DebounceButton
// Rob Dobson 2018

// Callback on button press via a hardware pin

#include "DebounceButton.h"
#include "utils.h"
#include "ArduinoLog.h"

// Setup
void DebounceButton::setup(int pin, int activeLevel, DebounceButtonCallback cb)
{
    _debounceLastMs = millis();
    _debounceVal = 0;
    _buttonPin = pin;
    _buttonActiveLevel = activeLevel;
    _callback = cb;
    if (_buttonPin >= 0)
    {
        pinMode(_buttonPin, INPUT);
        _debounceVal = digitalRead(_buttonPin);
    }
}

void DebounceButton::service()
{
    if (_buttonPin < 0)
        return;
    // Check for change of value
    if (digitalRead(_buttonPin) != _debounceVal)
    {
        if (Utils::isTimeout(millis(), _debounceLastMs, PIN_DEBOUNCE_MS))
        {
            _debounceVal = !_debounceVal;
            if (_callback)
                _callback(_debounceVal == _buttonActiveLevel);
            _debounceLastMs = millis();
        }
    }
    else
    {
        _debounceLastMs = millis();
    }
}
