// ESP32 Software Serial using Interrupts for Rx and Tx
// Rob Dobson 2018

// Note: parity is not supported

#pragma once

#include <Arduino.h>
#include "ArduinoLog.h"
#include "RingBufferPosn.h"
#include "limits.h"

class SwSerial : public Stream
{
public:
    SwSerial(int txPin, int rxPin, int txBufferSize = 500, int rxBufferSize = 500)
    {
        // Buffers
        _txBufferPosn.init(txBufferSize);
        _pTxBuffer = new uint8_t[txBufferSize];
        _rxBufferPosn.init(rxBufferSize);
        _pRxBuffer = new uint8_t[rxBufferSize];
        // Pins
        _swSerialTxPin = txPin;
        pinMode(txPin, OUTPUT);
        digitalWrite(txPin, HIGH);
        _swSerialRxPin = rxPin;
        pinMode(rxPin, INPUT);
    }

    ~SwSerial()
    {
        if (_pTxBuffer)
            delete[] _pTxBuffer;
        if (_pRxBuffer)
            delete[] _pRxBuffer;
    }

    void begin(long speed);
    long baudRate();
    virtual int read();
    virtual int peek();
    virtual void flush();
    virtual int available();
    virtual size_t write(uint8_t ch);
    uint32_t getBitDurationInCycles();

    using Print::write;

private:
    // Tx Timer
    hw_timer_t *_txTimer = NULL;

    // Rx & Tx Pins
    static int _swSerialTxPin;
    static int _swSerialRxPin;

    // Serial Config
    static int _nDataBits;
    static int _nStopBits;

    // Tx Buffer
    static uint8_t *_pTxBuffer;
    static RingBufferPosn _txBufferPosn;

    // Tx progress
    static uint8_t _txCurCh;
    static int _txCurBit;

    // Rx Buffer
    static uint8_t *_pRxBuffer;
    static RingBufferPosn _rxBufferPosn;

    // Rx progress
    static uint8_t _rxCurCh;
    static int _rxCurBit;

    // Time in machine cycles since last edge
    static unsigned long _rxLastEdgeCycleCount;

    // Baudrate
    int _baudRate;

    // Time of a bit duration (and half bit duration) in cycles
    static unsigned long _rxBitTimeInCycles;
    static unsigned long _rxHalfBitTimeInCycles;

    // ISR for tx
    static void IRAM_ATTR onBitTimer();

    // ISR for Rx (change of pin value)
    static void IRAM_ATTR onRxChange();
};
