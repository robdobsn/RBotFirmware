// RBotFirmware
// Rob Dobson 2017

#pragma once

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
        _betweenStepsNs = 1000000;
        _betweenStepsNsChangePerStep = 0;
    }

    double stepsPerUnit()
    {
        if (_unitsPerRotation != 0)
            return _stepsPerRotation / _unitsPerRotation;
        return 1;
    }
};
