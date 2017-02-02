// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "ConfigManager.h"

class AxisParams
{
public:
    static constexpr double maxSpeed_default = 100.0;
    static constexpr double acceleration_default = 100.0;
    static constexpr double stepsPerRotation_default = 1.0;
    static constexpr double unitsPerRotation_default = 1.0;

    // Parameters
    double _maxSpeed;
    double _acceleration;
    double _stepsPerRotation;
    double _unitsPerRotation;
    bool _minValValid;
    double _minVal;
    bool _maxValValid;
    double _maxVal;
    bool _isDominantAxis;

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
        _maxSpeed = ConfigManager::getDouble("maxSpeed", AxisParams::maxSpeed_default, axisJSON);
        _acceleration = ConfigManager::getDouble("acceleration", AxisParams::acceleration_default, axisJSON);
        _stepsPerRotation = ConfigManager::getDouble("stepsPerRotation", AxisParams::stepsPerRotation_default, axisJSON);
        _unitsPerRotation = ConfigManager::getDouble("unitsPerRotation", AxisParams::unitsPerRotation_default, axisJSON);
        _minVal = ConfigManager::getDouble("minVal", 0, _minValValid, axisJSON);
        _maxVal = ConfigManager::getDouble("maxVal", 0, _maxValValid, axisJSON);
        _isDominantAxis = ConfigManager::getLong("isDominantAxis", 0, axisJSON) != 0;
    }
};
