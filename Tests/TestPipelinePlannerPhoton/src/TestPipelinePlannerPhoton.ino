
#include "MotionHelper.h"
#include "Utils.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

static const char* ROBOT_CONFIG_STR_MUGBOT =
	"{\"robotType\": \"XYBot\", \"xMaxMM\":500, \"yMaxMM\":500, \"pipelineLen\":100, "
    " \"stepEnablePin\":\"D4\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
    " \"cmdsAtStart\":\"\", "
    " \"axis0\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":10.0, \"maxAcc\":100.0,"
    " \"stepsPerRotation\":6400, \"unitsPerRotation\":360 },"
    " \"axis1\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":10.0, \"maxAcc\":100.0,"
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

static bool ptToActuator(AxisFloats& pt, AxisFloats& actuatorCoords, AxisParams axisParams[], int numAxes)
{
	bool isValid = true;
	for (int i = 0; i < RobotConsts::MAX_AXES; i++)
	{
		// Axis val from home point
		float axisValFromHome = pt.getVal(i) - axisParams[i]._homeOffsetVal;
		// Convert to steps and add offset to home in steps
		actuatorCoords.setVal(i, axisValFromHome * axisParams[i].stepsPerUnit()
			+ axisParams[i]._homeOffsetSteps);

		// Check machine bounds
		bool thisAxisValid = true;
		if (axisParams[i]._minValValid && pt.getVal(i) < axisParams[i]._minVal)
			thisAxisValid = false;
		if (axisParams[i]._maxValValid && pt.getVal(i) > axisParams[i]._maxVal)
			thisAxisValid = false;
		// Log.trace("ptToActuator (%s) %f -> %f (homeOffVal %f, homeOffSteps %ld)", thisAxisValid ? "OK" : "INVALID",
		// 	pt.getVal(i), actuatorCoords._pt[i], axisParams[i]._homeOffsetVal, axisParams[i]._homeOffsetSteps);
		isValid &= thisAxisValid;
	}
	return isValid;
}

static void actuatorToPt(AxisFloats& actuatorCoords, AxisFloats& pt, AxisParams axisParams[], int numAxes)
{
	for (int i = 0; i < RobotConsts::MAX_AXES; i++)
	{
		float ptVal = actuatorCoords.getVal(i) - axisParams[i]._homeOffsetSteps;
		ptVal = ptVal / axisParams[i].stepsPerUnit() + axisParams[i]._homeOffsetVal;
		if (axisParams[i]._minValValid && ptVal < axisParams[i]._minVal)
			ptVal = axisParams[i]._minVal;
		if (axisParams[i]._maxValValid && ptVal > axisParams[i]._maxVal)
			ptVal = axisParams[i]._maxVal;
		pt.setVal(i, ptVal);
		// Log.trace("actuatorToPt %d %f -> %f (perunit %f)", i, actuatorCoords.getVal(i), ptVal, axisParams[i].stepsPerUnit());
	}
}

static void correctStepOverflow(AxisParams axisParams[], int numAxes)
{
	// Not necessary for a non-continuous rotation bot
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
				{ {100,100} };

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
