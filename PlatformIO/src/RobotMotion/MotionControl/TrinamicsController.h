// RBotFirmware
// Rob Dobson 2016-19

#pragma once

#include <ArduinoLog.h>
#include <SPI.h>

class TrinamicsController
{
public:
    TrinamicsController();
    ~TrinamicsController();

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
    int getPinAndConfigure(const char* configJSON, const char* pinSelector, int direction, int initValue);
    uint64_t tmcWrite(int chipIdx, uint8_t cmd, uint32_t data);
    uint8_t tmcRead(int chipIdx, uint8_t cmd, uint32_t* data);
    void chipSel(int chipIdx, bool en);
    void performSel(int singleCS, int mux1, int mux2, int mux3, int muxCS, bool en);
    uint64_t tmc5072Init(int chipIdx);

    static TrinamicsController* _pThisObj;

    bool _isEnabled;
    // TimerHandle_t _trinamicsTimerHandle;
    esp_timer_handle_t _trinamicsTimerHandle;
    bool _trinamicsTimerStarted;
    int _miso;
    int _mosi;
    int _sck;

    // un-multiplexed chip selects
    int _cs1;
    int _cs2;
    int _cs3;

    // multiplexer pins and chip select mux values
    int _mux1;
    int _mux2;
    int _mux3;
    int _muxCS1;
    int _muxCS2;
    int _muxCS3;

    // SPI controller
    SPIClass* _pVSPI;

    static const int MAX_TMC2130 = 3;

    static constexpr uint32_t TRINAMIC_TIMER_PERIOD_MS = 1000;
    static const int SPI_CLOCK_HZ = 1000000;

    static const int WRITE_FLAG = 1 << 7;
    static const int READ_FLAG = 0;
    static const int REG_GCONF = 0x00;
    static const int REG_GSTAT = 0x01;
    static const int REG_IHOLD_IRUN = 0x10;
    static const int REG_CHOPCONF = 0x6C;
    static const int REG_COOLCONF = 0x6D;
    static const int REG_DCCTRL = 0x6E;
    static const int REG_DRVSTATUS = 0x6F;

    // Debug
    uint32_t _debugTimerLast;
};