#pragma once

#include "application.h"
#include "MotionPipeline.h"
#include "MotionIO.h"

class MotionActuator
{
public:
	static constexpr uint32_t TICK_INTERVAL_NS = 1000;

private:
	volatile bool _isPaused;
	MotionPipeline& _motionPipeline;
	MotionIO& _motionIO;
	unsigned long _blockStartMicros;

private:
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

public:
	MotionActuator(MotionIO& motionIO, MotionPipeline& motionPipeline) :
		_motionPipeline(motionPipeline),
		_motionIO(motionIO)
	{
		_isPaused = false;
		_blockStartMicros = 0;
	}

	void config()
	{

	}

	void clear()
	{
		_isPaused = false;
		_blockStartMicros = 0;
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
			for (uint8_t axisIdx = 0; RobotConsts::MAX_AXES; axisIdx++)
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
			axisExecInfo._curAccumulatorNs += TICK_INTERVAL_NS;

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
					for (int phaseIdx = axisExecInfo._axisStepPhaseNum+1; phaseIdx < MotionBlock::MAX_STEP_PHASES; phaseIdx++)
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

	void processSmoothie()
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

