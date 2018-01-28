// TestPipelinePlannerCLRCPP.cpp : main project file.

#include "conio.h"
#include "Stopwatch.h"
using namespace System::Runtime::InteropServices;
using namespace System;

#include "TestCaseMotion.h"
#include "MotionHelper.h"

double __maxElapsedMoveToTime = 0;

#define ROBOT_XY 1

#ifdef ROBOT_MUGBOT

static const char* ROBOT_CONFIG_STR =
"{\"robotType\": \"XYBot\", \"xMaxMM\":500, \"yMaxMM\":500, \"pipelineLen\":100, "
" \"stepEnablePin\":\"D4\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
" \"cmdsAtStart\":\"\", "
" \"axis0\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":10.0, \"maxAcc\":10.0,"
" \"stepsPerRotation\":6400, \"unitsPerRotation\":360 },"
" \"axis1\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":10.0, \"maxAcc\":10.0,"
" \"stepsPerRotation\":1600, \"unitsPerRotation\":1 },"
" \"commandQueue\": { \"cmdQueueMaxLen\":50 } "
"}";

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

#elif defined ROBOT_XY

static const char* ROBOT_CONFIG_STR =
"{\"robotType\": \"XYBot\", \"xMaxMM\":500, \"yMaxMM\":500, \"pipelineLen\":100, "
" \"stepEnablePin\":\"D4\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
" \"cmdsAtStart\":\"\", "
" \"axis0\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":100.0, \"maxAcc\":10.0,"
" \"stepsPerRotation\":3200, \"unitsPerRotation\":32 },"
" \"axis1\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":100.0, \"maxAcc\":10.0,"
" \"stepsPerRotation\":3200, \"unitsPerRotation\":32 },"
" \"commandQueue\": { \"cmdQueueMaxLen\":50 } "
"}";

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

#else

static const char* ROBOT_CONFIG_STR =
"{\"robotType\": \"AxiBot\", \"xMaxMM\":500, \"yMaxMM\":500, \"pipelineLen\":100, "
" \"stepEnablePin\":\"D4\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
" \"cmdsAtStart\":\"\", "
" \"axis0\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":10.0, \"maxAcc\":10.0,"
" \"stepsPerRotation\":3200, \"unitsPerRotation\":32 },"
" \"axis1\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":10.0, \"maxAcc\":10.0,"
" \"stepsPerRotation\":1600, \"unitsPerRotation\":32 },"
" \"commandQueue\": { \"cmdQueueMaxLen\":50 } "
"}";

static bool ptToActuator(AxisFloats& pt, AxisFloats& actuatorCoords, AxesParams& axesParams)
{
	// Check machine bounds and fix the value if required
	bool ptWasValid = axesParams.ptInBounds(pt, true);

	// Perform conversion
	float axis0ValFromHome = pt.getVal(0) - axesParams.getHomeOffsetVal(0);
	float axis1ValFromHome = pt.getVal(1) - axesParams.getHomeOffsetVal(1);
	float axis2ValFromHome = pt.getVal(2) - axesParams.getHomeOffsetVal(2);

	// Convert to steps and add offset to home in steps
	actuatorCoords.setVal(0, (axis0ValFromHome + axis1ValFromHome) * axesParams.getStepsPerUnit(0)
		+ axesParams.getHomeOffsetSteps(0));
	actuatorCoords.setVal(1, (axis0ValFromHome - axis1ValFromHome) * axesParams.getStepsPerUnit(1)
		+ axesParams.getHomeOffsetSteps(1));

	// Convert to steps and add offset to home in steps
	actuatorCoords.setVal(2, axis2ValFromHome * axesParams.getStepsPerUnit(2)
		+ axesParams.getHomeOffsetSteps(2));

	// Log.trace("ptToActuator %f -> %f (homeOffVal %f, homeOffSteps %ld)",
	// 	pt.getVal(axisIdx), actuatorCoords._pt[axisIdx],
	// 	axesParams.getHomeOffsetVal(axisIdx), axesParams.getHomeOffsetSteps(axisIdx));

	return ptWasValid;
}

