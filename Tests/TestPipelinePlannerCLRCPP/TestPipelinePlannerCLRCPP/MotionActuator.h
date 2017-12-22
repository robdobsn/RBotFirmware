#pragma once

#include "application.h"
#include "MotionPipeline.h"

class MotionActuator
{
private:
	volatile bool _isPaused;
	MotionPipeline _motionPipeline;

public:
	MotionActuator()
	{
		_isPaused = false;
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

		// Get current uS elapsed
		static uint32_t lastUs = micros();
		uint32_t curUs = micros();
		uint32_t elapsed = curUs - lastUs;
		if (lastUs > curUs)
			elapsed = 0xffffffff - lastUs + curUs;	// Go through axes

		for (int axisIdx = 0; axisIdx < pElem->_numAxes; axisIdx++)
		{
			// 

		}
	}
};

