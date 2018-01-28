// RBotFirmware
// Rob Dobson 2018

#include "MotionActuator.h"

// Test of motion actuator
#ifdef TEST_MOTION_ACTUATOR_ENABLE
TestMotionActuator* MotionActuator::_pTestMotionActuator = NULL;
#endif

#ifdef USE_SPARK_INTERVAL_TIMER_ISR

// Static interval timer
IntervalTimer MotionActuator::_isrMotionTimer;

// Static refrerence to a single MotionActuator instance
MotionActuator* MotionActuator::_pMotionActuatorInstance = NULL;

void MotionActuator::_isrStepperMotion(void)
{
  // Test code if enabled
#ifdef TEST_MOTION_ACTUATOR_ENABLE
  if (_pTestMotionActuator)
  {
    _pTestMotionActuator->blink();
    // Time execution
    _pTestMotionActuator->timeStart();
  }
#endif

  // Process block
  if (_pMotionActuatorInstance)
    _pMotionActuatorInstance->procTick();

#ifdef TEST_MOTION_ACTUATOR_ENABLE
  // Time execution
  if (_pTestMotionActuator)
    _pTestMotionActuator->timeEnd();
#endif
}

#endif

void MotionActuator::process()
{
  // If not using ISR call procTick on every process call
#ifndef USE_SPARK_INTERVAL_TIMER_ISR
  procTick();
#endif

  // Test code if enabled process test data
#ifdef TEST_MOTION_ACTUATOR_OUTPUT
  if (_pTestMotionActuator)
    _pTestMotionActuator->process();
#endif
}

void MotionActuator::procTick()
{
  // Check if paused
  if (_isPaused)
    return;

  // Test code
#ifdef TEST_MOTION_ACTUATOR_OUTPUT
  if (_pTestMotionActuator)
    _pTestMotionActuator->stepEnd();
#endif

  // Do a step-end for any motor which needs one - return here to avoid too short a pulse
  bool anyPinReset = false;
  for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
  {
    RobotConsts::RawMotionAxis_t* pAxisInfo = &_rawMotionHwInfo._axis[axisIdx];
    if (pAxisInfo->_pinStepCurLevel == 1)
    {
      pinResetFast(pAxisInfo->_pinStep);
      anyPinReset = true;
    }
    pAxisInfo->_pinStepCurLevel = 0;
  }
  if (anyPinReset)
    return;

  // Peek a MotionPipelineElem from the queue
  MotionBlock* pBlock = _motionPipeline.peekGet();
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
    // Step counts and direction for each axis
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
      int32_t stepsTotal = pBlock->_stepsTotalMaybeNeg[axisIdx];
      _stepsTotalAbs[axisIdx]          = abs(stepsTotal);
      _curStepCount[axisIdx]           = 0;
      _curAccumulatorRelative[axisIdx] = 0;
      // Set direction for the axis
      RobotConsts::RawMotionAxis_t* pAxisInfo = &_rawMotionHwInfo._axis[axisIdx];
      if (pAxisInfo->_pinDirection != -1)
        digitalWriteFast(pAxisInfo->_pinDirection,
                         (stepsTotal >= 0) == pAxisInfo->_pinDirectionReversed);

      // Test code
#ifdef TEST_MOTION_ACTUATOR_OUTPUT
      if (_pTestMotionActuator)
        _pTestMotionActuator->stepDirn(axisIdx, stepsTotal >= 0);
#endif
    }

    // Accumulator reset
    _curAccumulatorStep = 0;
    _curAccumulatorNS   = 0;

    // Step rate
    _curStepRatePerTTicks = pBlock->_initialStepRatePerTTicks;

    // Return here to reduce the maximum time this function takes
    // Assuming this function is called frequently (<50uS intervals say)
    // then it will make little difference if we return now and pick up on the next tick
    return;
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
      //Log.trace("Decel Steps/s %ld Accel %ld", axisExecData._curStepRatePerKTicks, axisExecData._accStepsPerKTicksPerMS);
      if (_curStepRatePerTTicks > std::max(MIN_STEP_RATE_PER_TTICKS + pBlock->_accStepsPerTTicksPerMS,
                                           pBlock->_finalStepRatePerTTicks + pBlock->_accStepsPerTTicksPerMS))
        _curStepRatePerTTicks -= pBlock->_accStepsPerTTicksPerMS;
      //else
      //	Log.trace("Didn't sub acceleration");
    }
    else if (_curStepRatePerTTicks < pBlock->_maxStepRatePerTTicks)
    {
      //Log.trace("Accel Steps/s %ld Accel %ld", axisExecData._curStepRatePerKTicks, axisExecData._accStepsPerKTicksPerMS);
      if (_curStepRatePerTTicks + pBlock->_accStepsPerTTicksPerMS < MotionBlock::TTICKS_VALUE)
        _curStepRatePerTTicks += pBlock->_accStepsPerTTicksPerMS;
      //else
      //	Log.trace("Didn't add acceleration");
    }
  }

  // Bump the step accumulator
  _curAccumulatorStep += _curStepRatePerTTicks;

  // Check for step accumulator overflow
  if (_curAccumulatorStep >= MotionBlock::TTICKS_VALUE)
  {
    bool anyAxisMoving   = false;
    int  axisIdxMaxSteps = pBlock->_axisIdxWithMaxSteps;

    // Subtract from accumulator leaving remainder
    _curAccumulatorStep -= MotionBlock::TTICKS_VALUE;

    // Step the axis with the greatest step count if needed
    if (_curStepCount[axisIdxMaxSteps] < _stepsTotalAbs[axisIdxMaxSteps])
    {
      // Step this axis
      RobotConsts::RawMotionAxis_t* pAxisInfo = &_rawMotionHwInfo._axis[axisIdxMaxSteps];
      if (pAxisInfo->_pinStep != -1)
        pinSetFast(pAxisInfo->_pinStep);
      pAxisInfo->_pinStepCurLevel = 1;
      _curStepCount[axisIdxMaxSteps]++;
      if (_curStepCount[axisIdxMaxSteps] < _stepsTotalAbs[axisIdxMaxSteps])
        anyAxisMoving = true;

      // Test code
#ifdef TEST_MOTION_ACTUATOR_OUTPUT
      if (_pTestMotionActuator)
        _pTestMotionActuator->stepStart(axisIdxMaxSteps);
#endif
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
        RobotConsts::RawMotionAxis_t* pAxisInfo = &_rawMotionHwInfo._axis[axisIdx];
        if (pAxisInfo->_pinStep != -1)
          pinSetFast(pAxisInfo->_pinStep);
        pAxisInfo->_pinStepCurLevel = 1;
        _curStepCount[axisIdx]++;
        if (_curStepCount[axisIdx] < _stepsTotalAbs[axisIdx])
          anyAxisMoving = true;

        // Test code
#ifdef TEST_MOTION_ACTUATOR_OUTPUT
        if (_pTestMotionActuator)
          _pTestMotionActuator->stepStart(axisIdx);
#endif
      }
    }

    // Any axes still moving?
    if (!anyAxisMoving)
    {
      // If not this block is complete
      _motionPipeline.remove();
    }
  }
}

void MotionActuator::showDebug()
{
#ifdef TEST_MOTION_ACTUATOR_ENABLE
  if (_pTestMotionActuator)
    _pTestMotionActuator->showDebug();
#endif
}
