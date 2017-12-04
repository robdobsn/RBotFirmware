// RBotFirmware
// Rob Dobson 2017
// Handles Interrupt Service Routine for stepper motors

#pragma once

#include "SparkIntervalTimer.h"

#define DEBUG_TIME_ISR_OVERALL 1
#define DEBUG_STEPS_FROM_ZERO 1
// #define DEBUG_TIME_ISR_JITTER 1

// Max axes and number of pipeline steps that can be accommodated
static const int ISR_MAX_AXES = 3;
static const int ISR_MAX_STEP_GROUPS = 50;

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
        _bufLen = maxLen;
    }

    bool canPut()
    {
        if (_putPos == _getPos)
            return true;
        if (_putPos > _getPos)
            if ((_putPos != _bufLen-1) || (_getPos != 0))
                return true;
        else
            if (_getPos - _putPos > 1)
                return true;
        return false;
    }

    bool canGet()
    {
        return _putPos != _getPos;
    }

    void hasPut()
    {
        _putPos++;
        if (_putPos >= _bufLen)
            _putPos = 0;
    }

    void hasGot()
    {
        _getPos++;
        if (_getPos >= _bufLen)
            _getPos = 0;
    }
};

// Class to handle motion variables for an axis
class ISRAxisMotionVars
{
public:
    // Pipeline elements (uS per step, number of steps and direction)
    uint32_t _stepUs[ISR_MAX_STEP_GROUPS];
    uint32_t _stepNum[ISR_MAX_STEP_GROUPS];
    uint32_t _stepDirn[ISR_MAX_STEP_GROUPS];
    // Accumulator for time elapsed between calls to ISR
    uint32_t _usAccum;
    // Count of steps performed in current pipeline element
    uint32_t _stepCount;
    // True if this axis is actively moving
    bool _isActive;
    // Pins and current values for step and direction
    int _stepPin;
    bool _stepPinValue;
    int _dirnPin;
    bool _dirnPinValue;
    bool _dirnPinSetForThisBlock;
#ifdef DEBUG_TIME_ISR_JITTER
    // Debug for jitter timing
    bool _dbgLastStepUsValid;
    uint32_t _dbgLastStepUs;
    uint32_t _dbgMaxStepUs;
    uint32_t _dbgMinStepUs;
#endif
#ifdef DEBUG_STEPS_FROM_ZERO
    volatile long _dbgStepsFromLastZero;
#endif

    ISRAxisMotionVars()
    {
        _usAccum = 0;
        _stepCount = 0;
        _isActive = false;
        _stepPin = -1;
        _stepPinValue = false;
        _dirnPin = -1;
        _dirnPinValue = false;
        _dirnPinSetForThisBlock = false;
#ifdef DEBUG_TIME_ISR_JITTER
        _dbgStepsFromLastZero = 0;
#endif
        dbgResetMinMax();
    }

    void dbgResetMinMax()
    {
#ifdef DEBUG_TIME_ISR_JITTER
        _dbgLastStepUsValid = false;
        _dbgMaxStepUs = 0;
        _dbgMinStepUs = 10000000;
#endif
    }

    void dbgResetZero()
    {
#ifdef DEBUG_STEPS_FROM_ZERO
        _dbgStepsFromLastZero = 0;
#endif
    }

    void setPins(int stepPin, int dirnPin)
    {
        _stepPin = stepPin;
        _dirnPin = dirnPin;
        digitalWrite(_stepPin, false);
        _stepPinValue = false;
        digitalWrite(_dirnPin, false);
        _dirnPinValue = false;
    }
};

// Interval timer
IntervalTimer __isrMotionTimer;
bool __isrIsEnabled = false;
#ifdef DEBUG_TIME_ISR_OVERALL
uint32_t __isrDbgTickMin = 100000000;
uint32_t __isrDbgTickMax = 0;
#endif

// Ring buffer to handle queue
MotionRingBufferPosn __isrRingBufferPosn(ISR_MAX_STEP_GROUPS);

// Axis variables
ISRAxisMotionVars __isrAxisVars[ISR_MAX_AXES];

