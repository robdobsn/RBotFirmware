// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "limits.h"

class Utils
{
public:
    static bool isTimeout(unsigned long curTime, unsigned long lastTime, unsigned long maxDuration)
    {
        if (curTime >= lastTime)
        {
            return (curTime > lastTime + maxDuration);
        }
        return (ULONG_MAX - (lastTime-curTime) > maxDuration);
    }
    static int timeToTimeout(unsigned long curTime, unsigned long lastTime, unsigned long maxDuration)
    {
        if (curTime >= lastTime)
        {
            if (curTime > lastTime + maxDuration)
            return 0;
            return maxDuration - (curTime - lastTime);
        }
        if (ULONG_MAX - (lastTime-curTime) > maxDuration)
        return 0;
        return maxDuration - (ULONG_MAX - (lastTime-curTime));
    }

};
