// RBotFirmware
// Rob Dobson 2016-18

#pragma once

#include "RdJson.h"

class AxisParams
{
public:
  static constexpr float maxSpeed_default         = 100.0f;
  static constexpr float acceleration_default     = 100.0f;
  static constexpr float stepsPerRotation_default = 1.0f;
  static constexpr float unitsPerRotation_default = 1.0f;
  static constexpr float homeOffsetVal_default    = 0.0f;
  static constexpr long homeOffsetSteps_default   = 0;
  static constexpr float minSpeedMMps_default     = 0.0f;

  // Parameters
  float _maxSpeedMMps;
  float _minSpeedMMps;
  float _maxAccelMMps2;
  float _stepsPerRotation;
  float _unitsPerRotation;
  bool _minValValid;
  float _minVal;
  bool _maxValValid;
  float _maxVal;
  bool _isPrimaryAxis;
  bool _isDominantAxis;
  // A servo axis is one which does not require blockwise stepping to a destination
  bool _isServoAxis;
  float _homeOffsetVal;
  long _homeOffsetSteps;

public:
  AxisParams()
  {
    clear();
  }

  void clear()
  {
    _maxSpeedMMps     = maxSpeed_default;
    _minSpeedMMps     = minSpeedMMps_default;
    _maxAccelMMps2    = acceleration_default;
    _stepsPerRotation = stepsPerRotation_default;
    _unitsPerRotation = unitsPerRotation_default;
    _minValValid      = false;
    _minVal           = 0;
    _maxValValid      = false;
    _maxVal           = 0;
    _isPrimaryAxis    = true;
    _isDominantAxis   = false;
    _isServoAxis      = false;
    _homeOffsetVal    = homeOffsetVal_default;
    _homeOffsetSteps  = homeOffsetSteps_default;
  }

  float stepsPerUnit()
  {
    if (_unitsPerRotation != 0)
      return _stepsPerRotation / _unitsPerRotation;
    return 1;
  }

  bool ptInBounds(float& val, bool correctValueInPlace)
  {
    bool wasValid = true;
    if (_minValValid && val < _minVal)
    {
      wasValid = false;
      if (correctValueInPlace)
        val = _minVal;
    }
    if (_maxValValid && val > _maxVal)
    {
      wasValid = false;
      if (correctValueInPlace)
        val = _maxVal;
    }
    return wasValid;
  }

  void setHomePosition(float val, long steps)
  {
    _homeOffsetVal   = val;
    _homeOffsetSteps = steps;
  }

  void setFromJSON(const char* axisJSON)
  {
    // Stepper motor
    _maxSpeedMMps     = float(RdJson::getDouble("maxSpeed", AxisParams::maxSpeed_default, axisJSON));
    _maxAccelMMps2    = float(RdJson::getDouble("maxAcc", AxisParams::acceleration_default, axisJSON));
    _stepsPerRotation = float(RdJson::getDouble("stepsPerRotation", AxisParams::stepsPerRotation_default, axisJSON));
    _unitsPerRotation = float(RdJson::getDouble("unitsPerRotation", AxisParams::unitsPerRotation_default, axisJSON));
    _minVal           = float(RdJson::getDouble("minVal", 0, _minValValid, axisJSON));
    _maxVal           = float(RdJson::getDouble("maxVal", 0, _maxValValid, axisJSON));
    _isDominantAxis   = RdJson::getLong("isDominantAxis", 0, axisJSON) != 0;
    _isPrimaryAxis    = RdJson::getLong("isPrimaryAxis", 1, axisJSON) != 0;
    _isServoAxis      = RdJson::getLong("isServoAxis", 0, axisJSON) != 0;
    _homeOffsetVal    = float(RdJson::getDouble("homeOffsetVal", 0, axisJSON));
    _homeOffsetSteps  = RdJson::getLong("homeOffsetSteps", 0, axisJSON);
  }

  void debugLog(int axisIdx)
  {
    Log.info("Axis%d params maxSpeed %02.f, acceleration %0.2f, stepsPerRotation %0.2f, unitsPerRotation %0.2f",
             axisIdx, _maxSpeedMMps, _maxAccelMMps2, _stepsPerRotation, _unitsPerRotation);
    Log.info("Axis%d params minVal %02.f (%d), maxVal %0.2f (%d), isDominant %d, isServo %d, homeOffVal %0.2f, homeOffSteps %ld",
             axisIdx, _minVal, _minValValid, _maxVal, _maxValValid, _isDominantAxis, _isServoAxis, _homeOffsetVal, _homeOffsetSteps);
  }
};