// The actual interrupt service routine
void __isrStepperMotion(void)
{
#ifdef DEBUG_TIME_ISR_OVERALL
    uint32_t startTicks = System.ticks();
#endif
    // Check enabled
    if (!__isrIsEnabled)
        return;
    static uint32_t lastUs = micros();
    // Get current uS elapsed
    uint32_t curUs = micros();
    uint32_t elapsed = curUs - lastUs;
    if (lastUs > curUs)
        elapsed = 0xffffffff-lastUs+curUs;
    // Check if queue is empty
    if (!__isrRingBufferPosn.canGet())
        return;

    bool allAxesDone = true;
    // Go through axes
    for (int axisIdx = 0; axisIdx < ISR_MAX_AXES; axisIdx++)
    {
        // Get pointer to this axis
        volatile ISRAxisMotionVars* pAxisVars = &(__isrAxisVars[axisIdx]);
        if (!pAxisVars->_isActive)
        {
            // This axis is still active so continue within this block
            allAxesDone = false;

            // Check if the step pin is already in active state - if so lower
            if (pAxisVars->_stepPinValue)
            {
                digitalWrite(pAxisVars->_stepPin, 0);
                pAxisVars->_stepPinValue = 0;
                continue;
            }

            // Set direction if at start of element
            if (!pAxisVars->_dirnPinSetForThisBlock)
            {
                bool stepDirn = pAxisVars->_stepDirn[__isrRingBufferPosn._getPos];
                if (pAxisVars->_dirnPinValue != stepDirn)
                {
                    digitalWrite(pAxisVars->_dirnPin, stepDirn);
                    pAxisVars->_dirnPinValue = stepDirn;
                }
                pAxisVars->_dirnPinSetForThisBlock = true;
            }

            // Accumulate time elapsed since last step
            pAxisVars->_usAccum += elapsed;
            uint32_t stepUs = pAxisVars->_stepUs[__isrRingBufferPosn._getPos];
            // See if it is time for another step
            if (pAxisVars->_usAccum > stepUs)
            {
                if (pAxisVars->_usAccum > stepUs + stepUs)
                    pAxisVars->_usAccum = 0;
                else
                    pAxisVars->_usAccum -= stepUs;
                // Step (and set pin value so that it completes pulse on next ISR call)
                digitalWrite(pAxisVars->_stepPin, 1);
                pAxisVars->_stepPinValue = 1;
                // Count the steps and position from last zero
                pAxisVars->_stepCount++;
#ifdef DEBUG_STEPS_FROM_ZERO
                pAxisVars->_dbgStepsFromLastZero +=
                    (pAxisVars->_stepDirn[__isrRingBufferPosn._getPos] ? 1 : -1);
#endif
                // Check if we have completed the pipeline block
                if (pAxisVars->_stepCount >=
                            pAxisVars->_stepNum[__isrRingBufferPosn._getPos])
                {
                    pAxisVars->_isActive = true;
                    pAxisVars->_stepCount = 0;
                    pAxisVars->_dirnPinSetForThisBlock = false;
                    pAxisVars->_usAccum = 0;
#ifdef DEBUG_TIME_ISR_JITTER
                    pAxisVars->_dbgLastStepUsValid = false;
#endif
                }
                else
                {
#ifdef DEBUG_TIME_ISR_JITTER
                    // Check timing
                    if (pAxisVars->_dbgLastStepUsValid)
                    {
                        uint32_t betweenStepsUs = curUs - pAxisVars->_dbgLastStepUs;
                        if (pAxisVars->_dbgLastStepUs > curUs)
                            betweenStepsUs = 0xffffffff - pAxisVars->_dbgLastStepUs + curUs;
                        if (pAxisVars->_dbgMaxStepUs < betweenStepsUs)
                            pAxisVars->_dbgMaxStepUs = betweenStepsUs;
                        if (pAxisVars->_dbgMinStepUs > betweenStepsUs)
                            pAxisVars->_dbgMinStepUs = betweenStepsUs;
                    }
                    // Record last time
                    pAxisVars->_dbgLastStepUs = curUs;
                    pAxisVars->_dbgLastStepUsValid = true;
#endif
                }
            }
        }
    }
    if (allAxesDone)
        __isrRingBufferPosn.hasGot();
    lastUs = curUs;

#ifdef DEBUG_TIME_ISR_OVERALL
    // Time the ISR over entire execution
    uint32_t endTicks = System.ticks();
    uint32_t elapsedTicks = endTicks - startTicks;
    if (startTicks > endTicks)
        elapsedTicks = 0xffffffff - startTicks + endTicks;
    if (__isrDbgTickMin > elapsedTicks)
        __isrDbgTickMin = elapsedTicks;
    if (__isrDbgTickMax < elapsedTicks)
        __isrDbgTickMax = elapsedTicks;
#endif
}

