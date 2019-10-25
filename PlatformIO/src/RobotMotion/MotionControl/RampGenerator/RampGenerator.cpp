// RBotFirmware
// Rob Dobson 2018

#include "RampGenerator.h"
#include "MotionInstrumentation.h"
#include "../MotionPipeline.h"

//#define USE_FAST_PIN_ACCESS 1

// Instrumentation of motion actuator
INSTRUMENT_MOTION_ACTUATOR_INSTANCE

//#define DEBUG_MONITOR_ISR_OPERATION 1

// #ifdef USE_ESP32_TIMER_ISR
// // Static interval timer
// hw_timer_t *RampGenerator::_isrMotionTimer;
// #endif

// Static refrerence to a single RampGenerator instance
// volatile int32_t RampGenerator::_axisTotalSteps[RobotConsts::MAX_AXES];
// volatile int32_t RampGenerator::_totalStepsInc[RobotConsts::MAX_AXES];
// RobotConsts::RawMotionHwInfo_t RampGenerator::_rawMotionHwInfo;
// MotionPipeline* RampGenerator::_pMotionPipeline = NULL;
// volatile bool RampGenerator::_isPaused = false;
// bool RampGenerator::_isEnabled = false;
// bool RampGenerator::_endStopReached = false;
// int RampGenerator::_lastDoneNumberedCmdIdx = 0;
// uint32_t RampGenerator::_stepsTotalAbs[RobotConsts::MAX_AXES];
// uint32_t RampGenerator::_curStepCount[RobotConsts::MAX_AXES];
// uint32_t RampGenerator::_curStepRatePerTTicks = 0;
// uint32_t RampGenerator::_curAccumulatorStep = 0;
// uint32_t RampGenerator::_curAccumulatorNS = 0;
// uint32_t RampGenerator::_curAccumulatorRelative[RobotConsts::MAX_AXES];
// int RampGenerator::_endStopCheckNum;
// RampGenerator::EndStopChecks RampGenerator::_endStopChecks[RobotConsts::MAX_AXES];
// bool RampGenerator::_isrTimerStarted = false;
// RampGenIO* RampGenerator::_pMotionIO = NULL;
// bool RampGenerator::_rampGenEnabled = false;

RampGenerator* RampGenerator::_pThis = NULL;

RampGenerator::RampGenerator(MotionPipeline* pMotionPipeline)
{
    // Static refrerence to a single RampGenerator instance
    _pThis = this;

    // Init
    _pMotionPipeline = pMotionPipeline;
    _isPaused = true;
    _endStopReached = false;
    _lastDoneNumberedCmdIdx = RobotConsts::NUMBERED_COMMAND_NONE;
    _isEnabled = false;
    _curStepRatePerTTicks = 0;
    _curAccumulatorStep = 0;
    _curAccumulatorNS = 0;
    _endStopCheckNum = 0;
    _isrTimerStarted = false;
    _rampGenEnabled = false;

#ifdef TEST_MOTION_ACTUATOR_ENABLE
    _pMotionInstrumentation = NULL;
#endif
    resetTotalStepPosition();
}

// void RampGenerator::setRawMotionHwInfo(RobotConsts::RawMotionHwInfo_t &rawMotionHwInfo)
// {
//     _rawMotionHwInfo = rawMotionHwInfo;
// }

void RampGenerator::setInstrumentationMode(const char *testModeStr)
{
#ifdef INSTRUMENT_MOTION_ACTUATOR_ENABLE
    _pMotionInstrumentation = new MotionInstrumentation();
    _pMotionInstrumentation->setInstrumentationMode(testModeStr);
#endif
}

void RampGenerator::deinit()
{
#ifdef USE_ESP32_TIMER_ISR
    if (_isrTimerStarted)
    {
        timerAlarmDisable(_isrMotionTimer);
        _isrTimerStarted = false;
    }
#endif
}

