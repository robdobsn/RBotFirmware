
#include "MotionHelper.h"
#include "Utils.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

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
  " \"axis0\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":10.0, \"maxAcc\":10.0,"
  " \"stepsPerRotation\":6400, \"unitsPerRotation\":32 },"
  " \"axis1\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":10.0, \"maxAcc\":10.0,"
  " \"stepsPerRotation\":1600, \"unitsPerRotation\":32 },"
  " \"commandQueue\": { \"cmdQueueMaxLen\":50 } "
  "}";

#else

static const char* ROBOT_CONFIG_STR =
  "{\"robotType\": \"AxiBot\", \"xMaxMM\":500, \"yMaxMM\":500, \"pipelineLen\":100, "
  " \"stepEnablePin\":\"D4\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
  " \"cmdsAtStart\":\"\", "
  " \"axis0\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":50.0, \"maxAcc\":10.0,"
  " \"stepsPerRotation\":3200, \"unitsPerRotation\":32 },"
  " \"axis1\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":50.0, \"maxAcc\":10.0,"
  " \"stepsPerRotation\":3200, \"unitsPerRotation\":32 },"
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
  //  pt.getVal(axisIdx), actuatorCoords._pt[axisIdx],
  //  axesParams.getHomeOffsetVal(axisIdx), axesParams.getHomeOffsetSteps(axisIdx));

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
  return(fabs(a - b) < epsilon);
}

bool isApproxL(int64_t a, int64_t b, int64_t epsilon = 1)
{
  return(llabs(a - b) < epsilon);
}

MotionHelper _motionHelper;

int          __curTestNum                                      = 0;
const int    __numTests                                        = 2;
int          __curTestErrorCount                               = 0;
const int    __testSquareDiagonalLen                           = 6;
int          __testSquareDiagonal [__testSquareDiagonalLen][2] =
{ { 100, 0 }, { 100, 100 }, { 0, 100 }, { 0, 0 }, { 100, 100 }, { 0, 0 } };
const int    __testOneBigMoveLen                       = 2;
int          __testOneBigMove [__testOneBigMoveLen][2] =
{ { 64, 64 }, { 0, 0 } };

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
  Log.info("=====================================================================");
  String systemName = "TEST PIPELINE PLANNER PHOTON";
  Log.info("%s (built %s %s)", systemName.c_str(), __DATE__, __TIME__);
  Log.info("========================== TESTING PIPELINE ==========================");

  _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
  _motionHelper.configure(ROBOT_CONFIG_STR);
  _motionHelper.setTestMode("TIMEISR");
  _motionHelper.pause(false);
}

bool          __debugTimingShown = true;
bool          __wasIdle          = false;
unsigned long __idleTime         = 0;
unsigned long __timeStartedTest  = millis();

void loop()
{
  while (true)
  {
    _motionHelper.service(true);
    if (_motionHelper.isIdle() || Utils::isTimeout(millis(), __timeStartedTest, 60000))
    {
      if (!__debugTimingShown)
      {
        _motionHelper.debugShowTiming();
        __debugTimingShown = true;
      }
      if (!__wasIdle)
      {
        __idleTime = millis();
        __wasIdle  = true;
      }
      else
      {
        if (Utils::isTimeout(millis(), __idleTime, 5000) || (__curTestNum == 0))
        {
          _motionHelper.pause(true);
          if (setupNextTest())
          {
            __debugTimingShown = false;
            __timeStartedTest  = millis();
          }
          __wasIdle = false;
          _motionHelper.pause(false);
        }
      }
    }
  }
}