// Wrapper class to handle access to the pipeline and ISR parameters
class MotionISRManager
{
public:
    MotionISRManager()
    {
        for (int i = 0; i < ISR_MAX_AXES; i++)
            __isrAxisVars[i]._isActive = false;
        __isrIsEnabled = false;
    }

    void start()
    {
        __isrMotionTimer.begin(__isrStepperMotion, 20, uSec);
        __isrIsEnabled = true;
    }

    void stop()
    {
        __isrMotionTimer.end();
    }

    void pauseResume(bool pause)
    {
        __isrIsEnabled == !pause;
    }

    bool setAxis(int axisIdx, int pinStep, int pinDirn)
    {
        if (axisIdx < 0 || axisIdx >= ISR_MAX_AXES)
            return false;
        __isrAxisVars[axisIdx].setPins(pinStep, pinDirn);
    }

    bool canAdd()
    {
        return __isrRingBufferPosn.canPut();
    }

    bool addAxisSteps(int axisIdx, int stepNum, bool stepDirection, uint32_t uSBetweenSteps)
    {
        __isrAxisVars[axisIdx]._stepUs[__isrRingBufferPosn._putPos] = uSBetweenSteps;
        __isrAxisVars[axisIdx]._stepNum[__isrRingBufferPosn._putPos] = stepNum;
        __isrAxisVars[axisIdx]._stepDirn[__isrRingBufferPosn._putPos] = stepDirection;
        __isrAxisVars[axisIdx]._usAccum = 0;
        __isrAxisVars[axisIdx]._stepCount = 0;
        __isrAxisVars[axisIdx]._dirnPinSetForThisBlock = false;
        __isrAxisVars[axisIdx]._isActive = (stepNum == 0);
#ifdef DEBUG_TIME_ISR_JITTER
        __isrAxisVars[axisIdx]._dbgLastStepUsValid = false;
        __isrAxisVars[axisIdx]._dbgMinStepUs = 10000000;
        __isrAxisVars[axisIdx]._dbgMaxStepUs = 0;
#endif
    }

    void addComplete()
    {
        __isrRingBufferPosn.hasPut();
    }
    void showDebug()
    {
        Serial.println("-----------------------------");
        Serial.printlnf("Ring buffer putPos, getPos %d, %d",
                    __isrRingBufferPosn._putPos, __isrRingBufferPosn._getPos);
#ifdef DEBUG_STEPS_FROM_ZERO
        Serial.printlnf("Steps from last zero X,Y,Z %ld,%ld,%ld",
                    __isrAxisVars[0]._dbgStepsFromLastZero,
                    __isrAxisVars[1]._dbgStepsFromLastZero,
                    __isrAxisVars[2]._dbgStepsFromLastZero);
#endif
#ifdef DEBUG_TIME_ISR_OVERALL
        Serial.printlnf("Min/Max ISR exec time %0.2fuS, %0.2fuS",
                    ((double)__isrDbgTickMin)/System.ticksPerMicrosecond(),
                    ((double)__isrDbgTickMax)/System.ticksPerMicrosecond());
        __isrDbgTickMin = 10000000;
        __isrDbgTickMax = 0;
#endif
#ifdef DEBUG_TIME_ISR_JITTER
        Serial.printlnf("Min,Max step times X Y Z %luuS,%luuS %luuS,%luuS  %luuS,%luuS",
                    __isrAxisVars[0]._dbgMinStepUs, __isrAxisVars[0]._dbgMaxStepUs,
                    __isrAxisVars[1]._dbgMinStepUs, __isrAxisVars[1]._dbgMaxStepUs,
                    __isrAxisVars[2]._dbgMinStepUs, __isrAxisVars[2]._dbgMaxStepUs);
        for (int i = 0; i < ISR_MAX_AXES; i++)
            __isrAxisVars[i].dbgResetMinMax();
#endif

        // Serial.printlnf("DBG %lu %lu %d %d", __isrTestMinStepUs, __isrTestMaxStepUs,
        //             __isrRingBufferPosn._putPos, __isrRingBufferPosn._getPos);
        // __isrTestMinStepUs = 1000000;
        // __isrTestMaxStepUs = 0;
    }
};
