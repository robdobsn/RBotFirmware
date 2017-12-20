// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "RobotCommandArgs.h"
#include "MotionHelper.h"

class RobotController
{
private:
	MotionHelper _motionHelper;

public:
	RobotController()
	{
		_motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
		_motionHelper.setAxisParams("");
	}

	~RobotController()
	{
	}

	bool init(const char* configStr)
	{
		return true;
	}

	void service()
	{
		_motionHelper.service(true);
	}
	void moveTo(RobotCommandArgs& args)
	{
		Log.trace("RobotController moveTo");
		_motionHelper.moveTo(args);
	}

	static bool ptToActuator(MotionPipelineElem& motionElem, AxisFloats& actuatorCoords, AxisParams axisParams[], int numAxes)
	{
		bool isValid = true;
		for (int i = 0; i < AxisBools::MAX_AXES; i++)
		{
			// Axis val from home point
			float axisValFromHome = motionElem._pt2MM.getVal(i) - axisParams[i]._homeOffsetVal;
			// Convert to steps and add offset to home in steps
			actuatorCoords.setVal(i, axisValFromHome * axisParams[i].stepsPerUnit()
								+ axisParams[i]._homeOffsetSteps);

			// Check machine bounds
			bool thisAxisValid = true;
			if (axisParams[i]._minValValid && motionElem._pt2MM.getVal(i) < axisParams[i]._minVal)
				thisAxisValid = false;
			if (axisParams[i]._maxValValid && motionElem._pt2MM.getVal(i) > axisParams[i]._maxVal)
				thisAxisValid = false;
			Log.trace("ptToActuator (%s) %f -> %f (homeOffVal %f, homeOffSteps %ld)", thisAxisValid ? "OK" : "INVALID",
				motionElem._pt2MM.getVal(i), actuatorCoords._pt[i], axisParams[i]._homeOffsetVal, axisParams[i]._homeOffsetSteps);
			isValid &= thisAxisValid;
		}
		return isValid;
	}

	static void actuatorToPt(AxisFloats& actuatorCoords, AxisFloats& pt, AxisParams axisParams[], int numAxes)
	{
		for (int i = 0; i < AxisBools::MAX_AXES; i++)
		{
			float ptVal = actuatorCoords.getVal(i) - axisParams[i]._homeOffsetSteps;
			ptVal = ptVal / axisParams[i].stepsPerUnit() + axisParams[i]._homeOffsetVal;
			if (axisParams[i]._minValValid && ptVal < axisParams[i]._minVal)
				ptVal = axisParams[i]._minVal;
			if (axisParams[i]._maxValValid && ptVal > axisParams[i]._maxVal)
				ptVal = axisParams[i]._maxVal;
			pt.setVal(i, ptVal);
			Log.trace("actuatorToPt %d %f -> %f (perunit %f)", i, actuatorCoords.getVal(i), ptVal, axisParams[i].stepsPerUnit());
		}
	}

	static void correctStepOverflow(AxisParams axisParams[], int numAxes)
	{
		// Not necessary for a non-continuous rotation bot
	}

	void debugIt()
	{
		_motionHelper.debugShowBlocks();
	}

};