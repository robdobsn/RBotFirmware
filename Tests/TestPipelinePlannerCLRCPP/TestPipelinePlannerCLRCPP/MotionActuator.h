// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

#ifdef SPARK
#define USE_SPARK_INTERVAL_TIMER_ISR 1
#endif

#include "application.h"
#include "MotionPipeline.h"
#include "MotionIO.h"
#ifdef USE_SPARK_INTERVAL_TIMER_ISR
#include "SparkIntervalTimer.h"
#endif
#include "TestMotionActuator.h"

class MotionActuator
{
private:
	volatile bool _isPaused;
	MotionPipeline& _motionPipeline;
	MotionIO& _motionIO;
	// This is to ensure that the robot never goes to 0 tick rate - which would leave it
	// immobile forever
	static constexpr uint32_t MIN_STEP_RATE_PER_SEC = 100;
	static constexpr uint32_t MIN_STEP_RATE_PER_TTICKS = uint32_t((MIN_STEP_RATE_PER_SEC * 1.0 * MotionBlock::TTICKS_VALUE) / MotionBlock::TICKS_PER_SEC);

#ifdef TEST_MOTION_ACTUATOR_ENABLE
	// Test code
	static TestMotionActuator* _pTestMotionActuator;
#endif

#ifdef USE_SPARK_INTERVAL_TIMER_ISR
	// ISR based interval timer
	static IntervalTimer _isrMotionTimer;
	static MotionActuator* _pMotionActuatorInstance;
	static constexpr uint16_t ISR_TIMER_PERIOD_US = uint16_t(MotionBlock::TICK_INTERVAL_NS / 1000l);
#endif

private:
	struct axisExecData_t
	{
		// Enabled flag
		bool _isEnabled;
		// True while axis is active - when all false block is complete
		bool _isActive;
		// Steps
		uint32_t _stepsTotalAbs;
		uint32_t _stepsBeforeDecel;
		uint32_t _curStepCount;
		// Max and final step rates
		uint32_t _maxStepRatePerTTicks;
		uint32_t _finalStepRatePerTTicks;
		// Current step rate (in steps per K ticks)
		uint32_t _curStepRatePerTTicks;
		// Acceleration rate
		uint32_t _accStepsPerTTicksPerMS;
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
		// Init
		clear();

		// If we are using the ISR then create the Spark Interval Timer and start it
#ifdef USE_SPARK_INTERVAL_TIMER_ISR
		if (_pMotionActuatorInstance == NULL)
		{
			_pMotionActuatorInstance = this;
			_isrMotionTimer.begin(_isrStepperMotion, ISR_TIMER_PERIOD_US, uSec);
		}
#endif
	}

	void setTestMode(const char* testModeStr)
	{
#ifdef TEST_MOTION_ACTUATOR_ENABLE
		_pTestMotionActuator = new TestMotionActuator();
		_pTestMotionActuator->setTestMode(testModeStr);
#endif
	}

	void config()
	{
	}

	void clear()
	{
		_isPaused = true;
#ifdef TEST_MOTION_ACTUATOR_ENABLE
		_pTestMotionActuator = NULL;
#endif
	}

	void pause(bool pauseIt)
	{
		_isPaused = pauseIt;
	}

	void process();

	void showDebug();

private:
#ifdef USE_SPARK_INTERVAL_TIMER_ISR
	static void _isrStepperMotion(void);
#endif
	void procTick();

};
