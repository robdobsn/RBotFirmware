// Debug Loop timer
// Used in a void loop() in Arduino / Particle to output a debug string
// periodically and indicate loop time
// Rob Dobson 2017

#pragma once

#include <Arduino.h>

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

    // Call frequently
    void service();

    // Clear all accumulated values
    void clearVals();

    // Add a timing block
    void blockAdd(int blkIdx, const char *blockName);

    // Enter a timing block
    void blockStart(int blkIdx);

    // Exit a timing block;
    void blockEnd(int blkIdx);
};
