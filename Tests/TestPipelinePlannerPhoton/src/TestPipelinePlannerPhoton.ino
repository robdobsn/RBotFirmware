
#include "MotionHelper.h"
#include "Utils.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

static const char* ROBOT_CONFIG_STR_MUGBOT =
	"{\"robotType\": \"XYBot\", \"xMaxMM\":500, \"yMaxMM\":500, \"pipelineLen\":100, "
    " \"stepEnablePin\":\"D4\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
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
    " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":10.0, \"maxAcc\":100.0,"
    " \"stepsPerRotation\":3200, \"unitsPerRotation\":60 },"
    " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":10.0, \"maxAcc\":100.0,"
    " \"stepsPerRotation\":3200, \"unitsPerRotation\":60 },"
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

MotionHelper _motionHelper;

int __curTestNum = 0;
const int __numTests = 2;
int __curTestErrorCount = 0;
const int __testSquareDiagonalLen = 6;
int __testSquareDiagonal [__testSquareDiagonalLen][2] =
				{ {1,0}, {1,1}, {0,1}, {0,0}, {1,1}, {0,0} };
const int __testOneBigMoveLen = 1;
int __testOneBigMove [__testOneBigMoveLen][2] =
				{ {10,10} };

bool setupNextTest()
{
	bool testSet = false;
	if (__curTestNum < __numTests)
	{
		RobotCommandArgs cmdArgs;
		Log.info("========================== STARTING TEST %d ==========================",
					__curTestNum);
		if (__curTestNum == 0)
		{
			for (int i = 0; i < __testSquareDiagonalLen; i++)
			{
				cmdArgs.setAxisValue(0, __testSquareDiagonal[i][0], true);
				cmdArgs.setAxisValue(1, __testSquareDiagonal[i][1], true);
				_motionHelper.moveTo(cmdArgs);
			}

			if (_motionHelper.testGetPipelineCount() != __testSquareDiagonalLen)
			{
			    Log.info("ERROR Pipeline len != cmd count\n");
				__curTestErrorCount++;
			}

			_motionHelper.debugShowBlocks();
			testSet = true;
		}
		else if (__curTestNum == 1)
		{
			unsigned long maxMoveToMicros = 0;
			for (int i = 0; i < __testOneBigMoveLen; i++)
			{
				cmdArgs.setAxisValue(0, __testOneBigMove[i][0], true);
				cmdArgs.setAxisValue(1, __testOneBigMove[i][1], true);
				unsigned long curMicros = micros();
				_motionHelper.moveTo(cmdArgs);
				unsigned long elapsedMicros = micros() - curMicros;
				if (maxMoveToMicros < elapsedMicros)
					maxMoveToMicros = elapsedMicros;
			}

			if (_motionHelper.testGetPipelineCount() != __testOneBigMoveLen)
			{
				Log.info("ERROR Pipeline len != cmd count\n");
				__curTestErrorCount++;
			}

			_motionHelper.debugShowBlocks();

			Log.info("Max elapsed uS in moveTo() %lu", maxMoveToMicros);
			testSet = true;
		}

		__curTestNum++;
	}
	return testSet;
}

void setup()
{
	Serial.begin(115200);
	delay(2000);
	Log.info(" ");
	Log.info("========================== TESTING PIPELINE ==========================");

	_motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
	_motionHelper.configure(ROBOT_CONFIG_STR_MUGBOT);
	_motionHelper.setTestMode("OUTPUTSTEPDATA");
	_motionHelper.pause(false);
}

bool __debugTimingShown = true;
bool __wasIdle = false;
unsigned long __idleTime = 0;

void loop()
{
	while (true)
	{
	    _motionHelper.service(true);
		if (_motionHelper.isIdle())
		{
			if (!__debugTimingShown)
			{
				_motionHelper.debugShowTiming();
				__debugTimingShown = true;
			}
			if (!__wasIdle)
			{
				__idleTime = millis();
				__wasIdle = true;
			}
			else
			{
				if (Utils::isTimeout(millis(), __idleTime, 5000) || (__curTestNum == 0))
				{
					_motionHelper.pause(true);
					if (setupNextTest())
					{
						__debugTimingShown = false;
					}
					__wasIdle = false;
					_motionHelper.pause(false);
				}
			}
		}
	}
}
