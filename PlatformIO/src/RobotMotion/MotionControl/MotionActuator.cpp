// RBotFirmware
// Rob Dobson 2018

#include "MotionActuator.h"
#include "MotionInstrumentation.h"
#include "MotionPipeline.h"

//#define USE_FAST_PIN_ACCESS 1

// Instrumentation of motion actuator
INSTRUMENT_MOTION_ACTUATOR_INSTANCE

//#define DEBUG_MONITOR_ISR_OPERATION 1

#ifdef USE_ESP32_TIMER_ISR
// Static interval timer
hw_timer_t *MotionActuator::_isrMotionTimer;
#endif

// Static refrerence to a single MotionActuator instance
volatile int32_t MotionActuator::_totalStepsMoved[RobotConsts::MAX_AXES];
volatile int32_t MotionActuator::_totalStepsInc[RobotConsts::MAX_AXES];
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
TrinamicController* MotionActuator::_pTrinamicController = NULL;
bool MotionActuator::_isrTimerStarted = false;

MotionActuator::MotionActuator(TrinamicController* pTrinamicController, MotionPipeline* pMotionPipeline)
{
    // Init
    _pMotionPipeline = pMotionPipeline;
    _pTrinamicController = pTrinamicController;
    clear();
    resetTotalStepPosition();
}

void MotionActuator::setRawMotionHwInfo(RobotConsts::RawMotionHwInfo_t &rawMotionHwInfo)
{
    _rawMotionHwInfo = rawMotionHwInfo;
}

void MotionActuator::setInstrumentationMode(const char *testModeStr)
{
#ifdef INSTRUMENT_MOTION_ACTUATOR_ENABLE
    _pMotionInstrumentation = new MotionInstrumentation();
    _pMotionInstrumentation->setInstrumentationMode(testModeStr);
#endif
}

void MotionActuator::deinit()
{
#ifdef USE_ESP32_TIMER_ISR
    if (_isrTimerStarted)
    {
        timerAlarmDisable(_isrMotionTimer);
        _isrTimerStarted = false;
    }
#endif
}

void MotionActuator::configure()
{
    // If we are using the ISR then create the Spark Interval Timer and start it
#ifdef USE_ESP32_TIMER_ISR
    if (!((_pTrinamicController) && (_pTrinamicController->isEnabled())))
    {    
        _isrMotionTimer = timerBegin(0, CLOCK_RATE_MHZ, true);
        timerAttachInterrupt(_isrMotionTimer, _isrStepperMotion, true);
        timerAlarmWrite(_isrMotionTimer, DIRECT_STEP_ISR_TIMER_PERIOD_US, true);
        timerAlarmEnable(_isrMotionTimer);
        Log.notice("MotionActuator: Starting ISR timer for direct stepping\n");
        _isrTimerStarted = true;
    }
#endif
}

void MotionActuator::clear()
{
    _isPaused = true;
    _endStopReached = false;
    _lastDoneNumberedCmdIdx = RobotConsts::NUMBERED_COMMAND_NONE;
#ifdef TEST_MOTION_ACTUATOR_ENABLE
    _pMotionInstrumentation = NULL;
#endif
}

void MotionActuator::stop()
{
    _isPaused = true;
    _endStopReached = false;
}

void MotionActuator::pause(bool pauseIt)
{
    _isPaused = pauseIt;
    if (!_isPaused)
    {
        _endStopReached = false;
    }
}

void MotionActuator::resetTotalStepPosition()
{
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
        _totalStepsMoved[i] = 0;
        _totalStepsInc[i] = 0;
    }
}
void MotionActuator::getTotalStepPosition(AxisInt32s& actuatorPos)
{
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
        actuatorPos.setVal(i, _totalStepsMoved[i]);
    }
}
void MotionActuator::setTotalStepPosition(int axisIdx, int32_t stepPos)
{
    if ((axisIdx >= 0) && (axisIdx < RobotConsts::MAX_AXES))
        _totalStepsMoved[axisIdx] = stepPos;
}
void MotionActuator::clearEndstopReached()
{
    _endStopReached = false;
}

bool MotionActuator::isEndStopReached()
{
    return _endStopReached;
}

int MotionActuator::getLastCompletedNumberedCmdIdx()
{
    return _lastDoneNumberedCmdIdx;
}

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
            _totalStepsMoved[axisIdx] += _totalStepsInc[axisIdx];
        }
        pAxisInfo->_pinStepCurLevel = 0;
    }
    return anyPinReset;
}

