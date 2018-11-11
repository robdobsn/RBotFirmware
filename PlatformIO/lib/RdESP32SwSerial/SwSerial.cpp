// ESP32 Software Serial using Interrupts for Rx and Tx
// Rob Dobson 2018

#include "SwSerial.h"

void SwSerial::begin(long speed)
{
    _baudRate = speed;
    // Tx interrupt
    int txTimerCount = 40000000 / speed;
    _txTimer = timerBegin(0, 2, true);
    timerAttachInterrupt(_txTimer, &onBitTimer, true);
    timerAlarmWrite(_txTimer, txTimerCount, true);
    timerAlarmEnable(_txTimer);
    // Rx interrupt
    _rxBitTimeInCycles = ESP.getCpuFreqMHz() * 1000000l / speed;
    _rxHalfBitTimeInCycles = _rxBitTimeInCycles / 2;
    attachInterrupt(digitalPinToInterrupt(_swSerialRxPin), onRxChange, CHANGE);
}

long SwSerial::baudRate()
{
    return _baudRate;
}

int SwSerial::read()
{
    if (!_rxBufferPosn.canGet())
        return -1;
    int ch = _pRxBuffer[_rxBufferPosn._getPos];
    _rxBufferPosn.hasGot();
    return ch;
}

int SwSerial::peek()
{
    if (!_rxBufferPosn.canGet())
        return -1;
    return _pRxBuffer[_rxBufferPosn._getPos];
}

void SwSerial::flush()
{
    _rxBufferPosn.clear();
}

int SwSerial::available()
{
    return _rxBufferPosn.count();
}

size_t SwSerial::write(uint8_t ch)
{
    if (!_txBufferPosn.canPut())
        return 0;
    _pTxBuffer[_txBufferPosn._putPos] = ch;
    _txBufferPosn.hasPut();
    return 1;
}

uint32_t SwSerial::getBitDurationInCycles()
{
    return _rxBitTimeInCycles;
}

// Hardware pins
int SwSerial::_swSerialTxPin = -1;
int SwSerial::_swSerialRxPin = -1;

// Buffers
uint8_t* SwSerial::_pTxBuffer = NULL;
RingBufferPosn SwSerial::_txBufferPosn(0);
uint8_t* SwSerial::_pRxBuffer = NULL;
RingBufferPosn SwSerial::_rxBufferPosn(0);

// Current character tx and rx
int SwSerial::_txCurBit = 0;
uint8_t SwSerial::_txCurCh = 0;
int SwSerial::_rxCurBit = 0;
uint8_t SwSerial::_rxCurCh = 0;

// Time in machine cycles since last edge
unsigned long SwSerial::_rxLastEdgeCycleCount = 0;

// Time of a bit (and half bit) duration in cycles - set elsewhere
unsigned long SwSerial::_rxBitTimeInCycles = 1;
unsigned long SwSerial::_rxHalfBitTimeInCycles = 1;

// Serial properties
int SwSerial::_nDataBits = 8;
int SwSerial::_nStopBits = 1;

// ISR for tx
void IRAM_ATTR SwSerial::onBitTimer()
{
    // See if already transmitting
    if (_txCurBit > 0)
    {
        // Send bit
        if (_txCurBit <= _nDataBits)
        {
            int nxtBitVal = _txCurCh >> (_txCurBit - 1);
            digitalWrite(_swSerialTxPin, (nxtBitVal & 0x01) != 0);
        }
        else
        {
            // Stop bits
            digitalWrite(_swSerialTxPin, 1);
        }
        // Next bit
        _txCurBit++;
        // Check if done
        if (_txCurBit >= 1 + _nDataBits + _nStopBits)
        {
            _txCurBit = 0;
        }
    }
    else
    {
        // Check for new data to send
        if (_txBufferPosn.canGet())
        {
            // Get char to send next
            _txCurCh = _pTxBuffer[_txBufferPosn._getPos];
            _txBufferPosn.hasGot();

            // Send start bit
            digitalWrite(_swSerialTxPin, 0);
            _txCurBit = 1;
        }
    }

    // Check for no rx edge for several bit periods
    if (_rxCurBit > 0)
    {
        unsigned long curCycles = ESP.getCycleCount();
        // Time since last change
        unsigned long sinceLast = curCycles - _rxLastEdgeCycleCount;
        if (curCycles < _rxLastEdgeCycleCount)
            sinceLast = ULONG_MAX - _rxLastEdgeCycleCount + curCycles;
        // Check timeout
        int bitCount = (sinceLast + _rxHalfBitTimeInCycles) / _rxBitTimeInCycles;
        // If there hasn't been an edge for 2.5 times the max bits in a transmission
        // Then the stop bit didn't involve a transition and there hasn't been
        // another transmission started yet (to create a start bit edge)
        // 2.5 times is chosen so there isn't too much delay getting the received
        // data and to reduce the chance of a race condition where another byte
        // might start to be received while we're in this function
        if (bitCount > (_nDataBits * 5) / 2)
        {
            // Receive complete - fill the remaining spaces with 1s
            for (int i = 0; i < _nDataBits + 2 - _rxCurBit; i++)
                _rxCurCh = (_rxCurCh >> 1) + 0x80;

            // Add to received data only if there still hasn't been a start bit
            // received
            if (_rxCurBit > 0)
            {
                if (_rxBufferPosn.canPut())
                {
                    _pRxBuffer[_rxBufferPosn._putPos] = _rxCurCh;
                    _rxBufferPosn.hasPut();
                }
                // Reset rx
                _rxCurCh = 0;
                _rxCurBit = 0;
            }
        }
    }
}

// ISR for Rx (change of pin value)
void IRAM_ATTR SwSerial::onRxChange()
{
    unsigned long curCycles = ESP.getCycleCount();
    // Time since last change
    unsigned long sinceLast = curCycles - _rxLastEdgeCycleCount;
    if (curCycles < _rxLastEdgeCycleCount)
        sinceLast = ULONG_MAX - _rxLastEdgeCycleCount + curCycles;
    int curLev = digitalRead(_swSerialRxPin);
    _rxLastEdgeCycleCount = ESP.getCycleCount();

    // Check the status of the edge
    if (_rxCurBit == 0)
    {
        if (curLev == 0)
        {
            // Start bit (no rx in progress)
            _rxCurCh = 0;
            _rxCurBit = 1;
        }
    }
    else
    {
        // Add the right number of bit of the opposite of the current value to the buffer
        int bitCount = (sinceLast + _rxHalfBitTimeInCycles) / _rxBitTimeInCycles;
        // // Check it is in a reasonable range
        if (bitCount > _nDataBits + 2 - _rxCurBit)
            bitCount = _nDataBits + 2 - _rxCurBit;
        // Move the bit position on
        _rxCurBit += bitCount;
        // Add the bits to the value - note that the start bit is initially shifted into
        // the byte and then this drops off the end
        while (bitCount > 0)
        {
            _rxCurCh = (_rxCurCh >> 1) + (curLev ? 0 : 0x80);
            bitCount--;
        }
        // Check if byte complete
        if (_rxCurBit > _nDataBits + 1)
        {
            // Add char to buffer
            if (_rxBufferPosn.canPut())
            {
                _pRxBuffer[_rxBufferPosn._putPos] = _rxCurCh;
                _rxBufferPosn.hasPut();
            }
            _rxCurCh = 0;
            _rxCurBit = 0;
            // Check for start bit
            if (curLev == 0)
            {
                // Start bit
                _rxCurCh = 0;
                _rxCurBit = 1;
            }
        }
    }
}
