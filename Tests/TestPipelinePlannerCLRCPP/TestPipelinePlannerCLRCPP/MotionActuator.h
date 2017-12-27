#pragma once

#include "application.h"
#include "MotionPipeline.h"
#include "MotionIO.h"

class MotionActuator
{
private:
	volatile bool _isPaused;
	volatile bool _isRunning;
	MotionPipeline& _motionPipeline;
	MotionIO& _motionIO;
	unsigned long _blockStartMicros;

public:
	MotionActuator(MotionIO& motionIO, MotionPipeline& motionPipeline) :
		_motionPipeline(motionPipeline),
		_motionIO(motionIO)
	{
		_isPaused = false;
		_isRunning = false;
		_blockStartMicros = 0;
	}

	void config()
	{

	}

	void clear()
	{
		_isPaused = false;
		_isRunning = false;
		_blockStartMicros = 0;
	}

	void pause(bool pauseIt)
	{
		_isPaused = pauseIt;
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
		MotionPipelineElem* pElem = _motionPipeline.peekGet();
		if (!pElem)
			return;

		// Check if the element is being changed
		if (pElem->_changeInProgress)
			return;

		// Signal that we are now working on this block
		bool newBlock = !pElem->_isRunning;
		pElem->_isRunning = true;

		// Get current uS elapsed
		uint32_t curUs = micros();
		uint32_t usElapsed = curUs - _blockStartMicros;
		if (_blockStartMicros > curUs)
			usElapsed = 0xffffffff - _blockStartMicros + curUs;

		if (curUs == 449409)
		{
			printf("Here");
		}
		// New block
		if (newBlock)
		{ 
			// need to prepare each active axis direction
			for (uint8_t m = 0; m < pElem->_numAxes; m++)
			{
				if (pElem->_tickInfo[m].steps_to_move == 0) 
					continue;
				_motionIO.stepDirn(m, pElem->_tickInfo[m]._stepDirection);
			}
			_blockStartMicros = curUs;
		}
		// Go through the axes
		bool still_moving = false;
		for (int m = 0; m < pElem->_numAxes; m++)
		{
			// Get pointer to this axis
			volatile MotionPipelineElem::tickinfo_t* pTickInfo = &(pElem->_tickInfo[m]);

			if (pElem->_tickInfo[m].steps_to_move == 0) 
				continue; // not active

			still_moving = true;

			pElem->_tickInfo[m].steps_per_tick += pElem->_tickInfo[m].acceleration_change;

			if (usElapsed == pElem->_tickInfo[m].next_accel_event) {
				if (usElapsed == pElem->_accelUntil) { // We are done accelerating, deceleration becomes 0 : plateau
					pElem->_tickInfo[m].acceleration_change = 0;
					if (pElem->_decelAfter < pElem->_totalMoveTicks) {
						pElem->_tickInfo[m].next_accel_event = pElem->_decelAfter;
						if (usElapsed != pElem->_decelAfter) { // We are plateauing
																				// steps/sec / tick frequency to get steps per tick
							pElem->_tickInfo[m].steps_per_tick = pElem->_tickInfo[m].plateau_rate;
						}
					}
				}

				if (usElapsed == pElem->_decelAfter) { // We start decelerating
					pElem->_tickInfo[m].acceleration_change = pElem->_tickInfo[m].deceleration_change;
				}
			}

			// protect against rounding errors and such
			if (pElem->_tickInfo[m].steps_per_tick <= 0) {
				pElem->_tickInfo[m].counter = STEPTICKER_FPSCALE; // we force completion this step by setting to 1.0
				pElem->_tickInfo[m].steps_per_tick = 0;
			}

			pElem->_tickInfo[m].counter += pElem->_tickInfo[m].steps_per_tick;

			if (pElem->_tickInfo[m].counter >= STEPTICKER_FPSCALE) { // >= 1.0 step time
				pElem->_tickInfo[m].counter -= STEPTICKER_FPSCALE; // -= 1.0F;
				++pElem->_tickInfo[m].step_count;

				// step the motor
				_motionIO.stepStart(m);
				if (pElem->_tickInfo[m].step_count == pElem->_tickInfo[m].steps_to_move) 
				{
					// done
					pElem->_tickInfo[m].steps_to_move = 0;
				}
			}
		}

		// see if any motors are still moving
		if (!still_moving) {

			// block complete
			_motionPipeline.remove();
			_isRunning = false;
		}
	}

	void processRob()
	{
		// Check if paused
		if (_isPaused)
			return;

		// Do a step-end for any motor which needs one - return here to avoid too short a pulse
		if (_motionIO.stepEnd())
			return;

		// Peek a MotionPipelineElem from the queue
		MotionPipelineElem* pElem = _motionPipeline.peekGet();
		if (!pElem)
			return;

		// Check if the element is being changed
		if (pElem->_changeInProgress)
			return;

		// Signal that we are now working on this block
		bool newBlock = !pElem->_isRunning;
		pElem->_isRunning = true;

		// Get current uS elapsed
		static uint32_t lastUs = micros();
		uint32_t curUs = micros();
		uint32_t usElapsed = curUs - lastUs;
		if (lastUs > curUs)
			usElapsed = 0xffffffff - lastUs + curUs;

		// Go through the axes
		bool allAxesDone = true;
		for (int axisIdx = 0; axisIdx < pElem->_numAxes; axisIdx++)
		{
			// Get pointer to this axis
			volatile MotionPipelineElem::tickinfo_t* pTickInfo = &(pElem->_tickInfo[axisIdx]);

			// Accumulate time elapsed since last step
			pTickInfo->_nsAccum += usElapsed * 1000;

			uint32_t nsToNextStep = pTickInfo->_nsToNextStep;
			// See if it is time for another step
			if (pTickInfo->_nsAccum > nsToNextStep)
			{
				// Avoid a rollover by checking for a signficant jump
				if (pTickInfo->_nsAccum > nsToNextStep + nsToNextStep)
					pTickInfo->_nsAccum = 0;
				else
					pTickInfo->_nsAccum -= nsToNextStep;
				// Set direction if not already set
				if (newBlock)
					_motionIO.stepDirn(axisIdx, pTickInfo->_stepDirection);
				// Start the step
				_motionIO.stepStart(axisIdx);
				// Count the steps
				pTickInfo->_curStepCount++;
				// Find the time for the next step
				if (pTickInfo->_curStepCount < pTickInfo->_accelSteps)
				{
					// We're accelerating
					allAxesDone = false;
					pTickInfo->_nsToNextStep -= pTickInfo->_accelReducePerStepNs;
				}
				else if (pTickInfo->_curStepCount < pTickInfo->_decelStartSteps)
				{
					// We're in the plateau
					allAxesDone = false;
					// No change to nsToNextStep
				}
				else if (pTickInfo->_curStepCount < pTickInfo->_totalSteps)
				{
					// We're in the deceleration phase
					allAxesDone = false;
					pTickInfo->_nsToNextStep += pTickInfo->_decelIncreasePerStepNs;
				}
				else
				{
					// We're done
				}
			}
		}

		// Check all axes done
		if (allAxesDone)
		{
			_motionPipeline.remove();
		}

		// Record last time
		lastUs = curUs;
	}
};

