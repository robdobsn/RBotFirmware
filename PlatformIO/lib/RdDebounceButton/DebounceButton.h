// DebounceButton
// Rob Dobson 2018

// Callback on button press via a hardware pin

#pragma once

#include <Arduino.h>
#include <functional>

typedef std::function<void(int val)> DebounceButtonCallback;

class DebounceButton
{
private:
    // Button
    int _buttonPin;
    int _buttonActiveLevel;
    uint32_t _debounceLastMs;
    int _debounceVal;
    bool _firstPass;
    static const int PIN_DEBOUNCE_MS = 50;

    // Callback
    DebounceButtonCallback _callback;

public:
    DebounceButton();
    // Setup
    void setup(int pin, int activeLevel, DebounceButtonCallback cb);
    // Service - must be called frequently to check button state
    void service();
};
