// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

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
	static constexpr uint32_t K_VALUE = 1000000000l;

	// Tick interval in NS
	// 20000NS means max of 50k steps per second
	static constexpr uint32_t TICK_INTERVAL_NS = 20000;

	// Number of ns in ms
	static constexpr uint32_t NS_IN_A_MS = 1000000;

public:
	// Max speed set for master axis (maybe reduced by feedrate in a GCode command)
	float _maxParamSpeedMMps;
	// Steps required for each axis
	AxisInt32s _axisStepsToTarget;
	// Distance (pythagorean) to move considering primary axes only
	float _moveDistPrimaryAxesMM;
	// Computed max entry speed for a block based on max junction deviation calculation
	float _maxEntrySpeedMMps;
	// Computed entry speed for this block
	float _entrySpeedMMps;
	// Computed exit speed for this block
	float _exitSpeedMMps;
	// Unit vectors for movement
	AxisFloats unitVectors;

	// Flags
	struct
	{
	// Flag indicating the block is currently executing
		volatile bool _isExecuting : 1;
	// Flag indicating the block can start executing
		volatile bool _canExecute : 1;
		// Flag indicating that this block need not be recalculated
		bool _recalculateFlag : 1;
		// Block can reach junction max speed regardless of entry/exit speed
		bool _canReachJnMax : 1;
	};

	struct axisStepData_t
	{
		uint32_t _minStepRatePerKTicks;
		uint32_t _initialStepRatePerKTicks;
		uint32_t _accStepsPerKTicksPerMS;
		uint32_t _stepsInAccPhase;
		uint32_t _stepsInPlateauPhase;
		uint32_t _stepsInDecelPhase;
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
		_maxParamSpeedMMps = 0;
		_moveDistPrimaryAxesMM = 0;
		_maxEntrySpeedMMps = 0;
		_entrySpeedMMps = 0;
		_exitSpeedMMps = 0;
		_isExecuting = false;
		_canExecute = false;
		_recalculateFlag = true;
		_canReachJnMax = false;
	}

	void getAbsMaxStepsForAnyAxis(uint32_t& axisAbsMaxSteps, int &axisIdxWithMaxSteps)
	{
		axisAbsMaxSteps = 0;
		axisIdxWithMaxSteps = 0;
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			if (axisAbsMaxSteps < uint32_t(labs(_axisStepsToTarget.vals[axisIdx])))
			{
				axisAbsMaxSteps = labs(_axisStepsToTarget.vals[axisIdx]);
				axisIdxWithMaxSteps = axisIdx;
			}
		}
	}

	float calcMaxSpeedReverse(float exitSpeed, AxesParams& axesParams)
	{
		// If entry speed is already at the maximum entry speed, no need to recheck. Block is cruising.
		// If not, block in state of acceleration or deceleration. Reset entry speed to maximum and
		// check for maximum allowable speed reductions to ensure maximum possible planned speed.
		if (_entrySpeedMMps != _maxEntrySpeedMMps)
		{
			if (!_canReachJnMax  && (_maxEntrySpeedMMps > exitSpeed))
			{
				float maxEntrySpeed = maxAllowableSpeed(axesParams._masterAxisMaxAccMMps2, exitSpeed, this->_moveDistPrimaryAxesMM);
				_entrySpeedMMps = fminf(maxEntrySpeed, _maxEntrySpeedMMps);
			}
			else
			{
				_entrySpeedMMps = _maxEntrySpeedMMps;
			}
		}
		return _entrySpeedMMps;
	}

	void calcMaxSpeedForward(float prevMaxExitSpeed, AxesParams& axesParams)
	{
		// If the previous block is an acceleration block, but it is not long enough to complete the
		// full speed change within the block, we need to adjust the entry speed accordingly. Entry
		// speeds have already been reset, maximized, and reverse planned by reverse planner.
		// If nominal length is true, max junction speed is guaranteed to be reached. No need to recheck.
		if (prevMaxExitSpeed > _maxParamSpeedMMps)
			prevMaxExitSpeed = _maxParamSpeedMMps;
		if (prevMaxExitSpeed > _maxEntrySpeedMMps)
			prevMaxExitSpeed = _maxEntrySpeedMMps;
		if (prevMaxExitSpeed <= _entrySpeedMMps)
		{
			// We're acceleration limited
			_entrySpeedMMps = prevMaxExitSpeed;
			_recalculateFlag = false;
		}
		// Now max out the exit speed
		maximizeExitSpeed(axesParams);
	}

	float maxAllowableSpeed(float acceleration, float target_velocity, float distance)
	{
		return sqrtf(target_velocity * target_velocity + 2.0F * acceleration * distance);
	}

	void maximizeExitSpeed(AxesParams& axesParams)
	{
		// If block is being executed then don't change
		if (_isExecuting)
			return;

		// If the flag is set showing that max junction speed can be reached regardless of entry/exit speed
		if (_canReachJnMax)
		{
			//_exitSpeedMMps = _maxParamSpeedMMps;
			return;
		}

		// Otherwise work out max exit speed based on entry and acceleration
		float maxExitSpeed = maxAllowableSpeed(axesParams._masterAxisMaxAccMMps2, _entrySpeedMMps, _moveDistPrimaryAxesMM);
		_exitSpeedMMps = fminf(maxExitSpeed, _exitSpeedMMps);
	}

	void forceInBounds(float& val, float lowBound, float highBound)
	{
		if (val < lowBound)
			val = lowBound;
		if (val > highBound)
			val = highBound;
	}

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
		forceInBounds(distAccelerating, 0, _moveDistPrimaryAxesMM);
		float distDecelerating = _moveDistPrimaryAxesMM - distAccelerating;
		float distPlateau = 0;

		// Check if max speed is reached
		float distToMaxSpeed = (powf(_maxParamSpeedMMps, 2) - powf(_entrySpeedMMps, 2)) / 2 / axesParams._masterAxisMaxAccMMps2;
		if (distToMaxSpeed < distAccelerating)
		{
			// Max speed reached so we need to plateau
			distAccelerating = distToMaxSpeed;
			// Also need to recalculate distance decelerating
			distDecelerating = (powf(_maxParamSpeedMMps, 2) - powf(_exitSpeedMMps, 2)) / 2 / axesParams._masterAxisMaxAccMMps2;
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

		// KTicks
		float ticksPerSec = 1e9 / TICK_INTERVAL_NS;

		// Master axis acceleration in steps per KTicks per millisecond
		float masterAxisMaxAccStepsPerSec2 = axesParams._masterAxisMaxAccMMps2 / axesParams._masterAxisStepDistanceMM;
		float masterAxisMaxAccStepsPerKTicksPerSec = (K_VALUE * masterAxisMaxAccStepsPerSec2) / ticksPerSec;
		float masterAxisMaxAccStepsPerKTicksPerMilliSec = masterAxisMaxAccStepsPerKTicksPerSec / 1000;

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
			uint32_t absStepsThisAxis = labs(_axisStepsToTarget.vals[axisIdx]);
			float axisFactor = absStepsThisAxis * oneOverAbsMaxStepsAnyAxis;

			// Step values
			uint32_t stepsAccel = uint32_t(ceilf(absStepsThisAxis * distPropAccelerating));
			uint32_t stepsPlateau = uint32_t(absStepsThisAxis * distPropPlateau);
			uint32_t stepsDecel = uint32_t(absStepsThisAxis - stepsAccel - stepsPlateau);

			// Axis max acceleration in units suitable for actuation
			float axisMaxAccStepsPerKTicksPerMilliSec = masterAxisMaxAccStepsPerKTicksPerMilliSec * axisFactor;

			// If there is a deceleration phase and this isn't the dominant axis then correct acceleration to compensate for step number rounding
			if (stepsDecel > 0 && axisFactor < 0.3f)
			{
				axisMaxAccStepsPerKTicksPerMilliSec *= (float(stepsDecel) / (stepsDecel + 1));
			}

			// Initial step rate for this axis
			float axisInitialStepRatePerSec = initialStepRatePerSec * axisFactor;

			// Minimum step rate for this axis per KTicks
			float axisMinStepRatePerKTicks = (K_VALUE * axesParams._minStepRatesPerSec.getVal(axisIdx)) / ticksPerSec;

			// Don't allow the step rate to go below the acceleration achieved in 10ms
			if (axisMinStepRatePerKTicks < axisMaxAccStepsPerKTicksPerMilliSec * 10)
				axisMinStepRatePerKTicks = axisMaxAccStepsPerKTicksPerMilliSec * 10;

			// Initial step rate for this axis per KTicks
			float axisInitialStepRatePerKTicksFloat = (K_VALUE * axisInitialStepRatePerSec) / ticksPerSec;
			if (axisInitialStepRatePerKTicksFloat < axisMinStepRatePerKTicks)
				axisInitialStepRatePerKTicksFloat = axisMinStepRatePerKTicks;

			// Setup actuation data
			_axisStepData[axisIdx]._minStepRatePerKTicks = uint32_t(axisMinStepRatePerKTicks);
			_axisStepData[axisIdx]._initialStepRatePerKTicks = uint32_t(axisInitialStepRatePerKTicksFloat);
			_axisStepData[axisIdx]._accStepsPerKTicksPerMS = uint32_t(axisMaxAccStepsPerKTicksPerMilliSec);
			_axisStepData[axisIdx]._stepsInAccPhase = stepsAccel;
			_axisStepData[axisIdx]._stepsInPlateauPhase = stepsPlateau;
			_axisStepData[axisIdx]._stepsInDecelPhase = stepsDecel;
		}
		// No more changes
		_canExecute = true;
	}

	void debugShowBlkHead()
	{
		Log.info("#i EntMMps ExtMMps XStps YStps   XStPKtk XAcPKms XAcSt XPlSt XDcSt   YStPKtk YAcPKms YAcSt YPlSt YDcSt");
	}

	void debugShowBlock(int elemIdx)
	{
		Log.info("%2d%8.3f%8.3f%6ld%6ld%10lu%8lu%6lu%6lu%6lu%10lu%8lu%6lu%6lu%6lu", elemIdx,
			_entrySpeedMMps, _exitSpeedMMps,
			_axisStepsToTarget.X(), _axisStepsToTarget.Y(),
			_axisStepData[0]._initialStepRatePerKTicks, _axisStepData[0]._accStepsPerKTicksPerMS,
			_axisStepData[0]._stepsInAccPhase, _axisStepData[0]._stepsInPlateauPhase, _axisStepData[0]._stepsInDecelPhase,
			_axisStepData[1]._initialStepRatePerKTicks, _axisStepData[1]._accStepsPerKTicksPerMS,
			_axisStepData[1]._stepsInAccPhase, _axisStepData[1]._stepsInPlateauPhase, _axisStepData[1]._stepsInDecelPhase);
	}

};
