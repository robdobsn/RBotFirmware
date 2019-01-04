// RBotFirmware
// Rob Dobson 2016-18

#pragma once

#include "RdJson.h"

class AxisParams
{
  public:
    static constexpr float maxSpeed_default = 100.0f;
    static constexpr float acceleration_default = 100.0f;
    static constexpr float stepsPerRot_default = 1.0f;
    static constexpr float unitsPerRot_default = 1.0f;
    static constexpr float maxRPM_default = 300.0f;
    static constexpr float homeOffsetVal_default = 0.0f;
    static constexpr long homeOffSteps_default = 0;
    static constexpr float minSpeedMMps_default = 0.0f;
    static constexpr uint32_t stepsForAxisHoming_default = 100000;

    // Parameters
    float _maxSpeedMMps;
    float _minSpeedMMps;
    float _maxAccelMMps2;
    float _stepsPerRot;
    float _unitsPerRot;
    float _maxRPM;
    bool _minValValid;
    float _minVal;
    bool _maxValValid;
    float _maxVal;
    bool _isPrimaryAxis;
    bool _isDominantAxis;
    // A servo axis is one which does not require blockwise stepping to a destination
    bool _isServoAxis;
    float _homeOffsetVal;
    long _homeOffSteps;

  public:
    AxisParams()
    {
        clear();
    }

    void clear()
    {
        _maxSpeedMMps = maxSpeed_default;
        _minSpeedMMps = minSpeedMMps_default;
        _maxAccelMMps2 = acceleration_default;
        _stepsPerRot = stepsPerRot_default;
        _unitsPerRot = unitsPerRot_default;
        _maxRPM = maxRPM_default;
        _minValValid = false;
        _minVal = 0;
        _maxValValid = false;
        _maxVal = 0;
        _isPrimaryAxis = true;
        _isDominantAxis = false;
        _isServoAxis = false;
        _homeOffsetVal = homeOffsetVal_default;
        _homeOffSteps = homeOffSteps_default;
    }

    float stepsPerUnit()
    {
        if (_unitsPerRot != 0)
            return _stepsPerRot / _unitsPerRot;
        return 1;
    }

    bool ptInBounds(float &val, bool correctValueInPlace)
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

    void setFromJSON(const char *axisJSON)
    {
        // Stepper motor
        _maxSpeedMMps = float(RdJson::getDouble("maxSpeed", AxisParams::maxSpeed_default, axisJSON));
        _maxAccelMMps2 = float(RdJson::getDouble("maxAcc", AxisParams::acceleration_default, axisJSON));
        _stepsPerRot = float(RdJson::getDouble("stepsPerRot", AxisParams::stepsPerRot_default, axisJSON));
        _unitsPerRot = float(RdJson::getDouble("unitsPerRot", AxisParams::unitsPerRot_default, axisJSON));
        _maxRPM = float(RdJson::getDouble("maxRPM", AxisParams::maxRPM_default, axisJSON));
        _minVal = float(RdJson::getDouble("minVal", 0, _minValValid, axisJSON));
        _maxVal = float(RdJson::getDouble("maxVal", 0, _maxValValid, axisJSON));
        _isDominantAxis = RdJson::getLong("isDominantAxis", 0, axisJSON) != 0;
        _isPrimaryAxis = RdJson::getLong("isPrimaryAxis", 1, axisJSON) != 0;
        _isServoAxis = RdJson::getLong("isServoAxis", 0, axisJSON) != 0;
        _homeOffsetVal = float(RdJson::getDouble("homeOffsetVal", 0, axisJSON));
        _homeOffSteps = RdJson::getLong("homeOffSteps", 0, axisJSON);
    }

    void debugLog(int axisIdx)
    {
        Log.notice("Axis%d params maxSpeed %F, acceleration %F, stepsPerRot %F, unitsPerRot %F, maxRPM %F\n",
                   axisIdx, _maxSpeedMMps, _maxAccelMMps2, _stepsPerRot, _unitsPerRot, _maxRPM);
        Log.notice("Axis%d params minVal %F (%d), maxVal %F (%d), isDominant %d, isServo %d, homeOffVal %F, homeOffSteps %d\n",
                   axisIdx, _minVal, _minValValid, _maxVal, _maxValValid, _isDominantAxis, _isServoAxis, _homeOffsetVal, _homeOffSteps);
    }
};
