// RBotFirmware
// Rob Dobson 2018

#include "MotionActuator.h"
#include "MotionInstrumentation.h"
#include "MotionPipeline.h"

//#define USE_FAST_PIN_ACCESS 1

// Instrumentation of motion actuator
INSTRUMENT_MOTION_ACTUATOR_INSTANCE

#ifdef USE_ESP32_TIMER_ISR
// Static interval timer
hw_timer_t *MotionActuator::_isrMotionTimer;
#endif

// Static refrerence to a single MotionActuator instance
RobotConsts::RawMotionHwInfo_t MotionActuator::_rawMotionHwInfo;
MotionPipeline* MotionActuator::_pMotionPipeline = NULL;
volatile bool MotionActuator::_isPaused = false;
bool MotionActuator::_isEnabled = false;
bool MotionActuator::_endStopReached = false;
int MotionActuator::_lastDoneNumberedCmdIdx = 0;
uint32_t MotionActuator::_stepsTotalAbs[RobotConsts::MAX_AXES];
uint32_t MotionActuator::_curStepCount[RobotConsts::MAX_AXES];
uint32_t MotionActuator::_curStepRatePerTTicks = 0;
uint32_t MotionActuator::_curAccumulatorStep = 0;
uint32_t MotionActuator::_curAccumulatorNS = 0;
uint32_t MotionActuator::_curAccumulatorRelative[RobotConsts::MAX_AXES];
int MotionActuator::_endStopCheckNum;
MotionActuator::EndStopChecks MotionActuator::_endStopChecks[RobotConsts::MAX_AXES];

// Handle the end of a step for any axis
bool IRAM_ATTR MotionActuator::handleStepEnd()
{
    bool anyPinReset = false;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        RobotConsts::RawMotionAxis_t *pAxisInfo = &_rawMotionHwInfo._axis[axisIdx];
        if (pAxisInfo->_pinStepCurLevel == 1)
        {
            digitalWrite(pAxisInfo->_pinStep, 0);
            anyPinReset = true;
        }
        pAxisInfo->_pinStepCurLevel = 0;
    }
    return anyPinReset;
}

// Setup new block - cache all the info needed to process the block and reset
// motion accumulators to facilitate the block's execution
void IRAM_ATTR MotionActuator::setupNewBlock(MotionBlock *pBlock)
{
    // Step counts and direction for each axis
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        int32_t stepsTotal = pBlock->_stepsTotalMaybeNeg[axisIdx];
        _stepsTotalAbs[axisIdx] = abs(stepsTotal);
        _curStepCount[axisIdx] = 0;
        _curAccumulatorRelative[axisIdx] = 0;
        // Set direction for the axis
        RobotConsts::RawMotionAxis_t *pAxisInfo = &_rawMotionHwInfo._axis[axisIdx];
        if (pAxisInfo->_pinDirection != -1)
        {
            digitalWrite(pAxisInfo->_pinDirection,
                            (stepsTotal >= 0) == pAxisInfo->_pinDirectionReversed);
        }
        // Endstops
        _endStopCheckNum = 0;
        if (pBlock->_endStopsToCheck.any())
        {
            // Go through axes - check if the axis is moving in a direction which might result in hitting an active
            // end-stop
            for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
            {
                int pinToTest = -1;
                bool valToTestFor = false;
                // Anything to check for?
                for (int minMaxIdx = 0; minMaxIdx < AxisMinMaxBools::VALS_PER_AXIS; minMaxIdx++)
                {
                    AxisMinMaxBools::AxisMinMaxEnum minMaxType = pBlock->_endStopsToCheck.get(axisIdx, minMaxIdx);
                    bool checkMax = minMaxIdx == AxisMinMaxBools::MAX_VAL_IDX;
                    bool checkMin = minMaxIdx == AxisMinMaxBools::MIN_VAL_IDX;
                    if (AxisMinMaxBools::END_STOP_TOWARDS)
                    {
                        checkMax = checkMax && pBlock->_stepsTotalMaybeNeg[axisIdx] > 0;
                        checkMin = checkMin && pBlock->_stepsTotalMaybeNeg[axisIdx] < 0;
                    }
                    // Check max
                    if (checkMax)
                    {
                        pinToTest = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMax;
                        valToTestFor = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMaxactLvl;
                    }
                    // Check min
                    else if (checkMin)
                    {
                        pinToTest = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMin;
                        valToTestFor = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMinactLvl;
                    }
                    // Check for inversion of value to test for
                    if (minMaxType == AxisMinMaxBools::END_STOP_NOT_HIT)
                        valToTestFor = !valToTestFor;
                }
                if (pinToTest != -1)
                {
                    _endStopChecks[_endStopCheckNum].pin = pinToTest;
                    _endStopChecks[_endStopCheckNum].val = valToTestFor;
                    _endStopCheckNum++;
                }
            }
        }

        // Instrumentation
        INSTRUMENT_MOTION_ACTUATOR_STEP_DIRN
    }

    // Accumulator reset
    _curAccumulatorStep = 0;
    _curAccumulatorNS = 0;

    // Step rate
    _curStepRatePerTTicks = pBlock->_initialStepRatePerTTicks;
}

