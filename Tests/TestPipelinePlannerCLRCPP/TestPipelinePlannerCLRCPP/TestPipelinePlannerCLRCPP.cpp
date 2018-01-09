// TestPipelinePlannerCLRCPP.cpp : main project file.

#include "conio.h"
#include "Stopwatch.h"
using namespace System::Runtime::InteropServices;
using namespace System;

#include "TestCaseMotion.h"
#include "MotionHelper.h"

double __maxElapsedMoveToTime = 0;

static const char* ROBOT_CONFIG_STR_MUGBOT =
"{\"robotType\": \"XYBot\", \"xMaxMM\":500, \"yMaxMM\":500, \"pipelineLen\":100, "
" \"stepEnablePin\":\"D7\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
" \"cmdsAtStart\":\"\", "
" \"axis0\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":10.0, \"maxAcc\":10.0,"
" \"stepsPerRotation\":6400, \"unitsPerRotation\":360 },"
" \"axis1\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":10.0, \"maxAcc\":10.0,"
" \"stepsPerRotation\":1600, \"unitsPerRotation\":1 },"
" \"commandQueue\": { \"cmdQueueMaxLen\":50 } "
"}";

static const char* ROBOT_CONFIG_STR_XY =
	"{\"robotType\": \"XYBot\", \"xMaxMM\":500, \"yMaxMM\":500, \"pipelineLen\":100, "
    " \"stepEnablePin\":\"A2\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
    " \"cmdsAtStart\":\"\", "
    " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":10.0, \"maxAcc\":10.0,"
    " \"stepsPerRotation\":3200, \"unitsPerRotation\":60 },"
    " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":10.0, \"maxAcc\":10.0,"
    " \"stepsPerRotation\":3200, \"unitsPerRotation\":60 },"
    " \"commandQueue\": { \"cmdQueueMaxLen\":50 } "
    "}";

#define ROBOT_CONFIG_STR ROBOT_CONFIG_STR_XY

static bool ptToActuator(AxisFloats& pt, AxisFloats& actuatorCoords, AxesParams& axesParams)
{
	// Check machine bounds and fix the value if required
	bool ptWasValid = axesParams.ptInBounds(pt, true);

	// Perform conversion
	for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
	{
		// Axis val from home point
		float axisValFromHome = pt.getVal(axisIdx) - axesParams.getHomeOffsetVal(axisIdx);
		// Convert to steps and add offset to home in steps
		actuatorCoords.setVal(axisIdx, axisValFromHome * axesParams.getStepsPerUnit(axisIdx)
			+ axesParams.getHomeOffsetSteps(axisIdx));

		Log.trace("ptToActuator %f -> %f (homeOffVal %f, homeOffSteps %ld)",
			pt.getVal(axisIdx), actuatorCoords._pt[axisIdx],
			axesParams.getHomeOffsetVal(axisIdx), axesParams.getHomeOffsetSteps(axisIdx));
	}
	return ptWasValid;
}

static void actuatorToPt(AxisFloats& actuatorCoords, AxisFloats& pt, AxesParams& axesParams)
{
	// Perform conversion
	for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
	{
		float ptVal = actuatorCoords.getVal(axisIdx) - axesParams.getHomeOffsetSteps(axisIdx);
		ptVal = ptVal / axesParams.getStepsPerUnit(axisIdx) + axesParams.getHomeOffsetVal(axisIdx);
		pt.setVal(axisIdx, ptVal);
		Log.trace("actuatorToPt %d %f -> %f (perunit %f)", axisIdx, actuatorCoords.getVal(axisIdx),
			ptVal, axesParams.getStepsPerUnit(axisIdx));
	}
}

static void correctStepOverflow(AxesParams& axesParams)
{
}

bool isApproxF(double a, double b, double epsilon = 0.001)
{
	if (a == b)
		return true;
	if ((fabs(a) < 1e-5) && (fabs(b) < 1e-5))
		return true;
	return (fabs(a - b) < epsilon);
}

