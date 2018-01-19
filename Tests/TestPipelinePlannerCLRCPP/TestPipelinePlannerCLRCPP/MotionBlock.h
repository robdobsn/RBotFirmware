// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

//#define USE_OLD_WAY_FOR_ENTRY_SPEED 1

#include "math.h"
#include "AxisValues.h"
#include "AxesParams.h"

#define DEBUG_STEP_TTICKS_TO_MMPS(val, axesParams, axisIdx) (((val * 1.0) * MotionBlock::TICKS_PER_SEC) / MotionBlock::TTICKS_VALUE) * axesParams.getStepDistMM(axisIdx)

class MotionBlock
{
public:
	// Minimum move distance
	static constexpr double MINIMUM_MOVE_DIST_MM = 0.0001;

	// Number of ticks to accumulate for rate actuation
	static constexpr uint32_t TTICKS_VALUE = 1000000000l;

	// Tick interval in NS
	// 20000NS means max of 50k steps per second
	static constexpr uint32_t TICK_INTERVAL_NS = 20000;
	static constexpr float TICKS_PER_SEC = (1e9f / TICK_INTERVAL_NS);

	// Number of ns in ms
	static constexpr uint32_t NS_IN_A_MS = 1000000;

public:
	// Max speed for move (maybe reduced by feedrate in a GCode command)
	float _feedrateMMps;
	// Distance (pythagorean) to move considering primary axes only
	float _moveDistPrimaryAxesMM;
	// Computed max entry speed for a block based on max junction deviation calculation
	float _maxEntrySpeedMMps;
	// Computed entry speed for this block
	float _entrySpeedMMps;
	// Computed exit speed for this block
	float _exitSpeedMMps;

	// Flags
	struct
	{
		// Flag indicating the block is currently executing
		volatile bool _isExecuting : 1;
		// Flag indicating the block can start executing
		volatile bool _canExecute : 1;
	};

	struct axisStepData_t
	{
		uint32_t _initialStepRatePerTTicks;
		uint32_t _maxStepRatePerTTicks;
		uint32_t _finalStepRatePerTTicks;
		uint32_t _accStepsPerTTicksPerMS;
		uint32_t _stepsBeforeDecel;
		int32_t _stepsTotalMaybeNeg;
	};
	axisStepData_t _axisStepData[RobotConsts::MAX_AXES];

public:
	MotionBlock()
	{
		clear();
	}

	void clear()
	{
		// Clear values
		_feedrateMMps = 0;
		_moveDistPrimaryAxesMM = 0;
		_maxEntrySpeedMMps = 0;
		_entrySpeedMMps = 0;
		_exitSpeedMMps = 0;
		_isExecuting = false;
		_canExecute = false;
	}

	int32_t getStepsToTarget(int axisIdx)
	{
		if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
		{
			return _axisStepData[axisIdx]._stepsTotalMaybeNeg;
		}
		return 0;
	}

	int32_t getAbsStepsToTarget(int axisIdx)
	{
		if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
		{
			return abs(_axisStepData[axisIdx]._stepsTotalMaybeNeg);
		}
		return 0;
	}

	void setStepsToTarget(int axisIdx, int32_t steps)
	{
		if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
		{
			_axisStepData[axisIdx]._stepsTotalMaybeNeg = steps;
		}
	}

