// TestPipelinePlannerCLRCPP.cpp : main project file.

#include "conio.h"

using namespace System;

#include "TestCaseMotion.h"
#include "MotionHelper.h"

static const char* ROBOT_CONFIG_STR_XY =
    "{\"robotType\": \"XYBot\", \"xMaxMM\":500, \"yMaxMM\":500, "
    " \"stepEnablePin\":\"A2\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
    " \"cmdsAtStart\":\"\", "
    " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":100.0, \"acceleration\":100.0,"
    " \"stepsPerRotation\":3200, \"unitsPerRotation\":60 },"
    " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":100.0, \"acceleration\":100.0,"
    " \"stepsPerRotation\":3200, \"unitsPerRotation\":60 },"
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

bool isApprox(double a, double b, double epsilon = 0.001)
{
	return (fabs(a - b) < epsilon);
}

static bool getCmdNumber(const char* pCmdStr, int& cmdNum)
{
	// String passed in should start with a G or M
	// And be followed immediately by a number
	if (strlen(pCmdStr) < 1)
		return false;
	const char* pStr = pCmdStr + 1;
	if (!isdigit(*pStr))
		return false;
	long rsltStr = strtol(pStr, NULL, 10);
	cmdNum = (int)rsltStr;
	return true;
}

static bool getGcodeCmdArgs(const char* pArgStr, RobotCommandArgs& cmdArgs)
{
	const char* pStr = pArgStr;
	char* pEndStr = NULL;
	while (*pStr)
	{
		switch (toupper(*pStr))
		{
		case 'X':
			cmdArgs.setAxisValue(0, (float)strtod(++pStr, &pEndStr), true);
			pStr = pEndStr;
			break;
		case 'Y':
			cmdArgs.setAxisValue(1, (float)strtod(++pStr, &pEndStr), true);
			pStr = pEndStr;
			break;
		case 'Z':
			cmdArgs.setAxisValue(2, (float)strtod(++pStr, &pEndStr), true);
			pStr = pEndStr;
			break;
		case 'E':
			cmdArgs.extrudeValid = true;
			cmdArgs.extrudeVal = (float)strtod(++pStr, &pEndStr);
			pStr = pEndStr;
			break;
		case 'F':
			cmdArgs.feedrateValid = true;
			cmdArgs.feedrateVal = (float)strtod(++pStr, &pEndStr);
			pStr = pEndStr;
			break;
		case 'S':
		{
			int endstopIdx = strtol(++pStr, &pEndStr, 10);
			pStr = pEndStr;
			if (endstopIdx == 1)
				cmdArgs.endstopEnum = RobotEndstopArg_Check;
			else if (endstopIdx == 0)
				cmdArgs.endstopEnum = RobotEndstopArg_Ignore;
			break;
		}
		default:
			pStr++;
			break;
		}
	}
	return true;
}

// Interpret GCode G commands
static bool interpG(String& cmdStr, bool takeAction, MotionHelper& motionHelper)
{
	// Command string as a text buffer
	const char* pCmdStr = cmdStr.c_str();

	// Command number
	int cmdNum = 0;
	bool rslt = getCmdNumber(cmdStr, cmdNum);
	if (!rslt)
		return false;

	// Get args string (separated from the GXX or MXX by a space)
	const char* pArgsStr = "";
	const char* pArgsPos = strstr(pCmdStr, " ");
	if (pArgsPos != 0)
		pArgsStr = pArgsPos + 1;
	RobotCommandArgs cmdArgs;
	rslt = getGcodeCmdArgs(pArgsStr, cmdArgs);

	switch (cmdNum)
	{
	case 0: // Move rapid
	case 1: // Move
		if (takeAction)
		{
			cmdArgs.moveRapid = (cmdNum == 0);
			motionHelper.moveTo(cmdArgs);
		}
		return true;
	}
	return false;
}

int main()
{
    Console::WriteLine(L"Test RBot Motion");

	TestCaseHandler testCaseHandler;
	bool readOk = testCaseHandler.readTestFile("TestCaseMotionFile.txt");
	if (!readOk)
	{
		printf("Failed to read test case file");
		exit(1);
	}

	int errorCount = 0;

	// Go through tests
	for each (TestCaseM^ tc in testCaseHandler.testCases)
	{
		MotionHelper _motionHelper;

		_motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
		_motionHelper.setAxisParams(ROBOT_CONFIG_STR_XY);

		for (int i = 0; i < tc->numIns(); i++)
		{
			RobotCommandArgs rcArgs;
			const char* pStr = NULL;
			tc->getIn(i, pStr);
			String cmdStr = pStr;
			interpG(cmdStr, true, _motionHelper);
		}

		if (tc->numIns() != _motionHelper.testGetPipelineCount())
		{
			Log.info("ERROR Pipeline len != Gcode count\n");
			errorCount++;
		}

		if (tc->numOuts() != _motionHelper.testGetPipelineCount())
		{
			Log.info("ERROR Pipeline len != Output check count\n");
			errorCount++;
		}

		for (int i = 0; i < tc->numOuts(); i++)
		{
			char* pStr = NULL;
			tc->getOut(i, pStr);
			MotionPipelineElem elem;
			_motionHelper.testGetPipelineElem(i, elem);
			char toks[] = " ";
			char *pRslt;
			pRslt = strtok(pStr, toks);
			int valIdx = 0;
			while (pRslt != NULL) 
			{
				if (valIdx == 0)
				{
					if (!isApprox(elem._entrySpeedMMps, atof(pRslt)))
					{
						Log.info("ERROR field _entrySpeedMMps mismatch");
						errorCount++;
					}
				}
				else if (valIdx == 1)
				{
					if (!isApprox(elem._exitSpeedMMps, atof(pRslt)))
					{
						Log.info("ERROR field _exitSpeedMMps mismatch");
						errorCount++;
					}
				}
				valIdx++;
				pRslt = strtok(NULL, toks);
			}

		}

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

		//rcArgs.setAxisValue(0, 1, 0);
		//rcArgs.setAxisValue(1, 0, 0);
		//_motionHelper.moveTo(rcArgs);
		//rcArgs.setAxisValue(0, 2, 0);
		//rcArgs.setAxisValue(1, 0, 0);
		//_motionHelper.moveTo(rcArgs);
		//rcArgs.setAxisValue(0, 3, 0);
		//rcArgs.setAxisValue(1, 0, 0);
		//_motionHelper.moveTo(rcArgs);
		//rcArgs.setAxisValue(0, 4, 0);
		//rcArgs.setAxisValue(1, 0, 0);
		//_motionHelper.moveTo(rcArgs);
		//rcArgs.setAxisValue(0, 5, 0);
		//rcArgs.setAxisValue(1, 0, 0);
		//_motionHelper.moveTo(rcArgs);
		//rcArgs.setAxisValue(0, 6, 0);
		//rcArgs.setAxisValue(1, 0, 0);
		//_motionHelper.moveTo(rcArgs);
		//rcArgs.setAxisValue(0, 7, 0);
		//rcArgs.setAxisValue(1, 0, 0);
		//_motionHelper.moveTo(rcArgs);
		//rcArgs.setAxisValue(0, 8, 0);
		//rcArgs.setAxisValue(1, 0, 0);
		//_motionHelper.moveTo(rcArgs);


		_motionHelper.debugShowBlocks();

		if (errorCount != 0)
		{
			Log.info("-------------ERRORS---------------");
		}
		Log.info("Total error count %d", errorCount);


		while (true)
		{
			if (_kbhit())
				break;
			_motionHelper.service(true);
		}

	}
    return 0;
}

