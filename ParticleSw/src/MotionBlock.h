// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

#include "math.h"
#include "AxisValues.h"
#include "AxesParams.h"

#define DEBUG_STEP_TTICKS_TO_MMPS(val, axesParams, axisIdx)    (((val * 1.0) * MotionBlock::TICKS_PER_SEC) / MotionBlock::TTICKS_VALUE) * axesParams.getStepDistMM(axisIdx)

class MotionBlock
{
public:
  // Minimum move distance
  static constexpr double MINIMUM_MOVE_DIST_MM = 0.0001;

  // Number of ticks to accumulate for rate actuation
  static constexpr uint32_t TTICKS_VALUE = 1000000000l;

  // Tick interval in NS
  // 20000NS means max of 25k steps per second (as each step requires 2 entries to ISR - at least)
  // The ISR time is now averaging 1.3uS and max 2.8uS so this could be reduced to 10000 if needed
  static constexpr uint32_t TICK_INTERVAL_NS = 20000;
  static constexpr float TICKS_PER_SEC       = (1e9f / TICK_INTERVAL_NS);

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
    volatile bool _canExecute  : 1;
  };

  // Steps to target and before deceleration
  int32_t _stepsTotalMaybeNeg[RobotConsts::MAX_AXES];
  int _axisIdxWithMaxSteps;
  uint32_t _stepsBeforeDecel;

  // Stepping acceleration/deceleration profile
  uint32_t _initialStepRatePerTTicks;
  uint32_t _maxStepRatePerTTicks;
  uint32_t _finalStepRatePerTTicks;
  uint32_t _accStepsPerTTicksPerMS;

