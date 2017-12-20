// TestPipelinePlannerCLRCPP.cpp : main project file.

#include "conio.h"

using namespace System;

#include "MotionHelper.h"

 static const char* ROBOT_CONFIG_STR_XY =
     "{\"robotType\": \"XYBot\", \"xMaxMM\":500, \"yMaxMM\":500, "
     " \"stepEnablePin\":\"A2\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
     " \"cmdsAtStart\":\"\", "
     " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":100.0, \"acceleration\":100.0,"
     " \"stepsPerRotation\":3200, \"unitsPerRotation\":60, \"minNsBetweenSteps\":10000},"
     " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":100.0, \"acceleration\":100.0,"
     " \"stepsPerRotation\":3200, \"unitsPerRotation\":60, \"minNsBetweenSteps\":10000},"
     " \"commandQueue\": { \"cmdQueueMaxLen\":50 } "
     "}";

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

int main()
{
    Console::WriteLine(L"Hello World");

	MotionHelper _motionHelper;
	RobotCommandArgs rcArgs;

	_motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
	_motionHelper.setAxisParams(ROBOT_CONFIG_STR_XY);
	
	//rcArgs.setAxisValue(0, 1, 0);
	//rcArgs.setAxisValue(1, 0, 0);
	//_motionHelper.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 2, 0);
	//rcArgs.setAxisValue(1, 0, 0);
	//_motionHelper.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 3, 0);
	//rcArgs.setAxisValue(1, 0, 0);
	//_motionHelper.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 3.5, 0);
	//rcArgs.setAxisValue(1, 0.5, 0);
	//_motionHelper.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 4, 0);
	//rcArgs.setAxisValue(1, 1.5, 0);
	//_motionHelper.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 5, 0);
	//rcArgs.setAxisValue(1, 3, 0);
	//_motionHelper.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 6, 0);
	//rcArgs.setAxisValue(1, 5, 0);
	//_motionHelper.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 7, 0);
	//rcArgs.setAxisValue(1, 7, 0);
	//_motionHelper.moveTo(rcArgs);

	rcArgs.setAxisValue(0, 1, 0);
	rcArgs.setAxisValue(1, 0, 0);
	_motionHelper.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 2, 0);
	rcArgs.setAxisValue(1, 0, 0);
	_motionHelper.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 3, 0);
	rcArgs.setAxisValue(1, 0, 0);
	_motionHelper.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 4, 0);
	rcArgs.setAxisValue(1, 0, 0);
	_motionHelper.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 5, 0);
	rcArgs.setAxisValue(1, 0, 0);
	_motionHelper.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 6, 0);
	rcArgs.setAxisValue(1, 0, 0);
	_motionHelper.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 7, 0);
	rcArgs.setAxisValue(1, 0, 0);
	_motionHelper.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 8, 0);
	rcArgs.setAxisValue(1, 0, 0);
	_motionHelper.moveTo(rcArgs);


	_motionHelper.debugShowBlocks();
	while (true)
	{
		if (_kbhit())
			break;
		_motionHelper.service(true);
	}
    return 0;
}

