// RBotFirmware
// Rob Dobson 2018

#include "MotionActuator.h"

//#define USE_FAST_PIN_ACCESS 1

// Test of motion actuator
TEST_MOTION_ACTUATOR_DEF

// Code here is using the SparkIntervalTimer from PKourany
// https://github.com/pkourany/Spark-Interval-Timer
#ifdef USE_SPARK_INTERVAL_TIMER_ISR

// Static interval timer
IntervalTimer MotionActuator::_isrMotionTimer;

// Static refrerence to a single MotionActuator instance
MotionActuator *MotionActuator::_pMotionActuatorInstance = NULL;

// Function that handles ISR calls based on a timer
// When ISR is enabled this is called every MotionBlock::TICK_INTERVAL_NS nanoseconds
void MotionActuator::_isrStepperMotion(void)
{
    // Instrumentation code to time ISR execution (if enabled - see TestMotionActuator.h)
    TEST_MOTION_ACTUATOR_TIME_START

    // Process block
    if (_pMotionActuatorInstance)
        _pMotionActuatorInstance->procTick();

    // Time execution
    TEST_MOTION_ACTUATOR_TIME_END
}

#endif

// Process method called by main program loop
void MotionActuator::process()
{
    // If not using ISR call procTick on every process call
#ifndef USE_SPARK_INTERVAL_TIMER_ISR
    procTick();
#endif

    // Instrumentation - used to collect test information about operation of MotionActuator
    TEST_MOTION_ACTUATOR_PROCESS
}

// procTick method called either by ISR or via the main program loop
// Handles all motor movement
void MotionActuator::procTick()
{
    // Log.trace("MotionActuator: procTick, paused %d, qLen %d\n", _isPaused, _motionPipeline.count());

    // Instrumentation
    TEST_MOTION_ACTUATOR_STEP_END

    // Do a step-end for any motor which needs one - return here to avoid too short a pulse
    bool anyPinReset = false;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        RobotConsts::RawMotionAxis_t *pAxisInfo = &_rawMotionHwInfo._axis[axisIdx];
        if (pAxisInfo->_pinStepCurLevel == 1)
        {
#ifdef USE_FAST_PIN_ACCESS
            pinResetFast(pAxisInfo->_pinStep);
#else
            digitalWrite(pAxisInfo->_pinStep, 0);
#endif
            anyPinReset = true;
        }
        pAxisInfo->_pinStepCurLevel = 0;
    }
    if (anyPinReset)
    {
        // Log.notice("MotionActuator: ResetPin return\n");
        return;
    }

    // Check if paused
    if (_isPaused)
        return;

    // Peek a MotionPipelineElem from the queue
    MotionBlock *pBlock = _motionPipeline.peekGet();
    if (!pBlock)
        return;

    // Check if the element can be executed
    if (!pBlock->_canExecute)
    {
        // Log.notice("MotionActuator: No exec return\n");
        return;
    }

    // See if the block was already executing and set isExecuting if not
    bool newBlock = !pBlock->_isExecuting;
    pBlock->_isExecuting = true;

    // New block
    if (newBlock)
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
#ifdef USE_FAST_PIN_ACCESS
                digitalWriteFast(pAxisInfo->_pinDirection,
                                 (stepsTotal >= 0) == pAxisInfo->_pinDirectionReversed);
#else
                digitalWrite(pAxisInfo->_pinDirection,
                             (stepsTotal >= 0) == pAxisInfo->_pinDirectionReversed);
