// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "PointND.h"

enum RobotEndstopArg
{
    RobotEndstopArg_None,
    RobotEndstopArg_Check,
    RobotEndstopArg_Ignore
};

class RobotCommandArgs
{
public:
    PointND pt;
    PointNDValid valid;
    bool extrudeValid;
    double extrudeVal;
    bool feedrateValid;
    double feedrateVal;
    RobotEndstopArg endstopEnum;
    bool moveClockwise;
    bool moveRapid;

public:
    RobotCommandArgs()
    {
        extrudeValid = feedrateValid = false;
        extrudeVal = feedrateVal = 0.0;
        endstopEnum = RobotEndstopArg_None;
        moveClockwise = moveRapid = false;
    }
    void setAxisValue(int axisIdx, double value, bool isValid)
    {
        if (axisIdx >= 0 && axisIdx < MAX_AXES)
        {
            pt.setVal(axisIdx, value);
            valid.setVal(axisIdx, isValid);
        }
    }
};