// Update millisecond accumulator to handle acceleration and deceleration
void IRAM_ATTR MotionActuator::updateMSAccumulator(MotionBlock *pBlock)
{
    // Bump the millisec accumulator
    _curAccumulatorNS += MotionBlock::TICK_INTERVAL_NS;

    // Check for millisec accumulator overflow
    if (_curAccumulatorNS >= MotionBlock::NS_IN_A_MS)
    {
        // Subtract from accumulator leaving remainder to combat rounding errors
        _curAccumulatorNS -= MotionBlock::NS_IN_A_MS;

        // Check if decelerating
        if (_curStepCount[pBlock->_axisIdxWithMaxSteps] > pBlock->_stepsBeforeDecel)
        {
            if (_curStepRatePerTTicks > std::max(MIN_STEP_RATE_PER_TTICKS + pBlock->_accStepsPerTTicksPerMS,
                                                 pBlock->_finalStepRatePerTTicks + pBlock->_accStepsPerTTicksPerMS))
                _curStepRatePerTTicks -= pBlock->_accStepsPerTTicksPerMS;
        }
        else if (_curStepRatePerTTicks < pBlock->_maxStepRatePerTTicks)
        {
            if (_curStepRatePerTTicks + pBlock->_accStepsPerTTicksPerMS < MotionBlock::TTICKS_VALUE)
                _curStepRatePerTTicks += pBlock->_accStepsPerTTicksPerMS;
        }
    }
}

// Handle start of step on each axis
bool IRAM_ATTR MotionActuator::handleStepMotion(MotionBlock *pBlock)
{
    // Complete Flag
    bool anyAxisMoving = false;

    // Axis with most steps
    int axisIdxMaxSteps = pBlock->_axisIdxWithMaxSteps;

    // Subtract from accumulator leaving remainder
    _curAccumulatorStep -= MotionBlock::TTICKS_VALUE;

    // Step the axis with the greatest step count if needed
    if (_curStepCount[axisIdxMaxSteps] < _stepsTotalAbs[axisIdxMaxSteps])
    {
        // Step this axis
        RobotConsts::RawMotionAxis_t *pAxisInfo = &_rawMotionHwInfo._axis[axisIdxMaxSteps];
        if (pAxisInfo->_pinStep != -1)
        {
            digitalWrite(pAxisInfo->_pinStep, 1);
        }
        pAxisInfo->_pinStepCurLevel = 1;
        _curStepCount[axisIdxMaxSteps]++;
        if (_curStepCount[axisIdxMaxSteps] < _stepsTotalAbs[axisIdxMaxSteps])
            anyAxisMoving = true;

        // Instrumentation
        INSTRUMENT_MOTION_ACTUATOR_STEP_START(axisIdxMaxSteps)
    }

    // Check if other axes need stepping
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        if ((axisIdx == axisIdxMaxSteps) || (_curStepCount[axisIdx] == _stepsTotalAbs[axisIdx]))
            continue;

        // Bump the relative accumulator
        _curAccumulatorRelative[axisIdx] += _stepsTotalAbs[axisIdx];
        if (_curAccumulatorRelative[axisIdx] >= _stepsTotalAbs[axisIdxMaxSteps])
        {
            // Do the remainder calculation
            _curAccumulatorRelative[axisIdx] -= _stepsTotalAbs[axisIdxMaxSteps];

            // Step the axis
            RobotConsts::RawMotionAxis_t *pAxisInfo = &_rawMotionHwInfo._axis[axisIdx];
            if (pAxisInfo->_pinStep != -1)
            {
                digitalWrite(pAxisInfo->_pinStep, 1);
            }
            // Log.trace("MotionActuator::procTick otherAxisStep: %d (ax %d)\n", pAxisInfo->_pinStep, axisIdx);
            pAxisInfo->_pinStepCurLevel = 1;
            _curStepCount[axisIdx]++;
            if (_curStepCount[axisIdx] < _stepsTotalAbs[axisIdx])
                anyAxisMoving = true;

            // Instrumentation
            INSTRUMENT_MOTION_ACTUATOR_STEP_START(axisIdx)
        }
    }

    // Return indicator of block complete
    return anyAxisMoving;
}

