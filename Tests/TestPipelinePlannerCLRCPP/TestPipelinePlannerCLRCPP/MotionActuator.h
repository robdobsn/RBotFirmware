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
		uint32_t elapsed = curUs - lastUs;
		if (lastUs > curUs)
			elapsed = 0xffffffff - lastUs + curUs;	// Go through axes

		// Go through the axes
		for (int axisIdx = 0; axisIdx < pElem->_numAxes; axisIdx++)
		{
			// Get pointer to this axis
			volatile MotionPipelineElem::tickinfo_t* pTickInfo = &(pElem->_tickInfo[axisIdx]);

			// Check if the step pin is already in active state - if so lower
			if (_motionIO.stepEnd(axisIdx))
				continue;

			// Accumulate time elapsed since last step
			pTickInfo->_usAccum += elapsed;
//			uint32_t stepUs = pAxisVars->_stepUs[qPos];
//			// See if it is time for another step
//			if (pAxisVars->_usAccum > stepUs)
//			{
//				if (pAxisVars->_usAccum > stepUs + stepUs)
//					pAxisVars->_usAccum = 0;
//				else
//					pAxisVars->_usAccum -= stepUs;
//				// Step (and set pin value so that it completes pulse on next ISR call)
//				digitalWrite(pAxisVars->_stepPin, 1);
//				pAxisVars->_stepPinValue = 1;
//				// Count the steps and position from last zero
//				pAxisVars->_stepCount++;
//				pAxisVars->_stepsFromLastZero +=
//					(pAxisVars->_stepDirn[qPos] ? 1 : -1);
//				// Check if we have completed the pipeline block
//				if (pAxisVars->_stepCount >=
//					pAxisVars->_stepNum[qPos])
//				{
//					pAxisVars->_isActive = false;
//				}
//				else
//				{
//#ifdef DEBUG_TIME_ISR_JITTER
//					// Check timing
//					if (pAxisVars->_dbgLastStepUsValid)
//					{
//						uint32_t betweenStepsUs = curUs - pAxisVars->_dbgLastStepUs;
//						if (pAxisVars->_dbgLastStepUs > curUs)
//							betweenStepsUs = 0xffffffff - pAxisVars->_dbgLastStepUs + curUs;
//						if (pAxisVars->_dbgMaxStepUs < betweenStepsUs)
//							pAxisVars->_dbgMaxStepUs = betweenStepsUs;
//						if (pAxisVars->_dbgMinStepUs > betweenStepsUs)
//							pAxisVars->_dbgMinStepUs = betweenStepsUs;
//					}
//					// Record last time
//					pAxisVars->_dbgLastStepUs = curUs;
//					pAxisVars->_dbgLastStepUsValid = true;
//#endif
//				}
//			}

		}

		// Record last time
		lastUs = curUs;
	}
};

