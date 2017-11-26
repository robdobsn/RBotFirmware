// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "RdJson.h"

class AxisParams
{
public:
    static constexpr double maxSpeed_default = 100.0;
    static constexpr double acceleration_default = 100.0;
    static constexpr double minNsBetweenSteps_default = 10000.0;
    static constexpr double stepsPerRotation_default = 1.0;
    static constexpr double unitsPerRotation_default = 1.0;
    static constexpr double homeOffsetVal_default = 0;
    static constexpr long homeOffsetSteps_default = 0;

    // Parameters
    double _maxSpeed;
    double _acceleration;
    double _minNsBetweenSteps;
    double _stepsPerRotation;
    double _unitsPerRotation;
    bool _minValValid;
    double _minVal;
    bool _maxValValid;
    double _maxVal;
    bool _isDominantAxis;
    // A servo axis is one which does not require blockwise stepping to a destination
    bool _isServoAxis;
    double _homeOffsetVal;
    long _homeOffsetSteps;

    // Motion values
    long _stepsFromHome;
    long _targetStepsFromHome;
    unsigned long _lastStepMicros;
    unsigned long _betweenStepsNs;
    long _betweenStepsNsChangePerStep;

public:
    AxisParams()
    {
        _maxSpeed = maxSpeed_default;
        _acceleration = acceleration_default;
        _minNsBetweenSteps = minNsBetweenSteps_default;
        _stepsPerRotation = stepsPerRotation_default;
        _unitsPerRotation = unitsPerRotation_default;
        _stepsFromHome = 0;
        _targetStepsFromHome = 0;
        _lastStepMicros = 0;
        _minValValid = false;
        _maxValValid = false;
        _isDominantAxis = false;
        _betweenStepsNs = 1000000;
        _betweenStepsNsChangePerStep = 0;
        _isServoAxis = false;
        _homeOffsetVal = homeOffsetVal_default;
        _homeOffsetSteps = homeOffsetSteps_default;
    }

    double stepsPerUnit()
    {
        if (_unitsPerRotation != 0)
            return _stepsPerRotation / _unitsPerRotation;
        return 1;
    }

    void setFromJSON(const char* axisJSON)
    {
        // Stepper motor
        _maxSpeed = RdJson::getDouble("maxSpeed", AxisParams::maxSpeed_default, axisJSON);
        _acceleration = RdJson::getDouble("acceleration", AxisParams::acceleration_default, axisJSON);
        _minNsBetweenSteps = RdJson::getDouble("minNsBetweenSteps", AxisParams::minNsBetweenSteps_default, axisJSON);
        _stepsPerRotation = RdJson::getDouble("stepsPerRotation", AxisParams::stepsPerRotation_default, axisJSON);
        _unitsPerRotation = RdJson::getDouble("unitsPerRotation", AxisParams::unitsPerRotation_default, axisJSON);
        _minVal = RdJson::getDouble("minVal", 0, _minValValid, axisJSON);
        _maxVal = RdJson::getDouble("maxVal", 0, _maxValValid, axisJSON);
        _isDominantAxis = RdJson::getLong("isDominantAxis", 0, axisJSON) != 0;
        _isServoAxis = RdJson::getLong("isServoAxis", 0, axisJSON) != 0;
        _homeOffsetVal = RdJson::getDouble("homeOffsetVal", 0, axisJSON);
        _homeOffsetSteps = RdJson::getLong("homeOffsetSteps", 0, axisJSON);
    }
};