	void getAbsMaxStepsForAnyAxis(uint32_t& axisAbsMaxSteps, int &axisIdxWithMaxSteps)
	{
		axisAbsMaxSteps = 0;
		axisIdxWithMaxSteps = 0;
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			if (axisAbsMaxSteps < uint32_t(getAbsStepsToTarget(axisIdx)))
			{
				axisAbsMaxSteps = getAbsStepsToTarget(axisIdx);
				axisIdxWithMaxSteps = axisIdx;
			}
		}
	}

	void getExitStepRatesPerTTicks(AxisInt32s& exitStepRates)
	{
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			exitStepRates.vals[axisIdx] = _axisStepData[axisIdx]._finalStepRatePerTTicks;
		}
	}

	float maxAchievableSpeed(float acceleration, float target_velocity, float distance)
	{
		return sqrtf(target_velocity * target_velocity + 2.0F * acceleration * distance);
	}
	void forceInBounds(float& val, float lowBound, float highBound)
	{
		if (val < lowBound)
			val = lowBound;
		if (val > highBound)
			val = highBound;
	}

	// The block's entry and exit speed are now known
	// The block can accelerate and decelerate as required as long as these criteria are met
	// We now compute the stepping parameters to make motion happen
	void prepareForStepping(AxesParams& axesParams, AxisInt32s& prevBlockExitStepRatesPerTTicks)
	{
		// If block is currently being executed don't change it
		if (_isExecuting)
			return;

		// Find the axisIdx with max steps and the max number of steps
		uint32_t absMaxStepsForAnyAxis = 0;
		int axisIdxWithMaxSteps = 0;
		getAbsMaxStepsForAnyAxis(absMaxStepsForAnyAxis, axisIdxWithMaxSteps);
		float oneOverAbsMaxStepsAnyAxis = 1.0f / absMaxStepsForAnyAxis;

		// Generate values for each axis
		for (uint8_t axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			// Check for no steps - but need to be careful to set final step rate to 0 as it is used as entry value for next block
			uint32_t axisStepsTotal = abs(_axisStepData[axisIdx]._stepsTotalMaybeNeg);
#ifndef USE_OLD_WAY_FOR_ENTRY_SPEED
			_axisStepData[axisIdx]._finalStepRatePerTTicks = 0;
#endif
			if (axisStepsTotal == 0)
				continue;

			// Calculate scaling factor for this axis
			float axisFactor = axisStepsTotal * oneOverAbsMaxStepsAnyAxis;

#ifndef USE_OLD_WAY_FOR_ENTRY_SPEED
			// Start with the values from the previous block if there are any
			_axisStepData[axisIdx]._initialStepRatePerTTicks = prevBlockExitStepRatesPerTTicks.vals[axisIdx];

			// Initial step rate per sec
			float initialStepRatePerSec = ((_axisStepData[axisIdx]._initialStepRatePerTTicks * 1.0f) * TICKS_PER_SEC) / TTICKS_VALUE;
#else
			float initialStepRatePerSec = _entrySpeedMMps * axisFactor / axesParams.getStepDistMM(axisIdx);
#endif

			// Calculate final tick rate
			float finalStepRatePerSec = _exitSpeedMMps * axisFactor / axesParams.getStepDistMM(axisIdx);

			// Reused values
			float axisAccStepsPerSec2 = axesParams.getMaxAccStepsPerSec2(axisIdx) * axisFactor;

			// Calculate the distance decelerating and ensure within bounds
			// Using the facts for the block ... (assuming max accleration followed by max deceleration):
			//		Vmax * Vmax = Ventry * Ventry + 2 * Amax * Saccelerating
			//		Vexit * Vexit = Vmax * Vmax - 2 * Amax * Sdecelerating
			//      Stotal = Saccelerating + Sdecelerating
			// And solving for Saccelerating (distance accelerating)
			uint32_t stepsAccelerating = 0;
			float stepsAcceleratingFloat = ceilf((powf(finalStepRatePerSec, 2) - powf(initialStepRatePerSec, 2)) / 4 / axisAccStepsPerSec2 + axisStepsTotal / 2);
			if (stepsAcceleratingFloat > 0)
			{
				stepsAccelerating = uint32_t(stepsAcceleratingFloat);
				if (stepsAccelerating > axisStepsTotal)
					stepsAccelerating = axisStepsTotal;
			}

			// Decelerating steps
			uint32_t stepsDecelerating = axisStepsTotal - stepsAccelerating;

			// Find max possible rate for this axis
			float axisMaxStepRatePerSec = _feedrateMMps * axisFactor / axesParams.getStepDistMM(axisIdx);

			// See if max speed will be reached
			uint32_t stepsToMaxSpeed = uint32_t((powf(axisMaxStepRatePerSec,2) - powf(initialStepRatePerSec, 2)) / 2 / axisAccStepsPerSec2);
			if (stepsAccelerating > stepsToMaxSpeed)
			{
				// Max speed will be reached
				stepsAccelerating = stepsToMaxSpeed;

				// Decelerating steps
				stepsDecelerating = uint32_t((powf(axisMaxStepRatePerSec, 2) - powf(finalStepRatePerSec, 2)) / 2 / axisAccStepsPerSec2);
			}
			else
			{
				// Calculate max speed that will be reached
				axisMaxStepRatePerSec = sqrtf(powf(initialStepRatePerSec, 2) + 2.0F * axisAccStepsPerSec2 * stepsAccelerating);
			}

			// Fill in the step values for this axis
#ifdef USE_OLD_WAY_FOR_ENTRY_SPEED
			_axisStepData[axisIdx]._initialStepRatePerTTicks = uint32_t((initialStepRatePerSec * TTICKS_VALUE) / TICKS_PER_SEC);
#endif
			_axisStepData[axisIdx]._maxStepRatePerTTicks = uint32_t((axisMaxStepRatePerSec * TTICKS_VALUE) / TICKS_PER_SEC);
			_axisStepData[axisIdx]._finalStepRatePerTTicks = uint32_t((fmax(axesParams._minStepRatesPerSec.getVal(axisIdx), finalStepRatePerSec) * TTICKS_VALUE) / TICKS_PER_SEC);
			_axisStepData[axisIdx]._accStepsPerTTicksPerMS = uint32_t(axesParams.getMaxAccStepsPerTTicksPerMs(axisIdx, TTICKS_VALUE, TICKS_PER_SEC) * axisFactor);
			_axisStepData[axisIdx]._stepsBeforeDecel = axisStepsTotal - stepsDecelerating;
		}

		// No more changes
		_canExecute = true;
	}

	void debugShowBlkHead()
	{
		Log.info("                   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY");
		Log.info("#i EntMMps ExtMMps  StTot  StDec    Init      Pk     Fin     Acc  StTot  StDec    Init      Pk     Fin     Acc");
	}

	void debugGetAxisStepInfoCleaned(axisStepData_t axisStepData[RobotConsts::MAX_AXES], AxesParams& axesParams)
	{
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			int32_t axSt = getStepsToTarget(axisIdx);
			axisStepData[axisIdx]._stepsTotalMaybeNeg = _axisStepData[axisIdx]._stepsTotalMaybeNeg;
			axisStepData[axisIdx]._stepsBeforeDecel = (axSt != 0) ? _axisStepData[axisIdx]._stepsBeforeDecel : 0;
			axisStepData[axisIdx]._initialStepRatePerTTicks = (axSt != 0) ? _axisStepData[axisIdx]._initialStepRatePerTTicks : 0;
			axisStepData[axisIdx]._maxStepRatePerTTicks = (axSt != 0) ? _axisStepData[axisIdx]._maxStepRatePerTTicks : 0;
			axisStepData[axisIdx]._finalStepRatePerTTicks = (axSt != 0) ? _axisStepData[axisIdx]._finalStepRatePerTTicks : 0;
			axisStepData[axisIdx]._accStepsPerTTicksPerMS = (axSt != 0) ? _axisStepData[axisIdx]._accStepsPerTTicksPerMS : 0;
		}
	}
	void debugShowBlock(int elemIdx, AxesParams& axesParams)
	{
		axisStepData_t axisStepData[RobotConsts::MAX_AXES];
		debugGetAxisStepInfoCleaned(axisStepData, axesParams);
		Log.info("%2d%8.3f%8.3f%7ld%7lu%8.3f%8.3f%8.3f%8lu%7ld%7lu%8.3f%8.3f%8.3f%8lu", elemIdx,
			_entrySpeedMMps, _exitSpeedMMps,
			getStepsToTarget(0),
			axisStepData[0]._stepsBeforeDecel,
			DEBUG_STEP_TTICKS_TO_MMPS(axisStepData[0]._initialStepRatePerTTicks, axesParams, 0),
			DEBUG_STEP_TTICKS_TO_MMPS(axisStepData[0]._maxStepRatePerTTicks, axesParams, 0),
			DEBUG_STEP_TTICKS_TO_MMPS(axisStepData[0]._finalStepRatePerTTicks, axesParams, 0),
			axisStepData[0]._accStepsPerTTicksPerMS,
			getStepsToTarget(1),
			axisStepData[1]._stepsBeforeDecel,
			DEBUG_STEP_TTICKS_TO_MMPS(axisStepData[1]._initialStepRatePerTTicks, axesParams, 1),
			DEBUG_STEP_TTICKS_TO_MMPS(axisStepData[1]._maxStepRatePerTTicks, axesParams, 1),
			DEBUG_STEP_TTICKS_TO_MMPS(axisStepData[1]._finalStepRatePerTTicks, axesParams, 1),
			axisStepData[1]._accStepsPerTTicksPerMS);
	}

};
