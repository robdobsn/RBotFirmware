// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

// #define USE_OLD_STEP_CALCS 1

#include "math.h"
#include "AxisValues.h"
#include "AxesParams.h"

class MotionBlock
{
public:
	// Step phases of each block: acceleration, plateau, deceleration
	static constexpr int STEP_PHASE_ACCEL = 0;
	static constexpr int STEP_PHASE_PLATEAU = 1;
	static constexpr int STEP_PHASE_DECEL = 2;

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

#ifdef USE_OLD_PLANNER_CODE
	void getMaxEntrySpeedFromExitSpeed(AxesParams& axesParams)
	{
		// If entry speed is already at the maximum entry speed, no need to recheck. Block is cruising.
		if (_entrySpeedMMps == _maxEntrySpeedMMps)
			return;

		// If not assume the block will be all deceleration and calculate the max speed we can enter to be able to slow
		// to the exit speed required
		float maxEntrySpeed = maxAchievableSpeed(axesParams._masterAxisMaxAccMMps2, _exitSpeedMMps, this->_moveDistPrimaryAxesMM);
		_entrySpeedMMps = fminf(maxEntrySpeed, _maxEntrySpeedMMps);
	}

	void setEntrySpeedFromPassedExitSpeed(float exitSpeed, AxesParams& axesParams)
	{
		// If entry speed is already at the maximum entry speed, no need to recheck. Block is cruising.
		if (_entrySpeedMMps == _maxEntrySpeedMMps)
			return;

		// If not assume the block will be all deceleration and calculate the max speed we can enter to be able to slow
		// to the exit speed required
		float maxEntrySpeed = maxAchievableSpeed(axesParams._masterAxisMaxAccMMps2, exitSpeed, this->_moveDistPrimaryAxesMM);
		_entrySpeedMMps = fminf(maxEntrySpeed, _maxEntrySpeedMMps);
	}

	float calcMaxSpeedForward(float prevMaxExitSpeed, AxesParams& axesParams)
	{
		// If the previous block is an acceleration block, but it is not long enough to complete the
		// full speed change within the block, we need to adjust the entry speed accordingly. Entry
		// speeds have already been reset, maximized, and reverse planned by reverse planner.
		// If nominal length is true, max junction speed is guaranteed to be reached. No need to recheck.
		if (prevMaxExitSpeed > _feedrateMMps)
			prevMaxExitSpeed = _feedrateMMps;
		if (prevMaxExitSpeed > _maxEntrySpeedMMps)
			prevMaxExitSpeed = _maxEntrySpeedMMps;
		if (prevMaxExitSpeed <= _entrySpeedMMps)
		{
			// We're acceleration limited
			_entrySpeedMMps = prevMaxExitSpeed;
			//_recalculateFlag = false;
		}
		// Now max out the exit speed
		maximizeExitSpeed(axesParams);
		return _exitSpeedMMps;
	}


	void maximizeExitSpeed(AxesParams& axesParams)
	{
		// If block is being executed then don't change
		if (_isExecuting)
			return;

		// If the flag is set showing that max junction speed can be reached regardless of entry/exit speed
		//if (_canReachJnMax)
		//{
		//	_exitSpeedMMps = _maxParamSpeedMMps;
		//	return;
		//}

		// Otherwise work out max exit speed based on entry and acceleration
		float maxExitSpeed = maxAchievableSpeed(axesParams._masterAxisMaxAccMMps2, _entrySpeedMMps, _moveDistPrimaryAxesMM);
		_exitSpeedMMps = fminf(maxExitSpeed, _exitSpeedMMps);
	}
#endif

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
			_axisStepData[axisIdx]._finalStepRatePerTTicks = 0;
			if (axisStepsTotal == 0)
				continue;

			// Calculate scaling factor for this axis
			float axisFactor = axisStepsTotal * oneOverAbsMaxStepsAnyAxis;

			// Start with the values from the previous block if there are any
			_axisStepData[axisIdx]._initialStepRatePerTTicks = prevBlockExitStepRatesPerTTicks.vals[axisIdx];

			// Reused values
			float axisAccStepsPerSec2 = axesParams.getMaxAccStepsPerSec2(axisIdx) * axisFactor;

			// Initial step rate per sec
			float initialStepRatePerSec = ((_axisStepData[axisIdx]._initialStepRatePerTTicks * 1.0f) * TICKS_PER_SEC) / TTICKS_VALUE;

			// Calculate final tick rate
			float finalStepRatePerSec = _exitSpeedMMps * axisFactor / axesParams.getStepDistMM(axisIdx);

