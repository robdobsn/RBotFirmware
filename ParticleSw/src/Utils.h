// RBotFirmware
// Rob Dobson 2016-2017

#pragma once

#include "application.h"
#include "limits.h"

class Utils
{
public:
  static bool isTimeout(unsigned long curTime, unsigned long lastTime, unsigned long maxDuration)
  {
    if (curTime >= lastTime)
    {
      return(curTime > lastTime + maxDuration);
    }
    return(ULONG_MAX - (lastTime - curTime) > maxDuration);
  }
  static int timeToTimeout(unsigned long curTime, unsigned long lastTime, unsigned long maxDuration)
  {
    if (curTime >= lastTime)
    {
      if (curTime > lastTime + maxDuration)
        return 0;
      return maxDuration - (curTime - lastTime);
    }
    if (ULONG_MAX - (lastTime - curTime) > maxDuration)
      return 0;
    return maxDuration - (ULONG_MAX - (lastTime - curTime));
  }
  static void logLongStr(const char* headerMsg, const char* toLog, bool infoLevel = false)
  {
    if (infoLevel)
      Log.info(headerMsg);
    else
      Log.trace(headerMsg);
    const int linLen = 80;
    for (unsigned int i = 0; i < strlen(toLog); i += linLen)
    {
      char pBuf[linLen + 1];
      strncpy(pBuf, toLog + i, linLen);
      pBuf[linLen] = 0;
      if (infoLevel)
        Log.info(pBuf);
      else
        Log.trace(pBuf);
    }
  }
};
