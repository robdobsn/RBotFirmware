// Debug Loop timer
// Used in a void loop() in Arduino / Particle to output a debug string
// periodically and indicate loop time
// Rob Dobson 2017

#include "ArduinoLog.h"
#include "DebugLoopTimer.h"

void DebugLoopTimer::service()
{
    // Monitor how long it takes to go around loop
    if (_lastLoopStartValid)
    {
        unsigned long loopTime = micros() - _lastLoopStartMicros;
        _loopTimeAvgSum += loopTime;
        _loopTimeAvgCount++;
        if (_loopTimeMin > loopTime)
            _loopTimeMin = loopTime;
        if (_loopTimeMax < loopTime)
            _loopTimeMax = loopTime;
    }
    _lastLoopStartMicros = micros();
    _lastLoopStartValid = true;

    // Every reporting period get info and display
    if (millis() > _lastDebugLoopTime + _reportingPeriodMs)
    {
        // Find average loop time
        char averageStr[30];
        strcpy(averageStr, "N/A");
        if (_loopTimeAvgCount > 0)
        {
            double avg = 0;
            avg = (1.0 * _loopTimeAvgSum) / _loopTimeAvgCount;
            dtostrf(avg, 4, 2, averageStr);
        }

        // Find slowest loop activity
        char slowest1Str[200];
        strcpy(slowest1Str, "");
        int curSlowestIdx = 0;
        for (int i = 1; i < _maxTimingBlocks; i++)
        {
            if (_blockMaxTime[i] > _blockMaxTime[curSlowestIdx])
                curSlowestIdx = i;
        }
        if (_blockMaxTime[curSlowestIdx] != 0)
        {
            sprintf(slowest1Str, "Slowest %s %ld", _blockName[curSlowestIdx].c_str(), _blockMaxTime[curSlowestIdx]);
        }

        // Second slowest
        int cur2ndSlowestIdx = 0;
        for (int i = 1; i < _maxTimingBlocks; i++)
        {
            if ((i != curSlowestIdx) && (_blockMaxTime[i] > _blockMaxTime[cur2ndSlowestIdx]))
                cur2ndSlowestIdx = i;
        }
        if (cur2ndSlowestIdx != curSlowestIdx && _blockMaxTime[cur2ndSlowestIdx] != 0)
        {
            sprintf(slowest1Str + strlen(slowest1Str), ", %s %ld", _blockName[cur2ndSlowestIdx].c_str(), _blockMaxTime[cur2ndSlowestIdx]);
        }

        String programInfoStr;
        _infoStrCallback(programInfoStr);

        char millisStr[20];
        sprintf(millisStr, "%05ld", millis());
        char maxMinStr[50];
        sprintf(maxMinStr, "Max %luuS Min %luuS", _loopTimeMax, _loopTimeMin);

        String totalStr = millisStr + String(" ") + programInfoStr +
                 String(" Avg ") + averageStr + String("uS ") + 
                 maxMinStr + String(" ") + slowest1Str + String("\n");
        Log.notice(totalStr.c_str());
        _lastDebugLoopTime = millis();

        // Clear values for next reporting interval
        clearVals();
    }
}

void DebugLoopTimer::clearVals()
{
    _loopTimeAvgSum = 0;
    _loopTimeAvgCount = 0;
    _loopTimeMin = 100000000;
    _loopTimeMax = 0;
    for (int i = 0; i < _maxTimingBlocks; i++)
    {
        _blockMaxTime[i] = 0;
    }
}

void DebugLoopTimer::blockAdd(int blkIdx, const char *blockName)
{
    if (blkIdx < _maxTimingBlocks)
        _blockName[blkIdx] = blockName;
}
void DebugLoopTimer::blockStart(int blkIdx)
{
    if (blkIdx < _maxTimingBlocks)
        _blockStartTime[blkIdx] = micros();
}
void DebugLoopTimer::blockEnd(int blkIdx)
{
    if (blkIdx < _maxTimingBlocks)
    {
        unsigned long durUs = micros() - _blockStartTime[blkIdx];
        if (_blockMaxTime[blkIdx] < durUs)
            _blockMaxTime[blkIdx] = durUs;
    }
}
