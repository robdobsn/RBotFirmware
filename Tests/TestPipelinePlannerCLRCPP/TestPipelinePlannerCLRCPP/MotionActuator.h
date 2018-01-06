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

	// Test code
	TestMotionActuator* _pTestMotionActuator;

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
		_motionIO(motionIO),
		_pTestMotionActuator(NULL)
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

	void setTest(TestMotionActuator* pTestMotionActuator)
	{
		_pTestMotionActuator = pTestMotionActuator;
	}

	void config()
	{
	}

	void clear()
	{
		_isPaused = true;
	}

	void pause(bool pauseIt)
	{
		_isPaused = pauseIt;
	}

	void process();


private:
#ifdef USE_SPARK_INTERVAL_TIMER_ISR
	static void _isrStepperMotion(void);
#endif
	void procTick();

};
