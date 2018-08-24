// Debug Loop timer
// Used in a void loop() in Arduino / Particle to output a debug string
// periodically and indicate loop time
// Rob Dobson 2017

#pragma once

typedef void (*DebugLoopTimer_InfoStrCb)(String &infoStr);

class DebugLoopTimer
{
  private:
    // Timing of the loop - used to determine if blocking/slow processes are delaying the loop iteration
    unsigned long _loopTimeAvgSum;
    unsigned long _loopTimeAvgCount;
    unsigned long _loopTimeMax;
    unsigned long _loopTimeMin;
    unsigned long _lastLoopStartMicros;
    bool _lastLoopStartValid = false;
    unsigned long _lastDebugLoopTime;
    long _reportingPeriodMs;
    DebugLoopTimer_InfoStrCb _infoStrCallback;

    // Block Timing
    static const int _maxTimingBlocks = 15;
    unsigned long _blockStartTime[_maxTimingBlocks];
    unsigned long _blockMaxTime[_maxTimingBlocks];
    String _blockName[_maxTimingBlocks];

  public:
    DebugLoopTimer(long reportingPeriodMs, DebugLoopTimer_InfoStrCb infoStrCallback)
    {
        clearVals();
        _lastLoopStartMicros = 0;
        _lastDebugLoopTime = 0;
        _reportingPeriodMs = reportingPeriodMs;
        _infoStrCallback = infoStrCallback;
    }

    void service()
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
            char _avgStr[30];
            strcpy(_avgStr, "N/A");
            if (_loopTimeAvgCount > 0)
            {
                double avg = 0;
                avg = (1.0 * _loopTimeAvgSum) / _loopTimeAvgCount;
                dtostrf(avg, 4, 2, _avgStr);
            }

            // Find slowest loop activity
            char _slowest1Str[200];
            strcpy(_slowest1Str, "");
            int curSlowestIdx = 0;
            for (int i = 1; i < _maxTimingBlocks; i++)
            {
                if (_blockMaxTime[i] > _blockMaxTime[curSlowestIdx])
                    curSlowestIdx = i;
            }
            if (_blockMaxTime[curSlowestIdx] != 0)
            {
                sprintf(_slowest1Str, "Slowest %s %ld", _blockName[curSlowestIdx].c_str(), _blockMaxTime[curSlowestIdx]);
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
                sprintf(_slowest1Str + strlen(_slowest1Str), ", %s %ld", _blockName[cur2ndSlowestIdx].c_str(), _blockMaxTime[cur2ndSlowestIdx]);
            }

            String programInfoStr;
            _infoStrCallback(programInfoStr);

            char _millisStr[20];
            sprintf(_millisStr, "%05ld", millis());
            char _maxMinStr[30];
            sprintf(_maxMinStr, "Max %luuS Min %luuS", _loopTimeMax, _loopTimeMin);

            String totalStr = _millisStr + String(" ") + programInfoStr + String(" Avg ") + _avgStr + String("uS ") + _maxMinStr + String(" ") + _slowest1Str + String("\n");
            Log.notice(totalStr.c_str());
            _lastDebugLoopTime = millis();

            // Clear values for next reporting interval
            clearVals();
        }
    }

    void clearVals()
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

    void blockAdd(int blkIdx, const char *blockName)
    {
        if (blkIdx < _maxTimingBlocks)
            _blockName[blkIdx] = blockName;
    }
    void blockStart(int blkIdx)
    {
        if (blkIdx < _maxTimingBlocks)
            _blockStartTime[blkIdx] = micros();
    }
    void blockEnd(int blkIdx)
    {
        if (blkIdx < _maxTimingBlocks)
        {
            unsigned long durUs = micros() - _blockStartTime[blkIdx];
            if (_blockMaxTime[blkIdx] < durUs)
                _blockMaxTime[blkIdx] = durUs;
        }
    }
};
