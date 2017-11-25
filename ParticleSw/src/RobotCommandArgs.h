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

enum RobotMoveTypeArg
{
    RobotMoveTypeArg_None,
    RobotMoveTypeArg_Absolute,
    RobotMoveTypeArg_Relative
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
    RobotMoveTypeArg moveRelative;

public:
    RobotCommandArgs()
    {
        clear();
    }
    void clear()
    {
        extrudeValid = feedrateValid = false;
        extrudeVal = feedrateVal = 0.0;
        endstopEnum = RobotEndstopArg_None;
        moveClockwise = moveRapid = false;
        moveRelative = RobotMoveTypeArg_None;
    }
    void copy(RobotCommandArgs& copyFrom)
    {
        clear();
        pt = copyFrom.pt;
        valid = copyFrom.valid;
        extrudeValid = copyFrom.extrudeValid;
        extrudeVal = copyFrom.extrudeVal;
        feedrateValid = copyFrom.feedrateValid;
        feedrateVal = copyFrom.feedrateVal;
        endstopEnum = copyFrom.endstopEnum;
        moveClockwise = copyFrom.moveClockwise;
        moveRapid = copyFrom.moveRapid;
        moveRelative = copyFrom.moveRelative;
    }
    void setAxisValue(int axisIdx, double value, bool isValid)
    {
        if (axisIdx >= 0 && axisIdx < MAX_AXES)
        {
            pt.setVal(axisIdx, value);
            valid.setVal(axisIdx, isValid);
        }
    }

    String toJSON()
    {
        String jsonStr;
        jsonStr = "{\"pos\":" + pt.toJSON();
        jsonStr += ", \"mvTyp\":" +
            (moveRelative == RobotMoveTypeArg_Relative ? "rel" : "abs") + "}";
        return jsonStr;
    }
};