bool isApproxL(int64_t a, int64_t b, int64_t epsilon = 1)
{
	return (llabs(a - b) < epsilon);
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
			Stopwatch sw;
			sw.Start();
			motionHelper.moveTo(cmdArgs);
			sw.Stop();
			double mx = sw.ElapsedMilliseconds();
			if (__maxElapsedMoveToTime < mx)
				__maxElapsedMoveToTime = mx;

		}
		return true;
	}
	return false;
}

struct TEST_FIELD
{
	bool checkThis;
	char* name;
	bool isFloat;
	double valFloat;
	int64_t valLong;
};


void testMotionElemVals(int outIdx, int valIdx, int& errorCount, MotionBlock& elem, const char* pRslt)
{
	TEST_FIELD __testFields[] = {
		{ false, "idx", false, 0, 0 },
		{ true, "_entrySpeedMMps", true, elem._entrySpeedMMps, 0},
		{ true, "_exitSpeedMMps", true, elem._exitSpeedMMps, 0 },
		{ true, "_axisStepsToTargetX", false, 0, elem._axisStepsToTarget.vals[0] },
		{ true, "_axisStepsToTargetY", false, 0, elem._axisStepsToTarget.vals[1] },
		{ true, "_initialStepRatePerKTicksX", false, 0, elem._axisStepData[0]._initialStepRatePerKTicks },
		{ true, "_accStepsPerKTicksPerMS", false, 0, elem._axisStepData[0]._accStepsPerKTicksPerMS },
		{ true, "_stepsInAccPhaseX", false, 0, elem._axisStepData[0]._stepsInAccPhase },
		{ true, "_stepsInPlateauPhaseX", false, 0, elem._axisStepData[0]._stepsInPlateauPhase },
		{ true, "_stepsInDecelPhaseX", false, 0, elem._axisStepData[0]._stepsInDecelPhase },
		{ true, "_initialStepRatePerKTicksY", false, 0, elem._axisStepData[1]._initialStepRatePerKTicks },
		{ true, "_accStepsPerKTicksPerMSY", false, 0, elem._axisStepData[1]._accStepsPerKTicksPerMS },
		{ true, "_stepsInAccPhaseY", false, 0, elem._axisStepData[1]._stepsInAccPhase },
		{ true, "_stepsInPlateauPhaseY", false, 0, elem._axisStepData[1]._stepsInPlateauPhase },
		{ true, "_stepsInDecelPhaseY", false, 0, elem._axisStepData[1]._stepsInDecelPhase },

#ifdef USE_SMOOTHIE_CODE
		{ true, "_totalMoveTicks", false, 0, elem._totalMoveTicks },
		{ true, "_initialStepRate", true, elem._initialStepRate, 0 },
		{ true, "_accelUntil", false, 0, elem._accelUntil },
		{ true, "_accelPerTick", true, elem._accelPerTick, 0 },
		{ true, "_decelAfter", false, 0, elem._decelAfter },
		{ true, "_decelPerTick", true, elem._decelPerTick, 0 }
#endif
	};

	if (valIdx >= sizeof(__testFields) / sizeof(TEST_FIELD))
		return;
	if (!__testFields[valIdx].checkThis)
		return;

	if (__testFields[valIdx].isFloat)
	{
		if (!isApproxF(__testFields[valIdx].valFloat, atof(pRslt), fabs(__testFields[valIdx].valFloat)/1000))
		{
			Log.info("ERROR Out %d field %s mismatch %f != %s", outIdx, __testFields[valIdx].name, __testFields[valIdx].valFloat, pRslt);
			errorCount++;
		}
	}
	else
	{
		if (!isApproxL(__testFields[valIdx].valLong, atol(pRslt), 1))
		{
			Log.info("ERROR Out %d field %s mismatch %lld != %s", outIdx, __testFields[valIdx].name, __testFields[valIdx].valLong, pRslt);
			errorCount++;
		}
	}
}

