// RBotFirmware
// Rob Dobson 2016-19

#pragma once

#include <ArduinoLog.h>

class TMC5072Controller
{
public:
    TMC5072Controller();
    ~TMC5072Controller();

    void configure(const char *configJSON);
    void deinit();
    void process();

    bool isEnabled()
    {
        return _isEnabled;
    }

    void _timerCallback(void* arg);

    static void _staticTimerCb(void* arg)
    {
        if (_pThisObj)
            _pThisObj->_timerCallback(arg);
    } 

private:
    static TMC5072Controller* _pThisObj;

    bool _isEnabled;
    // TimerHandle_t _trinamicsTimerHandle;
    esp_timer_handle_t _trinamicsTimerHandle;
    bool _trinamicsTimerStarted;
    int _miso;
    int _mosi;
    int _sck;
    int _cs1;
    int _cs2;

    static constexpr uint32_t TRINAMIC_TIMER_PERIOD_MS = 1000;
};