public:
  MotionBlock()
  {
    clear();
  }

  void clear()
  {
    // Clear values
    _feedrateMMps             = 0;
    _moveDistPrimaryAxesMM    = 0;
    _maxEntrySpeedMMps        = 0;
    _entrySpeedMMps           = 0;
    _exitSpeedMMps            = 0;
    _isExecuting              = false;
    _canExecute               = false;
    _axisIdxWithMaxSteps      = 0;
    _accStepsPerTTicksPerMS   = 0;
    _finalStepRatePerTTicks   = 0;
    _initialStepRatePerTTicks = 0;
    _maxStepRatePerTTicks     = 0;
    _stepsBeforeDecel         = 0;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
      _stepsTotalMaybeNeg[axisIdx] = 0;
  }

  int32_t getStepsToTarget(int axisIdx)
  {
    if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
    {
      return _stepsTotalMaybeNeg[axisIdx];
    }
    return 0;
  }

  int32_t getAbsStepsToTarget(int axisIdx)
  {
    if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
    {
      return abs(_stepsTotalMaybeNeg[axisIdx]);
    }
    return 0;
  }

  void setStepsToTarget(int axisIdx, int32_t steps)
  {
    if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
    {
      _stepsTotalMaybeNeg[axisIdx] = steps;
      if (abs(steps) > abs(_stepsTotalMaybeNeg[_axisIdxWithMaxSteps]))
        _axisIdxWithMaxSteps = axisIdx;
    }
  }

  uint32_t getExitStepRatePerTTicks()
  {
    return _finalStepRatePerTTicks;
  }

  static float maxAchievableSpeed(float acceleration, float target_velocity, float distance)
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
  void prepareForStepping(AxesParams& axesParams, uint32_t prevBlockExitStepRatesPerTTicks)
  {
    // If block is currently being executed don't change it
    if (_isExecuting)
      return;

    // Find the max number of steps for any axis
    uint32_t absMaxStepsForAnyAxis = abs(_stepsTotalMaybeNeg[_axisIdxWithMaxSteps]);

    // Get the initial step rate, final step rate and max acceleration for the axis with max steps
    float initialStepRatePerSec = _entrySpeedMMps / axesParams.getStepDistMM(_axisIdxWithMaxSteps);
    float finalStepRatePerSec   = _exitSpeedMMps / axesParams.getStepDistMM(_axisIdxWithMaxSteps);
    float axisAccStepsPerSec2   = axesParams.getMaxAccStepsPerSec2(_axisIdxWithMaxSteps);

    // Calculate the distance decelerating and ensure within bounds
    // Using the facts for the block ... (assuming max accleration followed by max deceleration):
    //		Vmax * Vmax = Ventry * Ventry + 2 * Amax * Saccelerating
    //		Vexit * Vexit = Vmax * Vmax - 2 * Amax * Sdecelerating
    //      Stotal = Saccelerating + Sdecelerating
    // And solving for Saccelerating (distance accelerating)
    uint32_t stepsAccelerating      = 0;
    float    stepsAcceleratingFloat =
							ceilf((powf(finalStepRatePerSec, 2) - powf(initialStepRatePerSec, 2)) / 4 /
											axisAccStepsPerSec2 + absMaxStepsForAnyAxis / 2);
    if (stepsAcceleratingFloat > 0)
    {
      stepsAccelerating = uint32_t(stepsAcceleratingFloat);
      if (stepsAccelerating > absMaxStepsForAnyAxis)
        stepsAccelerating = absMaxStepsForAnyAxis;
    }

    // Decelerating steps
    uint32_t stepsDecelerating = 0;

    // Find max possible rate for this axis
    float axisMaxStepRatePerSec = _feedrateMMps / axesParams.getStepDistMM(_axisIdxWithMaxSteps);

    // See if max speed will be reached
    uint32_t stepsToMaxSpeed =
							uint32_t((powf(axisMaxStepRatePerSec, 2) - powf(initialStepRatePerSec, 2)) /
							 					2 / axisAccStepsPerSec2);
    if (stepsAccelerating > stepsToMaxSpeed)
    {
      // Max speed will be reached
      stepsAccelerating = stepsToMaxSpeed;

      // Decelerating steps
      stepsDecelerating =
							uint32_t((powf(axisMaxStepRatePerSec, 2) - powf(finalStepRatePerSec, 2)) /
							 					2 / axisAccStepsPerSec2);
    }
    else
    {
      // Calculate max speed that will be reached
      axisMaxStepRatePerSec =
							sqrtf(powf(initialStepRatePerSec, 2) + 2.0F * axisAccStepsPerSec2 * stepsAccelerating);

      // Decelerating steps
      stepsDecelerating = absMaxStepsForAnyAxis - stepsAccelerating;
    }

    // Fill in the step values for this axis
    _initialStepRatePerTTicks = uint32_t((initialStepRatePerSec * TTICKS_VALUE) / TICKS_PER_SEC);
    _maxStepRatePerTTicks     = uint32_t((axisMaxStepRatePerSec * TTICKS_VALUE) / TICKS_PER_SEC);
    _finalStepRatePerTTicks   = uint32_t((finalStepRatePerSec * TTICKS_VALUE) / TICKS_PER_SEC);
    _accStepsPerTTicksPerMS   = uint32_t(axesParams.getMaxAccStepsPerTTicksPerMs(_axisIdxWithMaxSteps, TTICKS_VALUE, TICKS_PER_SEC));
    _stepsBeforeDecel         = absMaxStepsForAnyAxis - stepsDecelerating;

    // No more changes
    _canExecute = true;
  }


  void debugShowBlkHead()
  {
    Log.info("#i EntMMps ExtMMps StTotX StTotY StTotZ St>Dec    Init      Pk     Fin     Acc");
  }

  void debugShowBlock(int elemIdx, AxesParams& axesParams)
  {
    Log.info("%2d%8.3f%8.3f%7ld%7ld%7ld%7lu%8.3f%8.3f%8.3f%8lu", elemIdx,
             _entrySpeedMMps, _exitSpeedMMps,
             getStepsToTarget(0),
             getStepsToTarget(1),
             getStepsToTarget(2),
             _stepsBeforeDecel,
             DEBUG_STEP_TTICKS_TO_MMPS(_initialStepRatePerTTicks, axesParams, 0),
             DEBUG_STEP_TTICKS_TO_MMPS(_maxStepRatePerTTicks, axesParams, 0),
             DEBUG_STEP_TTICKS_TO_MMPS(_finalStepRatePerTTicks, axesParams, 0),
             _accStepsPerTTicksPerMS);
  }
};
