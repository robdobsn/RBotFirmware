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
#ifdef TEST_MOTION_ACTUATOR_ENABLE
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
#ifdef TEST_MOTION_ACTUATOR_ENABLE
	if (_pTestMotionActuator)
		_pTestMotionActuator->stepEnd();
#endif

	// Do a step-end for any motor which needs one - return here to avoid too short a pulse
	if (_motionIO.stepEnd())
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
		// Prep each axis separately
		for (uint8_t axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			// Set inactive for now
			axisExecData_t& axisExecData = _axisExecData[axisIdx];
			axisExecData._isActive = false;

			// Find first phase with steps
			MotionBlock::axisStepData_t& axisStepData = pBlock->_axisStepData[axisIdx];
			if (axisStepData._stepsInAccPhase + axisStepData._stepsInPlateauPhase + axisStepData._stepsInDecelPhase != 0)
			{
				// Initialse accumulators and other vars
				axisExecData._minStepRatePerKTicks = axisStepData._minStepRatePerKTicks;
				axisExecData._curStepRatePerKTicks = axisStepData._initialStepRatePerKTicks;
				axisExecData._accStepsPerKTicksPerMS = axisStepData._accStepsPerKTicksPerMS;
				axisExecData._stepsAccPhase = axisStepData._stepsInAccPhase;
				axisExecData._stepsPlateauPhase = axisStepData._stepsInPlateauPhase;
				axisExecData._stepsDecelPhase = axisStepData._stepsInDecelPhase;
				axisExecData._curAccumulatorStep = 0;
				axisExecData._curAccumulatorNS = 0;
				axisExecData._curPhaseStepCount = 0;
				// Find the phase and target count
				axisExecData._axisStepPhaseNum = MotionBlock::STEP_PHASE_ACCEL;
				axisExecData._targetStepCount = axisExecData._stepsAccPhase;
				if (axisExecData._stepsAccPhase == 0)
				{
					if (axisExecData._stepsPlateauPhase != 0)
					{
						axisExecData._axisStepPhaseNum = MotionBlock::STEP_PHASE_PLATEAU;
						axisExecData._targetStepCount = axisExecData._stepsPlateauPhase;
					}
					else
					{
						axisExecData._axisStepPhaseNum = MotionBlock::STEP_PHASE_DECEL;
						axisExecData._targetStepCount = axisExecData._stepsDecelPhase;
					}
				}
				// Set active
				axisExecData._isActive = true;
				// Set direction for the axis
				_motionIO.stepDirn(axisIdx, pBlock->_axisStepsToTarget.getVal(axisIdx) >= 0);

				// Test code
#ifdef TEST_MOTION_ACTUATOR_ENABLE
				if (_pTestMotionActuator)
					_pTestMotionActuator->stepDirn(axisIdx, pBlock->_axisStepsToTarget.getVal(axisIdx) >= 0);
#endif

				//Log.trace("BLK axisIdx %d stepsToTarget %ld stepRtPerKTks %ld accStepsPerKTksPerMs %ld stepAcc %ld stepPlat %ld stepDecel %ld",
				//			axisIdx, pBlock->_axisStepsToTarget.getVal(axisIdx),
				//			axisExecData._curStepRatePerKTicks, axisExecData._accStepsPerKTicksPerMS,
				//			axisExecData._stepsAccPhase, axisExecData._stepsPlateauPhase, axisExecData._stepsDecelPhase);

			}
		}
		// Return here to reduce the maximum time this function takes
		// Assuming this function is called frequently (<50uS intervals say)
		// then it will make little difference if we return now and pick up on the next tick
		return;
	}

	// Go through the axes
	bool anyAxisMoving = false;
	for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
	{
		// Check if there is any motion left for this axis
		axisExecData_t& axisExecData = _axisExecData[axisIdx];
		if (!axisExecData._isActive)
			continue;

		// Bump the millisec accumulator
		axisExecData._curAccumulatorNS += MotionBlock::TICK_INTERVAL_NS;

		// Check for millisec accumulator overflow
		if (axisExecData._curAccumulatorNS >= MotionBlock::NS_IN_A_MS)
		{
			// Subtract from accumulator leaving remainder to combat rounding errors
			axisExecData._curAccumulatorNS -= MotionBlock::NS_IN_A_MS;

			// Accelerate as required (changing interval between steps)
			if (axisExecData._axisStepPhaseNum == MotionBlock::STEP_PHASE_ACCEL)
			{
				//Log.trace("Accel Steps/s %ld Accel %ld", axisExecData._curStepRatePerKTicks, axisExecData._accStepsPerKTicksPerMS);
				if (axisExecData._curStepRatePerKTicks + axisExecData._accStepsPerKTicksPerMS < MotionBlock::TTICKS_VALUE)
					axisExecData._curStepRatePerKTicks += axisExecData._accStepsPerKTicksPerMS;
				//else
				//	Log.trace("Didn't add acceleration");
			}
			else if (axisExecData._axisStepPhaseNum == MotionBlock::STEP_PHASE_DECEL)
			{
				//Log.trace("Decel Steps/s %ld Accel %ld", axisExecData._curStepRatePerKTicks, axisExecData._accStepsPerKTicksPerMS);
				if (axisExecData._curStepRatePerKTicks > axisExecData._minStepRatePerKTicks * 2)
					axisExecData._curStepRatePerKTicks -= axisExecData._accStepsPerKTicksPerMS;
				//else
				//	Log.trace("Didn't sub acceleration");
			}
		}

		// Bump the step accumulator
		axisExecData._curAccumulatorStep += axisExecData._curStepRatePerKTicks;

		// Check for step accumulator overflow
		if (axisExecData._curAccumulatorStep >= MotionBlock::TTICKS_VALUE)
		{
			// Subtract from accumulator leaving remainder
			axisExecData._curAccumulatorStep -= MotionBlock::TTICKS_VALUE;

			// Step
			_motionIO.stepStart(axisIdx);
			axisExecData._curPhaseStepCount++;

			// Test code
#ifdef TEST_MOTION_ACTUATOR_ENABLE
			if (_pTestMotionActuator)
				_pTestMotionActuator->stepStart(axisIdx);
#endif

			// Check if phase is done
			if (axisExecData._curPhaseStepCount >= axisExecData._targetStepCount)
			{
				// Check for the next phase
				axisExecData._curPhaseStepCount = 0;
				axisExecData._isActive = false;
				if (axisExecData._axisStepPhaseNum == MotionBlock::STEP_PHASE_ACCEL)
				{
					if (axisExecData._stepsPlateauPhase != 0)
					{
						axisExecData._axisStepPhaseNum = MotionBlock::STEP_PHASE_PLATEAU;
						axisExecData._targetStepCount = axisExecData._stepsPlateauPhase;
						axisExecData._isActive = true;
					}
					else if (axisExecData._stepsDecelPhase != 0)
					{
						axisExecData._axisStepPhaseNum = MotionBlock::STEP_PHASE_DECEL;
						axisExecData._targetStepCount = axisExecData._stepsDecelPhase;
						axisExecData._isActive = true;
					}
				}
				else if (axisExecData._axisStepPhaseNum == MotionBlock::STEP_PHASE_PLATEAU)
				{
					if (axisExecData._stepsDecelPhase != 0)
					{
						axisExecData._axisStepPhaseNum = MotionBlock::STEP_PHASE_DECEL;
						axisExecData._targetStepCount = axisExecData._stepsDecelPhase;
						axisExecData._isActive = true;
					}
				}
			}
		}
		// Check if axis is moving
		anyAxisMoving |= axisExecData._isActive;
	}

	// Any axes still moving?
	if (!anyAxisMoving)
	{
		// If not this block is complete
		_motionPipeline.remove();
	}
}

void MotionActuator::showDebug()
{
#ifdef TEST_MOTION_ACTUATOR_ENABLE
	if (_pTestMotionActuator)
		_pTestMotionActuator->showDebug();
#endif
}