#endif

static void actuatorToPt(AxisFloats& actuatorCoords, AxisFloats& pt, AxesParams& axesParams)
{
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
			if (!cmdArgs.pt.isValid(0) && !cmdArgs.pt.isValid(1))
				break;
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


void testMotionElemVals(int outIdx, int valIdx, int& errorCount, MotionBlock& elem, const char* pRslt, AxesParams& axesParams)
{
	TEST_FIELD __testFields[] = {
		{ false, "idx", false, 0, 0 },
		{ true, "_entrySpeedMMps", true, elem._entrySpeedMMps, 0},
		{ true, "_exitSpeedMMps", true, elem._exitSpeedMMps, 0 },
		{ true, "_axisStepsToTargetX", false, 0, elem.getStepsToTarget(0) },
		{ true, "_axisStepsToTargetY", false, 0, elem.getStepsToTarget(1) },
		{ true, "_axisStepsToTargetZ", false, 0, elem.getStepsToTarget(2) },
		{ true, "_stepsBeforeDecel", false, 0, elem._stepsBeforeDecel },
		{ true, "_initialStepRatePerKTicks", true, DEBUG_STEP_TTICKS_TO_MMPS(elem._initialStepRatePerTTicks, axesParams, 0), 0 },
		{ true, "_maxStepRatePerTTicks", true, DEBUG_STEP_TTICKS_TO_MMPS(elem._maxStepRatePerTTicks, axesParams, 0), 0 },
		{ true, "_finalStepRatePerTTicks", true, DEBUG_STEP_TTICKS_TO_MMPS(elem._finalStepRatePerTTicks, axesParams, 0), 0 },
		{ true, "_accStepsPerKTicksPerMS", false, 0, elem._accStepsPerTTicksPerMS },
	};

	if (valIdx >= sizeof(__testFields) / sizeof(TEST_FIELD))
		return;
	if (!__testFields[valIdx].checkThis)
		return;

	if (__testFields[valIdx].isFloat)
	{
		if (!isApproxF(__testFields[valIdx].valFloat, atof(pRslt), fabs(__testFields[valIdx].valFloat)/100))
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
	Log.trace("Sizeof MotionBlock %d, AxisBools %d, AxisFloat %d, AxisUint32s %d",
		sizeof(MotionBlock), sizeof(AxisBools), sizeof(AxisFloats), sizeof(AxisInt32s));

	Log.trace("");
	Log.trace("##################################################");
	Log.trace("Starting Tests");
	Log.trace("##################################################");

	int totalErrorCount = 0;

	int debugFileNameIdx = 0;
	int testIdx = 0;

	// Go through tests
	for each (TestCaseM^ tc in testCaseHandler.testCases)
	{
		Log.trace("");
		Log.trace("========================== STARTING TEST .... %s", tc->_name);
		int errorCount = 0;

		MotionHelper _motionHelper;
		_motionHelper.setTestMode("OUTPUTSTEPDATA");
		_motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
		_motionHelper.configure(ROBOT_CONFIG_STR);

		for (int i = 0; i < tc->numIns(); i++)
		{
			RobotCommandArgs rcArgs;
			const char* pStr = NULL;
			tc->getIn(i, pStr);
			String cmdStr = pStr;
			Log.trace("Adding Block %s", cmdStr);
			interpG(cmdStr, true, _motionHelper);
		}

		if (tc->numOuts() != _motionHelper.testGetPipelineCount())
		{
			Log.info("ERROR Pipeline len != Output check count (%d vs %d)\n", _motionHelper.testGetPipelineCount(), tc->numOuts());
			errorCount++;
		}

		// Unpause the pipeline - only do this now for testing as it makes debug simpler
		_motionHelper.pause(false);

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
				testMotionElemVals(i, valIdx, errorCount, elem, pRslt, _motionHelper.getAxesParams());
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
		testStarting(testIdx, pTestName, ROBOT_CONFIG_STR, testIdx==0, debugFileNameIdx);
		testIdx++;

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
				break;
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