void RampGenerator::configure(bool rampGenEnabled)
{
    // Cache axis and endstop info
    _rampGenIO.getRawMotionHwInfo(_rawMotionHwInfo);

    // TODO check we don't need this...

    // // Give the RampGenerator access to raw motionIO info
    // // this enables ISR based motion to be faster
    // RobotConsts::RawMotionHwInfo_t rawMotionHwInfo;
    // _rampGenerator.setRawMotionHwInfo(rawMotionHwInfo);




    _rampGenEnabled = rampGenEnabled;
    // If we are using the ISR then create the Spark Interval Timer and start it
#ifdef USE_ESP32_TIMER_ISR
    if (_rampGenEnabled)
    {
        Log.notice("RampGenerator: Starting ISR timer for direct stepping\n");
        _isrMotionTimer = timerBegin(0, CLOCK_RATE_MHZ, true);
        timerAttachInterrupt(_isrMotionTimer, _staticISRStepperMotion, true);
        timerAlarmWrite(_isrMotionTimer, DIRECT_STEP_ISR_TIMER_PERIOD_US, true);
        timerAlarmEnable(_isrMotionTimer);
        _isrTimerStarted = true;
    }
#endif
}

void RampGenerator::stop()
{
    _isPaused = true;
    _endStopReached = false;
}

void RampGenerator::pause(bool pauseIt)
{
    _isPaused = pauseIt;
    if (!_isPaused)
    {
        _endStopReached = false;
    }
}

void RampGenerator::resetTotalStepPosition()
{
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
        _axisTotalSteps[i] = 0;
        _totalStepsInc[i] = 0;
    }
}
void RampGenerator::getTotalStepPosition(AxisInt32s& actuatorPos)
{
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
        actuatorPos.setVal(i, _axisTotalSteps[i]);
    }
}
void RampGenerator::setTotalStepPosition(int axisIdx, int32_t stepPos)
{
    if ((axisIdx >= 0) && (axisIdx < RobotConsts::MAX_AXES))
        _axisTotalSteps[axisIdx] = stepPos;
}
void RampGenerator::clearEndstopReached()
{
    _endStopReached = false;
}

bool RampGenerator::isEndStopReached()
{
    return _endStopReached;
}

int RampGenerator::getLastCompletedNumberedCmdIdx()
{
    return _lastDoneNumberedCmdIdx;
}

// Handle the end of a step for any axis
bool IRAM_ATTR RampGenerator::handleStepEnd()
{
    bool anyPinReset = false;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        if (_rampGenIO.stepEnd(axisIdx))
        {
            anyPinReset = true;
            _axisTotalSteps[axisIdx] += _totalStepsInc[axisIdx];
        }
    }
    return anyPinReset;
}

// Setup new block - cache all the info needed to process the block and reset
// motion accumulators to facilitate the block's execution
void IRAM_ATTR RampGenerator::setupNewBlock(MotionBlock *pBlock)
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
        _rampGenIO.setDirection(axisIdx, stepsTotal >= 0);
        _totalStepsInc[axisIdx] = (stepsTotal >= 0) ? 1 : -1;

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
void IRAM_ATTR RampGenerator::updateMSAccumulator(MotionBlock *pBlock)
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
bool IRAM_ATTR RampGenerator::handleStepMotion(MotionBlock *pBlock)
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
        _rampGenIO.stepStart(axisIdxMaxSteps);
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
            _rampGenIO.stepStart(axisIdx);
            // Log.trace("RampGenerator::procTick otherAxisStep: %d (ax %d)\n", pAxisInfo->_pinStep, axisIdx);
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

void IRAM_ATTR RampGenerator::endMotion(MotionBlock *pBlock)
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
void IRAM_ATTR RampGenerator::_staticISRStepperMotion()
{
    if (_pThis)
        _pThis->isrStepperMotion();
}

void IRAM_ATTR RampGenerator::isrStepperMotion()
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
void RampGenerator::process()
{
    // Service RampGenIO
    _rampGenIO.service();

    // If using a controller with a ramp generator then service the block handling
    if (_rampGenEnabled)
    {
        // If not using ISR call _isrStepperMotion on every process call
#ifndef USE_ESP32_TIMER_ISR
        _isrStepperMotion();
#endif
    }

    // Instrumentation - used to collect test information about operation of RampGenerator
    INSTRUMENT_MOTION_ACTUATOR_PROCESS
}

String RampGenerator::getDebugStr()
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

void RampGenerator::showDebug()
{
#ifdef INSTRUMENT_MOTION_ACTUATOR_ENABLE
    if (_pMotionInstrumentation)
        _pMotionInstrumentation->showDebug();
#endif
}