			// Calculate the distance decelerating and ensure within bounds
			// Using the facts for the block ... (assuming max accleration followed by max deceleration):
			//		Vmax * Vmax = Ventry * Ventry + 2 * Amax * Saccelerating
			//		Vexit * Vexit = Vmax * Vmax - 2 * Amax * Sdecelerating
			//      Stotal = Saccelerating + Sdecelerating
			// And solving for Saccelerating (distance accelerating)
			uint32_t stepsAccelerating = 0;
			float stepsAcceleratingFloat = (powf(finalStepRatePerSec, 2) - powf(initialStepRatePerSec, 2)) / 4 / axisAccStepsPerSec2 + axisStepsTotal / 2;
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
			_axisStepData[axisIdx]._maxStepRatePerTTicks = uint32_t((axisMaxStepRatePerSec * TTICKS_VALUE) / TICKS_PER_SEC);
			_axisStepData[axisIdx]._finalStepRatePerTTicks = uint32_t((fmax(axesParams._minStepRatesPerSec.getVal(axisIdx), finalStepRatePerSec) * TTICKS_VALUE) / TICKS_PER_SEC);
			_axisStepData[axisIdx]._accStepsPerTTicksPerMS = uint32_t(axesParams.getMaxAccStepsPerTTicksPerMs(axisIdx, TTICKS_VALUE, TICKS_PER_SEC) * axisFactor);
			_axisStepData[axisIdx]._stepsBeforeDecel = axisStepsTotal - stepsDecelerating;

