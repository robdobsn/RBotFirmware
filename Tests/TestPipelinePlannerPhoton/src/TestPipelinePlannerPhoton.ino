
#include "MotionHelper.h"

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_TRACE);

static const char* ROBOT_CONFIG_STR_XY =
	"{\"robotType\": \"XYBot\", \"xMaxMM\":500, \"yMaxMM\":500, \"pipelineLen\":100, "
    " \"stepEnablePin\":\"A2\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
    " \"cmdsAtStart\":\"\", "
    " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":100.0, \"maxAcc\":100.0,"
    " \"stepsPerRotation\":3200, \"unitsPerRotation\":60 },"
    " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":100.0, \"maxAcc\":100.0,"
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
		Log.trace("ptToActuator (%s) %f -> %f (homeOffVal %f, homeOffSteps %ld)", thisAxisValid ? "OK" : "INVALID",
			pt.getVal(i), actuatorCoords._pt[i], axisParams[i]._homeOffsetVal, axisParams[i]._homeOffsetSteps);
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
		Log.trace("actuatorToPt %d %f -> %f (perunit %f)", i, actuatorCoords.getVal(i), ptVal, axisParams[i].stepsPerUnit());
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

void setup()
{
	pinMode(D7, OUTPUT);
	Serial.begin(115200);
	delay(5000);
	Serial.println("STARTING");
	delay(5000);
	Log.trace("");
	Log.trace("========================== STARTING TEST ==========================");
	int errorCount = 0;

	const int TEST_NUM_MOTION_BLOCKS = 6;

	_motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
	_motionHelper.configure(ROBOT_CONFIG_STR_XY);
	_motionHelper.pause(false);

	RobotCommandArgs cmdArgs;
	cmdArgs.setAxisValue(0, 1, true);
	cmdArgs.setAxisValue(1, 0, true);
	_motionHelper.moveTo(cmdArgs);
	cmdArgs.setAxisValue(0, 1, true);
	cmdArgs.setAxisValue(1, 1, true);
	_motionHelper.moveTo(cmdArgs);
	cmdArgs.setAxisValue(0, 0, true);
	cmdArgs.setAxisValue(1, 1, true);
	_motionHelper.moveTo(cmdArgs);
	cmdArgs.setAxisValue(0, 0, true);
	cmdArgs.setAxisValue(1, 0, true);
	_motionHelper.moveTo(cmdArgs);
	cmdArgs.setAxisValue(0, 1, true);
	cmdArgs.setAxisValue(1, 1, true);
	_motionHelper.moveTo(cmdArgs);
	cmdArgs.setAxisValue(0, 0, true);
	cmdArgs.setAxisValue(1, 0, true);
	_motionHelper.moveTo(cmdArgs);

	if (_motionHelper.testGetPipelineCount() != TEST_NUM_MOTION_BLOCKS)
	{
	    Log.info("ERROR Pipeline len != Gcode count\n");
	    errorCount++;
	}

	// for (int i = 0; i < TEST_NUM_MOTION_BLOCKS; i++)
	// {
	//     MotionBlock elem;
	//     _motionHelper.testGetPipelineBlock(i, elem);
    //     testMotionElemVals(errorCount, elem, );
	// }

	_motionHelper.debugShowBlocks();

	if (errorCount != 0)
	{
	    Log.info("-------------ERRORS---------------");
	}
	Log.info("Test Case error count %d", errorCount);

}

void loop()
{
	while (true)
	{
	    _motionHelper.service(true);
	    if (_motionHelper.isIdle())
	    {
	        // testCompleted();
	    }
	}
}
