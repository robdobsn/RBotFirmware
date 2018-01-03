#pragma once

#include "application.h"
#include "MotionPipeline.h"
#include "MotionIO.h"

class MotionActuator
{
private:
	volatile bool _isPaused;
	MotionPipeline& _motionPipeline;
	MotionIO& _motionIO;

private:

#ifdef USE_SMOOTHIE_CODE
	// Currently executing block
	struct axisExecInfo_t
	{
		bool _isActive;
		uint32_t _curAccumulatorNs;
		uint32_t _curStepIntervalNs;
		uint32_t _curStepCount;
		uint32_t _targetStepCount;
		int32_t _stepIntervalChangePerStepNs;
		int _axisStepPhaseNum;
	};
	axisExecInfo_t _axisExecInfo[RobotConsts::MAX_AXES];
#endif

	struct axisExecData_t
	{
		// True while axis is active - when all false block is complete
		bool _isActive;
		// Steps in each phase of motion
		uint32_t _stepsAccPhase;
		uint32_t _stepsPlateauPhase;
		uint32_t _stepsDecelPhase;
		// Current step rate (in steps per K ticks) and count of steps made in this block
		uint32_t _curStepRatePerKTicks;
		uint32_t _curPhaseStepCount;
		uint32_t _targetStepCount;
		// Phase number (accel/plateau/decel) and acceleration rate
		int _axisStepPhaseNum;
		uint32_t _accStepsPerKTicksPerMS;
		// Accumulators for stepping and acceleration increments
		uint32_t _curAccumulatorStep;
		uint32_t _curAccumulatorNS;
	};
	axisExecData_t _axisExecData[RobotConsts::MAX_AXES];

public:
	MotionActuator(MotionIO& motionIO, MotionPipeline& motionPipeline) :
		_motionPipeline(motionPipeline),
		_motionIO(motionIO)
	{
		_isPaused = false;
	}

	void config()
	{

	}

	void clear()
	{
		_isPaused = false;
	}

	void pause(bool pauseIt)
	{
		_isPaused = pauseIt;
	}

