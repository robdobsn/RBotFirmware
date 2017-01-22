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
    double _maxSpeed;
    double _acceleration;
    double _stepsPerRotation;
    double _unitsPerRotation;
    unsigned long _stepsFromHome;

public:
    AxisParams()
    {
        _maxSpeed = maxSpeed_default;
        _acceleration = acceleration_default;
        _stepsPerRotation = stepsPerRotation_default;
        _unitsPerRotation = unitsPerRotation_default;
        _stepsFromHome = 0;
    }

    double stepsPerUnit()
    {
        if (_unitsPerRotation != 0)
            return _stepsPerRotation / _unitsPerRotation;
        return 1;
    }
};