			int a = 1;
		}



		//	// Calculate the distance accelerating and ensure within bounds
		//	// Using the facts for the block ... (assuming max accleration followed by max deceleration):
		//	//		Vmax * Vmax = Ventry * Ventry + 2 * Amax * Saccelerating
		//	//		Vexit * Vexit = Vmax * Vmax - 2 * Amax * Sdecelerating
		//	//      Stotal = Saccelerating + Sdecelerating
		//	// And solving for Saccelerating (distance accelerating)
		//	float distAccelerating = (powf(_exitSpeedMMps, 2) - powf(_entrySpeedMMps, 2)) / 4 / axesParams._masterAxisMaxAccMMps2 + _moveDistPrimaryAxesMM / 2;
		//	if (distAccelerating < 0.0001f)
		//		distAccelerating = 0;
		//	forceInBounds(distAccelerating, 0, _moveDistPrimaryAxesMM);
		//	float distDecelerating = _moveDistPrimaryAxesMM - distAccelerating;
		//	float distPlateau = 0;

		//	// Check if max speed is reached
		//	float maxSpeedReachedMMps = _feedrateMMps;
		//	float distToMaxSpeed = (powf(_feedrateMMps, 2) - powf(_entrySpeedMMps, 2)) / 2 / axesParams._masterAxisMaxAccMMps2;
		//	if (distToMaxSpeed < distAccelerating)
		//	{
		//		// Max speed reached so we need to plateau
		//		distAccelerating = distToMaxSpeed;
		//		// Also need to recalculate distance decelerating
		//		distDecelerating = (powf(_feedrateMMps, 2) - powf(_exitSpeedMMps, 2)) / 2 / axesParams._masterAxisMaxAccMMps2;
		//		// And the plateau
		//		distPlateau = _moveDistPrimaryAxesMM - distAccelerating - distDecelerating;
		//	}
		//	else
		//	{
		//		maxSpeedReachedMMps = powf(_entrySpeedMMps, 2) + 2 * axesParams._masterAxisMaxAccMMps2
		//	}

		//// Calculate the initial step rate in steps per sec
		//float initialStepRatePerSec = _entrySpeedMMps / axesParams._masterAxisStepDistanceMM;

		//// Proportions of distance in each phase (acceleration, plateau, deceleration)
		//float distPropAccelerating = distAccelerating / _moveDistPrimaryAxesMM;
		//float distPropPlateau = distPlateau / _moveDistPrimaryAxesMM;

		//// Find the axisIdx with max steps and the max number of steps
		//uint32_t absMaxStepsForAnyAxis = 0;
		//int axisIdxWithMaxSteps = 0;
		//getAbsMaxStepsForAnyAxis(absMaxStepsForAnyAxis, axisIdxWithMaxSteps);
		//float oneOverAbsMaxStepsAnyAxis = 1.0f / absMaxStepsForAnyAxis;

		//// We are about to make changes to the block - only do this if the block is not currently executing
		//// So firstly indicate that execution should not start and then check it hasn't started - this avoids a
		//// potential race condition but only in one direction - this code can get interrupted by the ISR but
		//// the ISR will then complete before returning. So if the check indicates it isn't executing then it won't start
		//// executing afterwards until we set the _canExecute flag to true.
		//_canExecute = false;
		//if (_isExecuting)
		//	return;

		//// Now setup the values for the ticker to generate steps
		//for (uint8_t axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		//{

		//	// Calculate scaling factor for this axis
		//	uint32_t absStepsThisAxis = labs(getStepsToTarget(axisIdx));
		//	float axisFactor = absStepsThisAxis * oneOverAbsMaxStepsAnyAxis;

		//	// Step values
		//	uint32_t stepsAccel = uint32_t(ceilf(absStepsThisAxis * distPropAccelerating));
		//	uint32_t stepsPlateau = uint32_t(absStepsThisAxis * distPropPlateau);
		//	uint32_t stepsDecel = uint32_t(absStepsThisAxis - stepsAccel - stepsPlateau);

		//	// Axis max acceleration in units suitable for actuation
		//	float axisMaxAccStepsPerKTicksPerMilliSec = axesParams.getMaxAccStepsPerTTicksPerMs(axisIdx, TTICKS_VALUE, TICKS_PER_SEC);

		//	// If there is a deceleration phase and this isn't the dominant axis then correct acceleration to compensate for step number rounding
		//	if (stepsDecel > 0 && axisFactor < 0.3f)
		//	{
		//		axisMaxAccStepsPerKTicksPerMilliSec *= (float(stepsDecel) / (stepsDecel + 1));
		//	}

		//	// Initial step rate for this axis
		//	float axisInitialStepRatePerSec = initialStepRatePerSec * axisFactor;

		//	// Minimum step rate for this axis per KTicks
		//	float axisMinStepRatePerKTicks = (TTICKS_VALUE * axesParams._minStepRatesPerSec.getVal(axisIdx)) / TICKS_PER_SEC;

		//	// Don't allow the step rate to go below the acceleration achieved in 10ms
		//	if (axisMinStepRatePerKTicks < axisMaxAccStepsPerKTicksPerMilliSec * 10)
		//		axisMinStepRatePerKTicks = axisMaxAccStepsPerKTicksPerMilliSec * 10;

		//	// Initial step rate for this axis per KTicks
		//	float axisInitialStepRatePerKTicksFloat = (TTICKS_VALUE * axisInitialStepRatePerSec) / TICKS_PER_SEC;
		//	if (axisInitialStepRatePerKTicksFloat < axisMinStepRatePerKTicks)
		//		axisInitialStepRatePerKTicksFloat = axisMinStepRatePerKTicks;

		//	// Setup actuation data
		//	_axisStepData[axisIdx]._minStepRatePerKTicks = uint32_t(axisMinStepRatePerKTicks);
		//	_axisStepData[axisIdx]._initialStepRatePerKTicks = uint32_t(axisInitialStepRatePerKTicksFloat);
		//	_axisStepData[axisIdx]._accStepsPerKTicksPerMS = uint32_t(axisMaxAccStepsPerKTicksPerMilliSec);
		//	_axisStepData[axisIdx]._stepsInAccPhase = stepsAccel;
		//	_axisStepData[axisIdx]._stepsInPlateauPhase = stepsPlateau;
		//	_axisStepData[axisIdx]._stepsInDecelPhase = stepsDecel;
		//}

		// No more changes
		_canExecute = true;
	}

