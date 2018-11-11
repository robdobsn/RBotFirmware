#pragma once

// Generic interrupt-safe ring buffer pointer class
// Each pointer is only updated by one source (ISR or main thread)
class MotionRingBufferPosn
{
  public:
    volatile unsigned int _putPos;
    volatile unsigned int _getPos;
    unsigned int _bufLen;

    MotionRingBufferPosn(int maxLen)
    {
        init(maxLen);
    }

    void init(int maxLen)
    {
        _bufLen = maxLen;
        _putPos = 0;
        _getPos = 0;
    }

    void clear()
    {
        _getPos = _putPos = 0;
    }
    bool canPut()
    {
        if (_bufLen == 0)
            return false;
        if (_putPos == _getPos)
            return true;
        unsigned int gp = _getPos;
        if (_putPos > gp)
        {
            if ((_putPos != _bufLen - 1) || (gp != 0))
                return true;
        }
        else
        {
            if (gp - _putPos > 1)
                return true;
        }
        return false;
    }

    bool IRAM_ATTR canGet()
    {
        return _putPos != _getPos;
    }

    void hasPut()
    {
        _putPos++;
        if (_putPos >= _bufLen)
            _putPos = 0;
    }

    void IRAM_ATTR hasGot()
    {
        _getPos++;
        if (_getPos >= _bufLen)
            _getPos = 0;
    }

    unsigned int count()
    {
        unsigned int getPos = _getPos;
        if (getPos <= _putPos)
            return _putPos - getPos;
        return _bufLen - getPos + _putPos;
    }

    // Get Nth element prior to the put position
    // 0 is the last element put in the queue
    // 1 is the one put in before that
    // Returns -1 if invalid
    int getNthFromPut(unsigned int N)
    {
        if (!canGet())
            return -1;
        if (N >= _bufLen)
            return -1;
        int nthPos = _putPos - 1 - N;
        if (nthPos < 0)
            nthPos += _bufLen;
        if (((unsigned int)(nthPos + 1) == _getPos) || ((unsigned int)(nthPos + 1) == _bufLen && _getPos == 0))
            return -1;
        return nthPos;
    }

    // Get Nth element from the get position
    // 0 is the element next got from the queue
    // 1 is the one got after that
    // returns -1 if invalid
    int getNthFromGet(unsigned int N)
    {
        if (!canGet())
            return -1;
        if (N >= _bufLen)
            return -1;
        unsigned int nthPos = _getPos + N;
        if (nthPos >= _bufLen)
            nthPos -= _bufLen;
        if (nthPos == _putPos)
            return -1;
        return nthPos;
    }
};