#endif
            }
            // Instrumentation
            TEST_MOTION_ACTUATOR_STEP_DIRN
        }

        // Accumulator reset
        _curAccumulatorStep = 0;
        _curAccumulatorNS = 0;

        // Step rate
        _curStepRatePerTTicks = pBlock->_initialStepRatePerTTicks;

        // Log.notice("MotionActuator: New Block XSt %ld, YSt %ld, ZSt %ld, MaxStpAx %d, initRt %ld, maxRt %ld, endRt %ld, acc %ld\n",
        //             _stepsTotalAbs[0], _stepsTotalAbs[1], _stepsTotalAbs[2],
        //             pBlock->_axisIdxWithMaxSteps,
        //             pBlock->_initialStepRatePerTTicks,
        //             pBlock->_maxStepRatePerTTicks,
        //             pBlock->_finalStepRatePerTTicks,
        //             pBlock->_accStepsPerTTicksPerMS);

        // Return here to reduce the maximum time this function takes
        // Assuming this function is called frequently (<50uS intervals say)
        // then it will make little difference if we return now and pick up on the next tick
        return;
    }

    // Check for any end-stops either hit or not hit
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
                switch (pBlock->_endStopsToCheck.get(axisIdx, minMaxIdx))
                {
                case AxisMinMaxBools::END_STOP_HIT:
                {
                    // Check max
                    if (minMaxIdx == AxisMinMaxBools::MAX_VAL_IDX)
                    {
                        pinToTest = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMax;
                        valToTestFor = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMaxactLvl;
                    }
                    // Check min
                    else if (minMaxIdx == AxisMinMaxBools::MIN_VAL_IDX)
                    {
                        pinToTest = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMin;
                        valToTestFor = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMinactLvl;
                    }
                    break;
                }
                case AxisMinMaxBools::END_STOP_NOT_HIT:
                {
                    // Check max
                    if (minMaxIdx == AxisMinMaxBools::MAX_VAL_IDX)
                    {
                        pinToTest = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMax;
                        valToTestFor = !_rawMotionHwInfo._axis[axisIdx]._pinEndStopMaxactLvl;
                    }
                    // Check min
                    else if (minMaxIdx == AxisMinMaxBools::MIN_VAL_IDX)
                    {
                        pinToTest = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMin;
                        valToTestFor = !_rawMotionHwInfo._axis[axisIdx]._pinEndStopMinactLvl;
                    }
                    break;
                }
                case AxisMinMaxBools::END_STOP_TOWARDS:
                {
                    // Check max if going towards max
                    if ((pBlock->_stepsTotalMaybeNeg[axisIdx] > 0) && (minMaxIdx == AxisMinMaxBools::MAX_VAL_IDX))
                    {
                        pinToTest = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMax;
                        valToTestFor = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMaxactLvl;
                    }
                    // Check min if going towards min
                    else if ((pBlock->_stepsTotalMaybeNeg[axisIdx] < 0) && (minMaxIdx == AxisMinMaxBools::MIN_VAL_IDX))
                    {
                        pinToTest = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMin;
                        valToTestFor = _rawMotionHwInfo._axis[axisIdx]._pinEndStopMinactLvl;
                    }
                    break;
                }
                default:
                {
                    break;
                }
                }
                // Log.notice("Ax %d, minMaxIdx %d, EndStopGet %08lx, Steps %ld, pinToTest %d, valToTestFor %d\n", axisIdx, minMaxIdx, pBlock->_endStopsToCheck.get(axisIdx, minMaxIdx),
                //               pBlock->_stepsTotalMaybeNeg[axisIdx], pinToTest, valToTestFor);
            }

            // Check if anything to test
            // Log.notice("End-stop ax%d PINTO %d toTest %d curVal %d\n", axisIdx, pinToTest, valToTestFor, pinToTest >= 0 ? (pinReadFast(pinToTest) != 0) : -1);
            if (pinToTest >= 0)
            {
// Log.notice("End-stop TEST %d, %d, cur %d\n", pinToTest, valToTestFor, pinReadFast(pinToTest));
#ifdef USE_FAST_PIN_ACCESS
                bool pinVal = pinReadFast(pinToTest);
#else
                bool pinVal = digitalRead(pinToTest);
#endif
                if ((pinVal != 0) == valToTestFor)
                {
                    // Cancel motion as end-stop reached
                    _endStopReached = true;
                    _motionPipeline.remove();
                    // Check if this is a numbered block - if so record its completion
                    if (pBlock->getNumberedCommandIndex() != RobotConsts::NUMBERED_COMMAND_NONE)
                    {
                        _lastDoneNumberedCmdIdx = pBlock->getNumberedCommandIndex();
                    }
                    // Log.notice("End-stop reached %d, %d\n", pinToTest, valToTestFor);
                    return;
                }
            }
        }
    }

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
            // Log.notice("MotionActuator: Decel Steps/s %ld Accel %ld\n", _curStepRatePerTTicks, pBlock->_accStepsPerTTicksPerMS);
            if (_curStepRatePerTTicks > std::max(MIN_STEP_RATE_PER_TTICKS + pBlock->_accStepsPerTTicksPerMS,
                                                 pBlock->_finalStepRatePerTTicks + pBlock->_accStepsPerTTicksPerMS))
                _curStepRatePerTTicks -= pBlock->_accStepsPerTTicksPerMS;
        }
        else if (_curStepRatePerTTicks < pBlock->_maxStepRatePerTTicks)
        {
            // Log.notice("MotionActuator: Accel Steps/s %ld Accel %ld\n", _curStepRatePerTTicks, pBlock->_accStepsPerTTicksPerMS);
            if (_curStepRatePerTTicks + pBlock->_accStepsPerTTicksPerMS < MotionBlock::TTICKS_VALUE)
                _curStepRatePerTTicks += pBlock->_accStepsPerTTicksPerMS;
        }
    }

    // Bump the step accumulator
    _curAccumulatorStep += _curStepRatePerTTicks;

    // Check for step accumulator overflow
    if (_curAccumulatorStep >= MotionBlock::TTICKS_VALUE)
    {
        bool anyAxisMoving = false;
        int axisIdxMaxSteps = pBlock->_axisIdxWithMaxSteps;

        // Subtract from accumulator leaving remainder
        _curAccumulatorStep -= MotionBlock::TTICKS_VALUE;

        // Step the axis with the greatest step count if needed
        if (_curStepCount[axisIdxMaxSteps] < _stepsTotalAbs[axisIdxMaxSteps])
        {
            // Step this axis
            RobotConsts::RawMotionAxis_t *pAxisInfo = &_rawMotionHwInfo._axis[axisIdxMaxSteps];
            // Log.notice("pinSetFast: %d (ax %d major)\n", pAxisInfo->_pinStep, axisIdxMaxSteps);
            if (pAxisInfo->_pinStep != -1)
            {
#ifdef USE_FAST_PIN_ACCESS
                pinSetFast(pAxisInfo->_pinStep);
#else
                digitalWrite(pAxisInfo->_pinStep, 1);
#endif
            }
            pAxisInfo->_pinStepCurLevel = 1;
            _curStepCount[axisIdxMaxSteps]++;
            if (_curStepCount[axisIdxMaxSteps] < _stepsTotalAbs[axisIdxMaxSteps])
                anyAxisMoving = true;

            // Instrumentation
            TEST_MOTION_ACTUATOR_STEP_START(axisIdxMaxSteps)
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
#ifdef USE_FAST_PIN_ACCESS
                    pinSetFast(pAxisInfo->_pinStep);
#else
                    digitalWrite(pAxisInfo->_pinStep, 1);
#endif
                }
                //  Log.trace("pinSetFast: %d (ax %d)\n", pAxisInfo->_pinStep, axisIdx);
                pAxisInfo->_pinStepCurLevel = 1;
                _curStepCount[axisIdx]++;
                if (_curStepCount[axisIdx] < _stepsTotalAbs[axisIdx])
                    anyAxisMoving = true;

                // Instrumentation
                TEST_MOTION_ACTUATOR_STEP_START(axisIdx)
            }
        }

        // Any axes still moving?
        if (!anyAxisMoving)
        {
            // If not this block is complete
            _motionPipeline.remove();
            // Log.notice("Block done\n");
            // Check if this is a numbered block - if so record its completion
            if (pBlock->getNumberedCommandIndex() != RobotConsts::NUMBERED_COMMAND_NONE)
            {
                _lastDoneNumberedCmdIdx = pBlock->getNumberedCommandIndex();
            }
        }
    }
}

String MotionActuator::getDebugStr()
{
#ifdef TEST_MOTION_ACTUATOR_ENABLE
    if (_pTestMotionActuator)
        return _pTestMotionActuator->getDebugStr();
    return "";
#else
    return "";
#endif
}

void MotionActuator::showDebug()
{
#ifdef TEST_MOTION_ACTUATOR_ENABLE
    if (_pTestMotionActuator)
        _pTestMotionActuator->showDebug();
#endif
}