void IRAM_ATTR MotionActuator::endMotion(MotionBlock *pBlock)
{
    _pMotionPipeline->remove();
    // Check if this is a numbered block - if so record its completion
    if (pBlock->getNumberedCommandIndex() != RobotConsts::NUMBERED_COMMAND_NONE)
        _lastDoneNumberedCmdIdx = pBlock->getNumberedCommandIndex();
}

// Function that handles ISR calls based on a timer
// When ISR is enabled this is called every MotionBlock::TICK_INTERVAL_NS nanoseconds
void IRAM_ATTR MotionActuator::_isrStepperMotion(void)
{
    // Instrumentation code to time ISR execution (if enabled - see MotionInstrumentation.h)
    INSTRUMENT_MOTION_ACTUATOR_TIME_START

    // Do a step-end for any motor which needs one - return here to avoid too short a pulse
    if (handleStepEnd())
        return;

    // Check if paused
    if (_isPaused)
        return;

    // Peek a MotionPipelineElem from the queue
    MotionBlock *pBlock = _pMotionPipeline->peekGet();
    if (!pBlock)
        return;

    // Check if the element can be executed
    if (!pBlock->_canExecute)
        return;

    // See if the block was already executing and set isExecuting if not
    bool newBlock = !pBlock->_isExecuting;
    pBlock->_isExecuting = true;

    // New block
    if (newBlock)
    {
        // Setup new block
        setupNewBlock(pBlock);

        // Return here to reduce the maximum time this function takes
        // Assuming this function is called frequently (<50uS intervals say)
        // then it will make little difference if we return now and pick up on the next tick
        return;
    }

    // Check endstops        
    bool endStopHit = false;
    for (int i = 0; i < _endStopCheckNum; i++)
    {
        bool pinVal = digitalRead(_endStopChecks[i].pin);
        if (pinVal == _endStopChecks[i].val)
            endStopHit = true;
    }

    // Handle end-stop hit
    if (endStopHit)
    {
        // Cancel motion (by removing the block) as end-stop reached
        _endStopReached = true;
        endMotion(pBlock);
    }

    // Update the millisec accumulator - this handles the process of changing speed incrementally to
    // implement acceleration and deceleration
    updateMSAccumulator(pBlock);

    // Bump the step accumulator
    _curAccumulatorStep += _curStepRatePerTTicks;

    // Check for step accumulator overflow
    if (_curAccumulatorStep >= MotionBlock::TTICKS_VALUE)
    {
        // Flag indicating this block is finished
        bool anyAxisMoving = false;

        // Handle a step
        anyAxisMoving = handleStepMotion(pBlock);

        // Any axes still moving?
        if (!anyAxisMoving)
        {
            // This block is done
            endMotion(pBlock);
        }
    }

    // Time execution
    INSTRUMENT_MOTION_ACTUATOR_TIME_END
}

// Process method called by main program loop
void MotionActuator::process()
{
    // If not using ISR call _isrStepperMotion on every process call
#ifndef USE_ESP32_TIMER_ISR
    _isrStepperMotion();
#endif

    // Instrumentation - used to collect test information about operation of MotionActuator
    INSTRUMENT_MOTION_ACTUATOR_PROCESS
}

String MotionActuator::getDebugStr()
{
#ifdef INSTRUMENT_MOTION_ACTUATOR_ENABLE
    if (_pMotionInstrumentation)
        return _pMotionInstrumentation->getDebugStr();
    return "";
#else
    return "";
#endif
}

void MotionActuator::showDebug()
{
#ifdef INSTRUMENT_MOTION_ACTUATOR_ENABLE
    if (_pMotionInstrumentation)
        _pMotionInstrumentation->showDebug();
#endif
}
