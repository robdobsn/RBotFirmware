// RBotFirmware
// Rob Dobson 2016-18

#pragma once

#include "RobotConsts.h"
#include "AxisParams.h"
#include "AxisValues.h"

class AxesParams
{
private:
  // Axis parameters
  AxisParams _axisParams[RobotConsts::MAX_AXES];
  // Master axis
  int _masterAxisIdx;

public:
  // Cache values for master axis as they are used frequently in the planner
  float _masterAxisMaxAccMMps2;
  float _masterAxisStepDistanceMM;
  // Cache max and min step rates
  AxisFloats _maxStepRatesPerSec;
  AxisFloats _minStepRatesPerSec;
  // Cache MaxAccStepsPerTTicksPerMs
  AxisFloats _maxAccStepsPerTTicksPerMs;
  float _cacheLastTickRatePerSec;


public:

  AxesParams()
  {
    clearAxes();
  }

  void clearAxes()
  {
    _masterAxisIdx            = -1;
    _masterAxisMaxAccMMps2    = AxisParams::acceleration_default;
    _masterAxisStepDistanceMM = AxisParams::unitsPerRotation_default / AxisParams::stepsPerRotation_default;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
      _axisParams[axisIdx].clear();
    _cacheLastTickRatePerSec = 0;
  }

  float getStepsPerUnit(int axisIdx)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return AxisParams::stepsPerRotation_default / AxisParams::unitsPerRotation_default;
    return _axisParams[axisIdx].stepsPerUnit();
  }

  float getStepsPerRotation(int axisIdx)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return AxisParams::stepsPerRotation_default;
    return _axisParams[axisIdx]._stepsPerRotation;
  }

  float getUnitsPerRotation(int axisIdx)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return AxisParams::unitsPerRotation_default;
    return _axisParams[axisIdx]._unitsPerRotation;
  }

  long getHomeOffsetSteps(int axisIdx)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return 0;
    return _axisParams[axisIdx]._homeOffsetSteps;
  }

  void setHomeOffsetSteps(int axisIdx, long newVal)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return;
    _axisParams[axisIdx]._homeOffsetSteps = newVal;
  }

  float getHomeOffsetVal(int axisIdx)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return 0;
    return _axisParams[axisIdx]._homeOffsetVal;
  }

  AxisParams* getAxisParamsArray()
  {
    return _axisParams;
  }

  float getMaxSpeed(int axisIdx)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return AxisParams::maxSpeed_default;
    return _axisParams[axisIdx]._maxSpeedMMps;
  }

  float getMinSpeed(int axisIdx)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return AxisParams::minSpeedMMps_default;
    return _axisParams[axisIdx]._minSpeedMMps;
  }

  float getStepDistMM(int axisIdx)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return AxisParams::unitsPerRotation_default / AxisParams::stepsPerRotation_default;
    return _axisParams[axisIdx]._unitsPerRotation / _axisParams[axisIdx]._stepsPerRotation;
  }

  float getMaxAccel(int axisIdx)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return AxisParams::acceleration_default;
    return _axisParams[axisIdx]._maxAccelMMps2;
  }

  float getMaxAccStepsPerSec2(int axisIdx)
  {
    return getMaxAccel(axisIdx) / getStepDistMM(axisIdx);
  }

  float getMaxAccStepsPerTTicksPerMs(int axisIdx, uint32_t T_VALUE, float tickRatePerSec)
  {
    if (_cacheLastTickRatePerSec != tickRatePerSec)
    {
      // Recalculate based on new tickRate
      for (int i = 0; i < RobotConsts::MAX_AXES; i++)
      {
        float maxAccStepsPerSec2        = getMaxAccel(i) / getStepDistMM(i);
        float maxAccStepsPerTTicksPerMs = (T_VALUE * maxAccStepsPerSec2) / tickRatePerSec / 1000;
        _maxAccStepsPerTTicksPerMs.setVal(i, maxAccStepsPerTTicksPerMs);
      }
      _cacheLastTickRatePerSec = tickRatePerSec;
    }
    return _maxAccStepsPerTTicksPerMs.getVal(axisIdx);
  }

  bool isPrimaryAxis(int axisIdx)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return false;
    return _axisParams[axisIdx]._isPrimaryAxis;
  }

  bool ptInBounds(AxisFloats& pt, bool correctValueInPlace)
  {
    bool wasValid = true;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
      wasValid = wasValid && _axisParams[axisIdx].ptInBounds(pt._pt[axisIdx], correctValueInPlace);
    return wasValid;
  }

  void setHomePosition(AxisFloats& pt, AxisInt32s& steps)
  {
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
      if (pt.isValid(axisIdx))
        _axisParams[axisIdx].setHomePosition(pt.getVal(axisIdx), steps.getVal(axisIdx));
  }

  bool configureAxis(const char* robotConfigJSON, int axisIdx, String& axisJSON)
  {
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
      return false;

    // Get params
    String axisIdStr = "axis" + String(axisIdx);
    axisJSON = RdJson::getString(axisIdStr, "{}", robotConfigJSON);
    if (axisJSON.length() == 0 || axisJSON.equals("{}"))
      return false;

    // Set the axis parameters
    _axisParams[axisIdx].setFromJSON(axisJSON.c_str());
    _axisParams[axisIdx].debugLog(axisIdx);

    // Find the master axis (dominant one, or first primary - or just first)
    setMasterAxis(axisIdx);

    // Cache axis max and min step rates
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
      _maxStepRatesPerSec.setVal(axisIdx, getMaxSpeed(axisIdx) / getStepDistMM(axisIdx));
      _minStepRatesPerSec.setVal(axisIdx, getMinSpeed(axisIdx) / getStepDistMM(axisIdx));
    }
    return true;
  }

  // Set the master axis either to the dominant axis (if there is one)
  // or from the primary axis with highest steps per unit
  // or just the first one found
  void setMasterAxis(int fallbackAxisIdx)
  {
    int   dominantIdx                = -1;
    int   primaryIdx                 = -1;
    float primaryAxisMaxStepsPerUnit = 0;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
      if (_axisParams[i]._isDominantAxis)
      {
        dominantIdx = i;
        break;
      }
      if (_axisParams[i]._isPrimaryAxis)
      {
        if (primaryIdx == -1 || primaryAxisMaxStepsPerUnit < getStepsPerUnit(i))
        {
          primaryIdx                 = i;
          primaryAxisMaxStepsPerUnit = getStepsPerUnit(i);
        }
      }
    }
    if (dominantIdx != -1)
      _masterAxisIdx = dominantIdx;
    else if (primaryIdx != -1)
      _masterAxisIdx = primaryIdx;
    else if (_masterAxisIdx == -1)
      _masterAxisIdx = fallbackAxisIdx;

    // Cache values for master axis
    _masterAxisMaxAccMMps2    = getMaxAccel(_masterAxisIdx);
    _masterAxisStepDistanceMM = getStepDistMM(_masterAxisIdx);
  }
};