int mainSpeedTest()
{
	MotionHelper _motionHelper;
	_motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
	_motionHelper.configure(ROBOT_CONFIG_STR);
	_motionHelper.pause(true);
	RobotCommandArgs cmdArgs;
	cmdArgs.setAxisValue(0, 100, true);
	cmdArgs.setAxisValue(1, 0, true);
	Stopwatch sw;
	sw.Start();
	_motionHelper.moveTo(cmdArgs);
	sw.Stop();
	double mx = sw.ElapsedMilliseconds();
	Console::WriteLine(mx);
	_getch();
	return 0;
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

#ifdef USE_SMOOTHIE_CODE
	Log.trace("Sizeof MotionPipelineElem %d, AxisBools %d, AxisFloat %d, AxisUint32s %d, axisStepInfo_t %d", 
				sizeof(MotionBlock), sizeof(AxisBools), sizeof(AxisFloats), sizeof(AxisInt32s),
				sizeof(MotionBlock::axisStepInfo_t));
#endif
	Log.trace("Sizeof MotionBlock %d, AxisBools %d, AxisFloat %d, AxisUint32s %d, axisStepData %d",
		sizeof(MotionBlock), sizeof(AxisBools), sizeof(AxisFloats), sizeof(AxisInt32s),
		sizeof(MotionBlock::axisStepData_t));

	Log.trace("");
	Log.trace("##################################################");
	Log.trace("Starting Tests");
	Log.trace("##################################################");

	int totalErrorCount = 0;

	// Go through tests
	for each (TestCaseM^ tc in testCaseHandler.testCases)
	{
		Log.trace("");
		Log.trace("========================== STARTING TEST .... %s", tc->_name);
		int errorCount = 0;

		MotionHelper _motionHelper;

		_motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
		_motionHelper.configure(ROBOT_CONFIG_STR);
		_motionHelper.pause(false);

		for (int i = 0; i < tc->numIns(); i++)
		{
			RobotCommandArgs rcArgs;
			const char* pStr = NULL;
			tc->getIn(i, pStr);
			String cmdStr = pStr;
			interpG(cmdStr, true, _motionHelper);
		}

		if (tc->numOuts() != _motionHelper.testGetPipelineCount())
		{
			Log.info("ERROR Pipeline len != Output check count (%d vs %d)\n", _motionHelper.testGetPipelineCount(), tc->numOuts());
			errorCount++;
		}

		for (int i = 0; i < tc->numOuts(); i++)
		{
			char* pStr = NULL;
			tc->getOut(i, pStr);
			MotionBlock elem;
			if (!_motionHelper.testGetPipelineBlock(i, elem))
				break;
			char toks[] = " ";
			char *pRslt;
			pRslt = strtok(pStr, toks);
			int valIdx = 0;
			while (pRslt != NULL)
			{
				testMotionElemVals(i, valIdx, errorCount, elem, pRslt);
				valIdx++;
				pRslt = strtok(NULL, toks);
			}
		}

		_motionHelper.debugShowBlocks();

		if (errorCount != 0)
		{
			Log.info("-------------ERRORS---------------");
		}
		Log.info("Test Case error count %d", errorCount);

		Log.info("Max elapsed ms %0.3f", __maxElapsedMoveToTime);

		const char* pTestName = tc->getName();
		testStarting(pTestName, ROBOT_CONFIG_STR);

		uint32_t tickCount = 0;
		while (true)
		{
			if (_kbhit())
			{
				_getch();
				break;
			}
			setTickCount(tickCount+=20);
			_motionHelper.service(true);
			if (_motionHelper.isIdle())
			{
				testCompleted();
			}
		}

		totalErrorCount += errorCount;

	}
	if (totalErrorCount != 0)
	{
		Log.info("-------------TOTAL ERRORS---------------");
	}
	Log.info("Total error count %d", totalErrorCount);
	return 0;
}

