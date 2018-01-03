#pragma once

#define USE_SPARK_INTERVAL_TIMER_ISR 1

#include "application.h"
#include "MotionPipeline.h"
#include "MotionIO.h"
#ifdef USE_SPARK_INTERVAL_TIMER_ISR
#include "SparkIntervalTimer.h"
#endif

class MotionActuator
{
private:
	volatile bool _isPaused;
	MotionPipeline& _motionPipeline;
	MotionIO& _motionIO;

	static uint32_t _testCount;

#ifdef USE_SPARK_INTERVAL_TIMER_ISR
	// ISR based interval timer
	static IntervalTimer _isrMotionTimer;
	static constexpr uint16_t ISR_TIMER_PERIOD_US = 50;
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

private:
	#ifdef USE_SPARK_INTERVAL_TIMER_ISR
		static void _isrStepperMotion(void);
	#endif

public:
	MotionActuator(MotionIO& motionIO, MotionPipeline& motionPipeline) :
		_motionPipeline(motionPipeline),
		_motionIO(motionIO)
	{
#ifdef USE_SPARK_INTERVAL_TIMER_ISR
		_isrMotionTimer.begin(_isrStepperMotion, ISR_TIMER_PERIOD_US, uSec);
#endif
		clear();
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

	void process()
	{
#ifndef USE_SPARK_INTERVAL_TIMER_ISR
	procBlock();
#endif
	}

	void procBlock()
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
};