#ifdef USE_OLD_STEP_CALCS

	// Calculate trapezoid parameters so that the entry- and exit-speed is compensated by the provided factors.
	// The factors represent a factor of braking and must be in the range 0.0-1.0.
	//                                +--------+ <- nominal_rate
	//                               /          \
	// nominal_rate*entry_factor -> +            \
	//                              |             + <- nominal_rate*exit_factor
	//                              +-------------+
	//                                  time -->
	void calculateTrapezoid(AxesParams& axesParams)
	{
		// If block is currently being executed don't change it
		if (_isExecuting)
			return;

		// At this point we are ready to calculate the phases of motion for this block: acceleration, plateau, deceleration

		// First calculate the step rate (steps per sec) for entry
		float initialStepRatePerSec = _entrySpeedMMps / axesParams._masterAxisStepDistanceMM;

		// Calculate the distance accelerating and ensure within bounds
		// Using the facts for the block ... (assuming max accleration followed by max deceleration):
		//		Vmax * Vmax = Ventry * Ventry + 2 * Amax * Saccelerating
		//		Vexit * Vexit = Vmax * Vmax - 2 * Amax * Sdecelerating
		//      Stotal = Saccelerating + Sdecelerating
		// And solving for Saccelerating (distance accelerating)
		float distAccelerating = (powf(_exitSpeedMMps, 2) - powf(_entrySpeedMMps, 2)) / 4 / axesParams._masterAxisMaxAccMMps2 + _moveDistPrimaryAxesMM / 2;
		if (distAccelerating < 0.0001f)
			distAccelerating = 0;
		forceInBounds(distAccelerating, 0, _moveDistPrimaryAxesMM);
		float distDecelerating = _moveDistPrimaryAxesMM - distAccelerating;
		float distPlateau = 0;

		// Check if max speed is reached
		float distToMaxSpeed = (powf(_feedrateMMps, 2) - powf(_entrySpeedMMps, 2)) / 2 / axesParams._masterAxisMaxAccMMps2;
		if (distToMaxSpeed < distAccelerating)
		{
			// Max speed reached so we need to plateau
			distAccelerating = distToMaxSpeed;
			// Also need to recalculate distance decelerating
			distDecelerating = (powf(_feedrateMMps, 2) - powf(_exitSpeedMMps, 2)) / 2 / axesParams._masterAxisMaxAccMMps2;
			// And the plateau
			distPlateau = _moveDistPrimaryAxesMM - distAccelerating - distDecelerating;
		}

		// Proportions of distance in each phase (acceleration, plateau, deceleration)
		float distPropAccelerating = distAccelerating / _moveDistPrimaryAxesMM;
		float distPropPlateau = distPlateau / _moveDistPrimaryAxesMM;

		// Find the axisIdx with max steps and the max number of steps
		uint32_t absMaxStepsForAnyAxis = 0;
		int axisIdxWithMaxSteps = 0;
		getAbsMaxStepsForAnyAxis(absMaxStepsForAnyAxis, axisIdxWithMaxSteps);
		float oneOverAbsMaxStepsAnyAxis = 1.0f / absMaxStepsForAnyAxis;

		// We are about to make changes to the block - only do this if the block is not currently executing
		// So firstly indicate that execution should not start and then check it hasn't started - this avoids a
		// potential race condition but only in one direction - this code can get interrupted by the ISR but
		// the ISR will then complete before returning. So if the check indicates it isn't executing then it won't start
		// executing afterwards until we set the _canExecute flag to true.
		_canExecute = false;
		if (_isExecuting)
			return;

		// Now setup the values for the ticker to generate steps
		for (uint8_t axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{

			// Calculate scaling factor for this axis
			uint32_t absStepsThisAxis = labs(getStepsToTarget(axisIdx));
			float axisFactor = absStepsThisAxis * oneOverAbsMaxStepsAnyAxis;

			// Step values
			uint32_t stepsAccel = uint32_t(ceilf(absStepsThisAxis * distPropAccelerating));
			uint32_t stepsPlateau = uint32_t(absStepsThisAxis * distPropPlateau);
			uint32_t stepsDecel = uint32_t(absStepsThisAxis - stepsAccel - stepsPlateau);

			// Axis max acceleration in units suitable for actuation
			float axisMaxAccStepsPerKTicksPerMilliSec = axesParams.getMaxAccStepsPerTTicksPerMs(axisIdx, TTICKS_VALUE, TICKS_PER_SEC);

			// If there is a deceleration phase and this isn't the dominant axis then correct acceleration to compensate for step number rounding
			if (stepsDecel > 0 && axisFactor < 0.3f)
			{
				axisMaxAccStepsPerKTicksPerMilliSec *= (float(stepsDecel) / (stepsDecel + 1));
			}

			// Initial step rate for this axis
			float axisInitialStepRatePerSec = initialStepRatePerSec * axisFactor;

			// Minimum step rate for this axis per KTicks
			float axisMinStepRatePerKTicks = (TTICKS_VALUE * axesParams._minStepRatesPerSec.getVal(axisIdx)) / TICKS_PER_SEC;

			// Don't allow the step rate to go below the acceleration achieved in 10ms
			if (axisMinStepRatePerKTicks < axisMaxAccStepsPerKTicksPerMilliSec * 10)
				axisMinStepRatePerKTicks = axisMaxAccStepsPerKTicksPerMilliSec * 10;

			// Initial step rate for this axis per KTicks
			float axisInitialStepRatePerKTicksFloat = (TTICKS_VALUE * axisInitialStepRatePerSec) / TICKS_PER_SEC;
			if (axisInitialStepRatePerKTicksFloat < axisMinStepRatePerKTicks)
				axisInitialStepRatePerKTicksFloat = axisMinStepRatePerKTicks;

			// Setup actuation data
			_axisStepData[axisIdx]._minStepRatePerTTicks = uint32_t(axisMinStepRatePerKTicks);
			_axisStepData[axisIdx]._initialStepRatePerTTicks = uint32_t(axisInitialStepRatePerKTicksFloat);
			_axisStepData[axisIdx]._accStepsPerTTicksPerMS = uint32_t(axisMaxAccStepsPerKTicksPerMilliSec);
			_axisStepData[axisIdx]._stepsInAccPhase = stepsAccel;
			_axisStepData[axisIdx]._stepsInPlateauPhase = stepsPlateau;
			_axisStepData[axisIdx]._stepsInDecelPhase = stepsDecel;
		}
		// No more changes
		_canExecute = true;
	}
#endif

	void debugShowBlkHead()
	{
		Log.info("                   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY");
		Log.info("#i EntMMps ExtMMps  StTot  StDec    Init      Pk     Fin     Acc  StTot  StDec    Init      Pk     Fin     Acc");
	}

	void debugShowBlock(int elemIdx, AxesParams& axesParams)
	{
#ifdef USE_OLD_STEP_CALCS
		Log.info("%2d%8.3f%8.3f%6ld%6ld%10lu%8lu%6lu%6lu%6lu%10lu%8lu%6lu%6lu%6lu", elemIdx,
			_entrySpeedMMps, _exitSpeedMMps,
			getStepsToTarget(0), getStepsToTarget(1),
			_axisStepData[0]._initialStepRatePerKTicks, _axisStepData[0]._accStepsPerKTicksPerMS,
			_axisStepData[0]._stepsInAccPhase, _axisStepData[0]._stepsInPlateauPhase, _axisStepData[0]._stepsInDecelPhase,
			_axisStepData[1]._initialStepRatePerKTicks, _axisStepData[1]._accStepsPerKTicksPerMS,
			_axisStepData[1]._stepsInAccPhase, _axisStepData[1]._stepsInPlateauPhase, _axisStepData[1]._stepsInDecelPhase);
#else
		int32_t ax0St = getStepsToTarget(0);
		int32_t ax1St = getStepsToTarget(1);
		Log.info("%2d%8.3f%8.3f%7ld%7lu%8.3f%8.3f%8.3f%8lu%7ld%7lu%8.3f%8.3f%8.3f%8lu", elemIdx,
			_entrySpeedMMps, _exitSpeedMMps,
			ax0St, 
			(ax0St != 0) ? _axisStepData[0]._stepsBeforeDecel : 0,
			(ax0St != 0) ? (((_axisStepData[0]._initialStepRatePerTTicks * 1.0) * TICKS_PER_SEC) / TTICKS_VALUE) * axesParams.getStepDistMM(0) : 0,
			(ax0St != 0) ? (((_axisStepData[0]._maxStepRatePerTTicks * 1.0) * TICKS_PER_SEC) / TTICKS_VALUE) * axesParams.getStepDistMM(0) : 0,
			(ax0St != 0) ? (((_axisStepData[0]._finalStepRatePerTTicks * 1.0) * TICKS_PER_SEC) / TTICKS_VALUE) * axesParams.getStepDistMM(0) : 0,
			(ax0St != 0) ? _axisStepData[0]._accStepsPerTTicksPerMS : 0,
			ax1St,
			(ax1St != 0) ? _axisStepData[1]._stepsBeforeDecel : 0,
			(ax1St != 0) ? (((_axisStepData[1]._initialStepRatePerTTicks * 1.0) * TICKS_PER_SEC) / TTICKS_VALUE) * axesParams.getStepDistMM(0) : 0,
			(ax1St != 0) ? (((_axisStepData[1]._maxStepRatePerTTicks * 1.0) * TICKS_PER_SEC) / TTICKS_VALUE) * axesParams.getStepDistMM(0) : 0,
			(ax1St != 0) ? (((_axisStepData[1]._finalStepRatePerTTicks * 1.0) * TICKS_PER_SEC) / TTICKS_VALUE) * axesParams.getStepDistMM(0) : 0,
			(ax1St != 0) ? _axisStepData[1]._accStepsPerTTicksPerMS : 0);
#endif
	}

};
