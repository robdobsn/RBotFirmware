// RBotFirmware
// Rob Dobson 2016

#pragma once

enum RobotEndstopArg
{
    RobotEndstopArg_None,
    RobotEndstopArg_Check,
    RobotEndstopArg_Ignore
};

class RobotCommandArgs
{
public:
    bool xValid;
    double xVal;
    bool yValid;
    double yVal;
    bool zValid;
    double zVal;
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
        xValid = yValid = zValid = extrudeValid = feedrateValid = false;
        xVal = yVal = zVal = extrudeVal = feedrateVal = 0.0;
        endstopEnum = RobotEndstopArg_None;
        moveClockwise = moveRapid = false;
    }
};
