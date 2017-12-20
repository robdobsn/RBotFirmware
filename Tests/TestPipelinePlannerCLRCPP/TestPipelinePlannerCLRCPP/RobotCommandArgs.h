// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "Application.h"
#include "AxisValues.h"

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
	AxisFloats pt;
	bool extrudeValid;
	float extrudeVal;
	bool feedrateValid;
	float feedrateVal;
	RobotEndstopArg endstopEnum;
	bool moveClockwise;
	bool moveRapid;
	RobotMoveTypeArg moveType;

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
		moveType = RobotMoveTypeArg_None;
	}
	void copy(RobotCommandArgs& copyFrom)
	{
		clear();
		pt = copyFrom.pt;
		extrudeValid = copyFrom.extrudeValid;
		extrudeVal = copyFrom.extrudeVal;
		feedrateValid = copyFrom.feedrateValid;
		feedrateVal = copyFrom.feedrateVal;
		endstopEnum = copyFrom.endstopEnum;
		moveClockwise = copyFrom.moveClockwise;
		moveRapid = copyFrom.moveRapid;
		moveType = copyFrom.moveType;
	}
	void setAxisValue(int axisIdx, float value, bool isValid)
	{
		if (axisIdx >= 0 && axisIdx < AxisBools::MAX_AXES)
		{
			pt.setVal(axisIdx, value);
			pt.setValid(axisIdx, isValid);
		}
	}
};