	void process()
	{
		// Check if paused
		if (_isPaused)
			return;

		// Do a step-end for any motor which needs one - return here to avoid too short a pulse
		if (_motionIO.stepEnd())
			return;

		// Peek a MotionPipelineElem from the queue
		MotionBlock* pBlock = _motionPipeline.peekGet();
		if (!pBlock)
			return;

		// Check if the element is being changed
		if (pBlock->_changeInProgress)
			return;

		// Signal that we are now working on this block
		bool newBlock = !pBlock->_isRunning;
		pBlock->_isRunning = true;

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

					//Log.trace("BLK axisIdx %d stepsToTarget %ld stepRtPerKTks %ld accStepsPerKTksPerMs %ld stepAcc %ld stepPlat %ld stepDecel %ld",
					//			axisIdx, pBlock->_axisStepsToTarget.getVal(axisIdx), 
					//			axisExecData._curStepRatePerKTicks, axisExecData._accStepsPerKTicksPerMS, 
					//			axisExecData._stepsAccPhase, axisExecData._stepsPlateauPhase, axisExecData._stepsDecelPhase);
				}
			}
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
					if (axisExecData._curStepRatePerKTicks + axisExecData._accStepsPerKTicksPerMS < MotionBlock::K_VALUE)
						axisExecData._curStepRatePerKTicks += axisExecData._accStepsPerKTicksPerMS;
					//else
					//	Log.trace("Didn't add acceleration");
				}
				else if (axisExecData._axisStepPhaseNum == MotionBlock::STEP_PHASE_DECEL)
				{
					//Log.trace("Decel Steps/s %ld Accel %ld", axisExecData._curStepRatePerKTicks, axisExecData._accStepsPerKTicksPerMS);
					if (axisExecData._curStepRatePerKTicks > axisExecData._accStepsPerKTicksPerMS)
						axisExecData._curStepRatePerKTicks -= axisExecData._accStepsPerKTicksPerMS;
					//else
					//	Log.trace("Didn't sub acceleration");
				}
			}

			// Bump the step accumulator
			axisExecData._curAccumulatorStep += axisExecData._curStepRatePerKTicks;

			// Check for step accumulator overflow
			if (axisExecData._curAccumulatorStep >= MotionBlock::K_VALUE)
			{
				// Subtract from accumulator leaving remainder
				axisExecData._curAccumulatorStep -= MotionBlock::K_VALUE;

				// Step
				_motionIO.stepStart(axisIdx);
				axisExecData._curPhaseStepCount++;

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

#ifdef USE_SMOOTHIE_CODE

	void processNSInterval()
	{
		// Check if paused
		if (_isPaused)
			return;

		// Do a step-end for any motor which needs one - return here to avoid too short a pulse
		if (_motionIO.stepEnd())
			return;

		// Peek a MotionPipelineElem from the queue
		MotionBlock* pBlock = _motionPipeline.peekGet();
		if (!pBlock)
			return;

		// Check if the element is being changed
		if (pBlock->_changeInProgress)
			return;

		// Signal that we are now working on this block
		bool newBlock = !pBlock->_isRunning;
		pBlock->_isRunning = true;

		// New block
		if (newBlock)
		{
			// Prep each axis separately
			for (uint8_t axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
			{
				// Set inactive for now
				axisExecInfo_t& axisExecInfo = _axisExecInfo[axisIdx];
				axisExecInfo._isActive = false;
				// Intialise the phase of the block
				for (int phaseIdx = 0; phaseIdx < MotionBlock::MAX_STEP_PHASES; phaseIdx++)
				{
					// Check if there are any steps in this phase
					MotionBlock::axisStepInfo_t& axisStepInfo = pBlock->_axisStepInfo[axisIdx];
					if (axisStepInfo._stepPhases[phaseIdx]._stepsInPhase != 0)
					{
						// Initialise the first active phase
						axisExecInfo._curAccumulatorNs = 0;
						axisExecInfo._curStepIntervalNs = axisStepInfo._initialStepIntervalNs;
						axisExecInfo._curStepCount = 0;
						axisExecInfo._targetStepCount = axisStepInfo._stepPhases[phaseIdx]._stepsInPhase;
						axisExecInfo._stepIntervalChangePerStepNs = axisStepInfo._stepPhases[phaseIdx]._stepIntervalChangeNs;
						axisExecInfo._axisStepPhaseNum = phaseIdx;
						axisExecInfo._isActive = true;
						// Set direction for the axis
						_motionIO.stepDirn(axisIdx, pBlock->_axisStepsToTarget.getVal(axisIdx) >= 0);
						break;
					}
				}
			}
		}

		// Go through the axes
		bool anyAxisMoving = false;
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			// Check if there is any motion left for this axis
			axisExecInfo_t& axisExecInfo = _axisExecInfo[axisIdx];
			if (!axisExecInfo._isActive)
				continue;

			// Bump the accumulator
			axisExecInfo._curAccumulatorNs += MotionBlock::TICK_INTERVAL_NS;

			// Check for accumulator overflow
			if (axisExecInfo._curAccumulatorNs >= axisExecInfo._curStepIntervalNs)
			{
				// Subtract from accumulator leaving remainder to combat rounding errors
				axisExecInfo._curAccumulatorNs -= axisExecInfo._curStepIntervalNs;

				// Accelerate as required (changing interval between steps)
				axisExecInfo._curStepIntervalNs += axisExecInfo._stepIntervalChangePerStepNs;

				// Step
				_motionIO.stepStart(axisIdx);
				axisExecInfo._curStepCount++;

				// Check if phase is done
				if (axisExecInfo._curStepCount >= axisExecInfo._targetStepCount)
				{
					// Check for the next phase
					axisExecInfo._isActive = false;
					MotionBlock::axisStepInfo_t& axisStepInfo = pBlock->_axisStepInfo[axisIdx];
					for (int phaseIdx = axisExecInfo._axisStepPhaseNum + 1; phaseIdx < MotionBlock::MAX_STEP_PHASES; phaseIdx++)
					{
						// Check if there are any steps in this phase
						if (axisStepInfo._stepPhases[phaseIdx]._stepsInPhase != 0)
						{
							// Initialise the phase
							axisExecInfo._curStepCount = 0;
							axisExecInfo._targetStepCount = axisStepInfo._stepPhases[phaseIdx]._stepsInPhase;
							axisExecInfo._stepIntervalChangePerStepNs = axisStepInfo._stepPhases[phaseIdx]._stepIntervalChangeNs;
							axisExecInfo._axisStepPhaseNum = phaseIdx;
							axisExecInfo._isActive = true;
							break;
						}
					}
				}
			}

			// Check if axis is moving
			anyAxisMoving |= axisExecInfo._isActive;
		}

		// Any axes still moving?
		if (!anyAxisMoving)
		{

			// If not this block is complete
			_motionPipeline.remove();
		}

	}

#endif

	void processSmoothie()
	{
#ifdef USE_SMOOTHIE_CODE

		// Check if paused
		if (_isPaused)
			return;

		// Do a step-end for any motor which needs one - return here to avoid too short a pulse
		if (_motionIO.stepEnd())
			return;

		// Peek a MotionPipelineElem from the queue
		MotionBlock* pBlock = _motionPipeline.peekGet();
		if (!pBlock)
			return;

		// Check if the element is being changed
		if (pBlock->_changeInProgress)
			return;

		// Signal that we are now working on this block
		bool newBlock = !pBlock->_isRunning;
		pBlock->_isRunning = true;

		// Get current uS elapsed
		uint32_t curUs = micros();
		uint32_t usElapsed = curUs - _blockStartMicros;
		if (_blockStartMicros > curUs)
			usElapsed = 0xffffffff - _blockStartMicros + curUs;

		// New block
		if (newBlock)
		{ 
			// need to prepare each active axis direction
			for (uint8_t m = 0; m < RobotConsts::MAX_AXES; m++)
			{
				if (pBlock->_tickInfo[m].steps_to_move == 0) 
					continue;
				_motionIO.stepDirn(m, pBlock->_tickInfo[m]._stepDirection);
			}
			_blockStartMicros = curUs;
		}
		// Go through the axes
		bool still_moving = false;
		for (int m = 0; m < RobotConsts::MAX_AXES; m++)
		{
			// Get pointer to this axis
			volatile MotionBlock::tickinfo_t* pTickInfo = &(pBlock->_tickInfo[m]);

			if (pBlock->_tickInfo[m].steps_to_move == 0) 
				continue; // not active

			still_moving = true;

			pBlock->_tickInfo[m].steps_per_tick += pBlock->_tickInfo[m].acceleration_change;

			if (usElapsed == pBlock->_tickInfo[m].next_accel_event) {
				if (usElapsed == pBlock->_accelUntil) { // We are done accelerating, deceleration becomes 0 : plateau
					pBlock->_tickInfo[m].acceleration_change = 0;
					if (pBlock->_decelAfter < pBlock->_totalMoveTicks) {
						pBlock->_tickInfo[m].next_accel_event = pBlock->_decelAfter;
						if (usElapsed != pBlock->_decelAfter) { // We are plateauing
																				// steps/sec / tick frequency to get steps per tick
							pBlock->_tickInfo[m].steps_per_tick = pBlock->_tickInfo[m].plateau_rate;
						}
					}
				}

				if (usElapsed == pBlock->_decelAfter) { // We start decelerating
					pBlock->_tickInfo[m].acceleration_change = pBlock->_tickInfo[m].deceleration_change;
				}
			}

			// protect against rounding errors and such
			if (pBlock->_tickInfo[m].steps_per_tick <= 0) {
				pBlock->_tickInfo[m].counter = STEPTICKER_FPSCALE; // we force completion this step by setting to 1.0
				pBlock->_tickInfo[m].steps_per_tick = 0;
			}

			pBlock->_tickInfo[m].counter += pBlock->_tickInfo[m].steps_per_tick;

			if (pBlock->_tickInfo[m].counter >= STEPTICKER_FPSCALE) { // >= 1.0 step time
				pBlock->_tickInfo[m].counter -= STEPTICKER_FPSCALE; // -= 1.0F;
				++pBlock->_tickInfo[m].step_count;

				// step the motor
				_motionIO.stepStart(m);
				if (pBlock->_tickInfo[m].step_count == pBlock->_tickInfo[m].steps_to_move) 
				{
					// done
					pBlock->_tickInfo[m].steps_to_move = 0;
				}
			}
		}

		// see if any motors are still moving
		if (!still_moving) {

			// block complete
			_motionPipeline.remove();
		}
#endif
	}

	
	//void processRob()
	//{
	//	// Check if paused
	//	if (_isPaused)
	//		return;

	//	// Do a step-end for any motor which needs one - return here to avoid too short a pulse
	//	if (_motionIO.stepEnd())
	//		return;

	//	// Peek a MotionPipelineElem from the queue
	//	MotionPipelineElem* pElem = _motionPipeline.peekGet();
	//	if (!pElem)
	//		return;

	//	// Check if the element is being changed
	//	if (pElem->_changeInProgress)
	//		return;

	//	// Signal that we are now working on this block
	//	bool newBlock = !pElem->_isRunning;
	//	pElem->_isRunning = true;

	//	// Get current uS elapsed
	//	static uint32_t lastUs = micros();
	//	uint32_t curUs = micros();
	//	uint32_t usElapsed = curUs - lastUs;
	//	if (lastUs > curUs)
	//		usElapsed = 0xffffffff - lastUs + curUs;

	//	// Go through the axes
	//	bool allAxesDone = true;
	//	for (int axisIdx = 0; axisIdx < pElem->_numAxes; axisIdx++)
	//	{
	//		// Get pointer to this axis
	//		volatile MotionPipelineElem::tickinfo_t* pTickInfo = &(pElem->_tickInfo[axisIdx]);

	//		// Accumulate time elapsed since last step
	//		pTickInfo->_nsAccum += usElapsed * 1000;

	//		uint32_t nsToNextStep = pTickInfo->_nsToNextStep;
	//		// See if it is time for another step
	//		if (pTickInfo->_nsAccum > nsToNextStep)
	//		{
	//			// Avoid a rollover by checking for a signficant jump
	//			if (pTickInfo->_nsAccum > nsToNextStep + nsToNextStep)
	//				pTickInfo->_nsAccum = 0;
	//			else
	//				pTickInfo->_nsAccum -= nsToNextStep;
	//			// Set direction if not already set
	//			if (newBlock)
	//				_motionIO.stepDirn(axisIdx, pTickInfo->_stepDirection);
	//			// Start the step
	//			_motionIO.stepStart(axisIdx);
	//			// Count the steps
	//			pTickInfo->_curStepCount++;
	//			// Find the time for the next step
	//			if (pTickInfo->_curStepCount < pTickInfo->_accelSteps)
	//			{
	//				// We're accelerating
	//				allAxesDone = false;
	//				pTickInfo->_nsToNextStep -= pTickInfo->_accelReducePerStepNs;
	//			}
	//			else if (pTickInfo->_curStepCount < pTickInfo->_decelStartSteps)
	//			{
	//				// We're in the plateau
	//				allAxesDone = false;
	//				// No change to nsToNextStep
	//			}
	//			else if (pTickInfo->_curStepCount < pTickInfo->_totalSteps)
	//			{
	//				// We're in the deceleration phase
	//				allAxesDone = false;
	//				pTickInfo->_nsToNextStep += pTickInfo->_decelIncreasePerStepNs;
	//			}
	//			else
	//			{
	//				// We're done
	//			}
	//		}
	//	}

	//	// Check all axes done
	//	if (allAxesDone)
	//	{
	//		_motionPipeline.remove();
	//	}

	//	// Record last time
	//	lastUs = curUs;
	//}
};