// Setup new block - cache all the info needed to process the block and reset
// motion accumulators to facilitate the block's execution
void IRAM_ATTR MotionActuator::setupNewBlock(MotionBlock *pBlock)
{
    // Setup step counts, direction and endstops for each axis
    _endStopCheckNum = 0;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        // Total steps
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
            _totalStepsInc[axisIdx] = (stepsTotal >= 0) ? 1 : -1;
        }

        // Instrumentation
        INSTRUMENT_MOTION_ACTUATOR_STEP_DIRN

        // Check if any endstops to setup
        if (!pBlock->_endStopsToCheck.any())
            continue;

        // Check if the axis is moving in a direction which might result in hitting an active end-stop
        for (int minMaxIdx = 0; minMaxIdx < AxisMinMaxBools::ENDSTOPS_PER_AXIS; minMaxIdx++)
        {
            int pinToTest = -1;
            bool valToTestFor = false;

            // See if anything to check for
            AxisMinMaxBools::AxisMinMaxEnum minMaxType = pBlock->_endStopsToCheck.get(axisIdx, minMaxIdx);
            if (minMaxType == AxisMinMaxBools::END_STOP_NONE)
                continue;

            // Check for towards - this is different from MAX or MIN because the axis will still move even if
            // an endstop is hit if the movement is away from that endstop
            if (minMaxType == AxisMinMaxBools::END_STOP_TOWARDS)
            {
                // Stop at max if we're heading towards max OR
                // stop at min if we're heading towards min
                if (!(((minMaxIdx == AxisMinMaxBools::MAX_VAL_IDX) && (stepsTotal > 0)) ||
                        ((minMaxIdx == AxisMinMaxBools::MIN_VAL_IDX) && (stepsTotal < 0))))
                    continue;
            }
            
            // Pin for stop
            pinToTest = (minMaxIdx == AxisMinMaxBools::MIN_VAL_IDX) ? 
                                _rawMotionHwInfo._axis[axisIdx]._pinEndStopMin : 
                                _rawMotionHwInfo._axis[axisIdx]._pinEndStopMax;

            // Endstop test
            valToTestFor = (minMaxType != AxisMinMaxBools::END_STOP_NOT_HIT) ? 
                                _rawMotionHwInfo._axis[axisIdx]._pinEndStopMaxactLvl :
                                !_rawMotionHwInfo._axis[axisIdx]._pinEndStopMaxactLvl;
            if (pinToTest != -1)
            {
                _endStopChecks[_endStopCheckNum].pin = pinToTest;
                _endStopChecks[_endStopCheckNum].val = valToTestFor;
                _endStopCheckNum++;
            }
        }
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
        else if ((_curStepRatePerTTicks < MIN_STEP_RATE_PER_TTICKS) || (_curStepRatePerTTicks < pBlock->_maxStepRatePerTTicks))
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

#ifdef DEBUG_MONITOR_ISR_OPERATION
volatile uint32_t accumStep = 0;
volatile uint32_t stepRate = 0;
volatile uint32_t accelacc = 0;
volatile int maxstepax = -1;
volatile uint32_t accrate = 0;
volatile int curSteps = -1;
volatile int befDec = -1;
volatile int maxStepRt = -1;
#endif

// Function that handles ISR calls based on a timer
// When ISR is enabled this is called every MotionBlock::TICK_INTERVAL_NS nanoseconds
void IRAM_ATTR MotionActuator::_isrStepperMotion()
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
    _curAccumulatorStep += std::max(_curStepRatePerTTicks, MIN_STEP_RATE_PER_TTICKS);

#ifdef DEBUG_MONITOR_ISR_OPERATION
    accumStep = _curAccumulatorStep;
    stepRate = _curStepRatePerTTicks;
    accelacc = _curAccumulatorNS;
    maxstepax = pBlock->_axisIdxWithMaxSteps;
    accrate = pBlock->_accStepsPerTTicksPerMS;
    curSteps = _curStepCount[pBlock->_axisIdxWithMaxSteps];
    befDec =pBlock->_stepsBeforeDecel;
    maxStepRt = pBlock->_maxStepRatePerTTicks;
#endif

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
#endif
#ifdef DEBUG_MONITOR_ISR_OPERATION
    char dbg[200];
    sprintf(dbg, "accum %d rate %d accacc %d maxstepidx %d accrate %d cursteps %d befDec %d maxRt %d",
            accumStep, stepRate, accelacc, maxstepax, accrate, curSteps, befDec, maxStepRt);
    anymov = 6;
    return dbg;
#endif
    return "";
}

void MotionActuator::showDebug()
{
#ifdef INSTRUMENT_MOTION_ACTUATOR_ENABLE
    if (_pMotionInstrumentation)
        _pMotionInstrumentation->showDebug();
#endif
}
