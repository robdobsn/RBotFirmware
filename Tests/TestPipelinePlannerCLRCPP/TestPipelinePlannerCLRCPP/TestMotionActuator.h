#pragma once

// Comment out these items to enable/disable test code
#define TEST_MOTION_ACTUATOR_ENABLE 1

#include "application.h"
#include <vector>
static constexpr int TEST_OUTPUT_STEPS = 1000;

#ifdef TEST_MOTION_ACTUATOR_ENABLE

class TestOutputStepData
{
public:
	struct TestOutputStepInf
	{
		uint32_t _micros;
		struct {
			int _pin : 8;
			int _val : 1;
		};
	};
	MotionRingBufferPosn _stepBufPos;
	std::vector<TestOutputStepInf> _stepBuf;

	TestOutputStepData() :
		_stepBufPos(TEST_OUTPUT_STEPS)
	{
		_stepBuf.resize(TEST_OUTPUT_STEPS);
	}

	void stepStart(int axisIdx)
	{
		addToQueue(axisIdx == 0 ? 2 : 4, 1);
	}

	void stepEnd()
	{
	}

	void stepDirn(int axisIdx, int val)
	{
		addToQueue(axisIdx == 0 ? 3 : 5, val);
	}

	void addToQueue(int pin, int val)
	{
		// Ignore if it is a lowering of a step pin (to avoid end of test problem)
		if ((val == 0) && (pin == 2 || pin == 4))
			return;
		if (_stepBufPos.canPut())
		{
			TestOutputStepInf newInf;
			newInf._micros = micros();
			newInf._pin = uint8_t(pin);
			newInf._val = val;
			_stepBuf[_stepBufPos._putPos] = newInf;
			_stepBufPos.hasPut();
		}
	}

	TestOutputStepInf getStepInf()
	{
		TestOutputStepInf inf = _stepBuf[_stepBufPos._getPos];
		_stepBufPos.hasGot();
		return inf;
	}

	void process()
	{
		// Log.trace("StepBuf getPos %d putPos %d count %d", _stepBufPos._getPos, _stepBufPos._putPos, _stepBufPos.count());

		// Get
		for (int i = 0; i < 5; i++)
		{
			// Check if can get
			if (!_stepBufPos.canGet())
			{
				// Log.trace("Process can't get");
				return;
			}

			TestOutputStepInf inf = getStepInf();
			Log.trace("W\t%lu\t%d\t%d", inf._micros, inf._pin, inf._val ? 1 : 0);
		}
	}
};
#endif

class TestMotionActuator
{
public:
	bool _outputStepData;
	bool _blinkD7OnISR;
	int _pinNum;

	TestMotionActuator(bool outputStepData, bool blinkD7OnISR)
	{
		_blinkD7OnISR = blinkD7OnISR;
		_outputStepData = outputStepData;
		if (_blinkD7OnISR)
		{
			_pinNum = ConfigPinMap::getPinFromName("D7");
			pinMode(_pinNum, OUTPUT);
		}
	}
#ifdef TEST_MOTION_ACTUATOR_ENABLE
	TestOutputStepData _testOutputStepData;
#endif
	static uint32_t _testCount;

	void process()
	{
#ifdef TEST_MOTION_ACTUATOR_ENABLE
		_testOutputStepData.process();
#endif
	}

	void blink()
	{
		uint32_t blinkRate = 10000;
		_testCount++;
		if (_testCount > blinkRate)
		{
			_testCount = 0;
			digitalWrite(_pinNum, !digitalRead(_pinNum));
		}
	}

	void stepStart(int axisIdx)
	{
#ifdef TEST_MOTION_ACTUATOR_ENABLE
		_testOutputStepData.stepStart(axisIdx);
#endif
	}

	void stepEnd()
	{
#ifdef TEST_MOTION_ACTUATOR_ENABLE
		_testOutputStepData.stepEnd();
#endif
	}

	void stepDirn(int axisIdx, int val)
	{
#ifdef TEST_MOTION_ACTUATOR_ENABLE
		_testOutputStepData.stepDirn(axisIdx, val);
#endif
	}

};
