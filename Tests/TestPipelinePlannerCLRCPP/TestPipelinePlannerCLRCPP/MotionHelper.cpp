// RBotFirmware
// Rob Dobson 2016-18

#include "application.h"
#include "ConfigPinMap.h"
#include "math.h"
#include "MotionHelper.h"
#include "Utils.h"

MotionHelper::MotionHelper() :
  _motionActuator(_motionIO, _motionPipeline)
{
  // Init
  _isPaused        = false;
  _moveRelative    = false;
  _xMaxMM          = 0;
  _yMaxMM          = 0;
  _blockDistanceMM = 0;
  // Clear axis current location
  _curAxisPosition.clear();
  // Coordinate conversion management
  _ptToActuatorFn        = NULL;
  _actuatorToPtFn        = NULL;
  _correctStepOverflowFn = NULL;
  // Handling of splitting-up of motion into smaller blocks
  _blocksToAddTotal = 0;
}

// Destructor
MotionHelper::~MotionHelper()
{
}

// Each robot has a set of functions that transform points from real-world coordinates
// to actuator coordinates
// There is also a function to correct step overflow which is important in robots
// which have continuous rotation as step counts would otherwise overflow 32bit integer values
void MotionHelper::setTransforms(ptToActuatorFnType ptToActuatorFn, actuatorToPtFnType actuatorToPtFn,
                                 correctStepOverflowFnType correctStepOverflowFn)
{
  // Store callbacks
  _ptToActuatorFn        = ptToActuatorFn;
  _actuatorToPtFn        = actuatorToPtFn;
  _correctStepOverflowFn = correctStepOverflowFn;
}

// Configure the robot and pipeline parameters using a JSON input string
void MotionHelper::configure(const char* robotConfigJSON)
{
  // MotionIO
  _motionIO.deinit();

  // Configure Axes
  _axesParams.clearAxes();
  String axisJSON;
  for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
  {
    if (_axesParams.configureAxis(robotConfigJSON, axisIdx, axisJSON))
    {
      // Configure motionIO - motors and end-stops
      _motionIO.configureAxis(axisJSON.c_str(), axisIdx);
    }
  }

  // Configure robot
  configureRobot(robotConfigJSON);

  // Give the MotionActuator access to raw motionIO info
  // this enables ISR based motion to be faster
  RobotConsts::RawMotionHwInfo_t rawMotionHwInfo;
  _motionIO.getRawMotionHwInfo(rawMotionHwInfo);
  _motionActuator.setRawMotionHwInfo(rawMotionHwInfo);

  // Clear motion info
  _curAxisPosition.clear();
}

// Configure the robot itself
bool MotionHelper::configureRobot(const char* robotConfigJSON)
{
  _xMaxMM = float(RdJson::getDouble("xMaxMM", 0, robotConfigJSON));
  _yMaxMM = float(RdJson::getDouble("yMaxMM", 0, robotConfigJSON));

  // Motion Pipeline and Planner
  _blockDistanceMM = float(RdJson::getDouble("blockDistanceMM", blockDistanceMM_default, robotConfigJSON));
  float junctionDeviation = float(RdJson::getDouble("junctionDeviation", junctionDeviation_default, robotConfigJSON));
  int   pipelineLen       = int(RdJson::getLong("pipelineLen", pipelineLen_default, robotConfigJSON));
  // Pipeline length and block size
  Log.info("MotionHelper configMotionPipeline len %d, _blockDistanceMM %0.2f (0=no-max)",
           pipelineLen, _blockDistanceMM);
  _motionPipeline.init(pipelineLen);
  _motionPlanner.configure(junctionDeviation);

  // MotionIO
  _motionIO.configureMotors(robotConfigJSON);
  return true;
}

// Check if a command can be accepted into the motion pipeline
bool MotionHelper::canAccept()
{
  // Check that the motion pipeline can accept new data
  return (_blocksToAddTotal == 0) && _motionPipeline.canAccept();
}

// Pause (or un-pause) all motion
void MotionHelper::pause(bool pauseIt)
{
  _motionActuator.pause(pauseIt);
  _isPaused = pauseIt;
}

// Check if paused
bool MotionHelper::isPaused()
{
  return _isPaused;
}

// Stop
void MotionHelper::stop()
{
  _motionPipeline.clear();
  _motionActuator.clear();
  _isPaused = false;
}

// Check if idle
bool MotionHelper::isIdle()
{
  return !_motionPipeline.canGet();
}

// Set parameters such as relative vs absolute motion
void MotionHelper::setMotionParams(RobotCommandArgs& args)
{
  // Check for relative movement specified and set accordingly
  if (args.moveType != RobotMoveTypeArg_None)
    _moveRelative = (args.moveType == RobotMoveTypeArg_Relative);
}

// Get current status of robot
void MotionHelper::getCurStatus(RobotCommandArgs& args)
{
  // Get current position
  AxisFloats axisPosns;
  args.pt = _curAxisPosition._axisPositionMM;
  // Absolute/Relative movement
  args.moveType = _moveRelative ? RobotMoveTypeArg_Relative : RobotMoveTypeArg_Absolute;
}

// Command the robot to move (adding a command to the pipeline of motion)
bool MotionHelper::moveTo(RobotCommandArgs& args)
{
  // Handle any motion parameters (such as relative movement, feedrate, etc)
  setMotionParams(args);

  // Handle relative motion and fill in the destPos for axes for
  // which values not specified
  // Don't use servo values for computing distance to travel
  AxisFloats destPos = args.pt;
  bool       includeDist[RobotConsts::MAX_AXES];
  for (int i = 0; i < RobotConsts::MAX_AXES; i++)
  {
    if (!args.pt.isValid(i))
    {
      destPos.setVal(i, _curAxisPosition._axisPositionMM.getVal(i));
    }
    else
    {
      if (_moveRelative)
        destPos.setVal(i, _curAxisPosition._axisPositionMM.getVal(i) + args.pt.getVal(i));
    }
    includeDist[i] = _axesParams.isPrimaryAxis(i);
  }

  // Split up into blocks of maximum length
  double lineLen = destPos.distanceTo(_curAxisPosition._axisPositionMM, includeDist);

  // Ensure at least one block
  int numBlocks = 1;
  if (_blockDistanceMM > 0.01f)
    numBlocks = int(lineLen / _blockDistanceMM);
  if (numBlocks == 0)
    numBlocks = 1;
  // Log.trace("MotionHelper numBlocks %d (lineLen %0.2f / blockDistMM %02.f)",
  //  numBlocks, lineLen, _blockDistanceMM);

  // Setup for adding blocks to the pipe
  _blocksToAddCommandArgs = args;
  _blocksToAddStartPos    = _curAxisPosition._axisPositionMM;
  _blocksToAddDelta       = (destPos - _curAxisPosition._axisPositionMM) / float(numBlocks);
  _blocksToAddEndPos      = destPos;
  _blocksToAddCurBlock    = 0;
  _blocksToAddTotal       = numBlocks;

  // Process anything that can be done immediately
  blocksToAddProcess();
  return true;
}

// Add a movement to the pipeline using the planner which computes suitable motion
bool MotionHelper::addToPlanner(RobotCommandArgs& args)
{
  // Convert the move to actuator coordinates
  AxisFloats actuatorCoords;
  _ptToActuatorFn(args.pt, actuatorCoords, _axesParams);

  // Plan the move
  bool moveOk = _motionPlanner.moveTo(args, actuatorCoords, _curAxisPosition, _axesParams, _motionPipeline);
  if (moveOk)
  {
    // Update axisMotion
    _curAxisPosition._axisPositionMM = args.pt;
  }
  return moveOk;
}

// A single moveTo command can be split into blocks - this function checks if such
// splitting is in progress and adds the split-up motion blocks accordingly
void MotionHelper::blocksToAddProcess()
{
  // Check if we can add anything to the pipeline
  while (_motionPipeline.canAccept())
  {
    // Check if any blocks remain to be expanded out
    if (_blocksToAddTotal <= 0)
      return;

    // Add to pipeline any blocks that are waiting to be expanded out
    AxisFloats nextBlockDest = _blocksToAddStartPos + _blocksToAddDelta * float(_blocksToAddCurBlock + 1);

    // If last block then just use end point coords
    if (_blocksToAddCurBlock + 1 >= _blocksToAddTotal)
      nextBlockDest = _blocksToAddEndPos;

    // Bump position
    _blocksToAddCurBlock++;

    // Check if done
    if (_blocksToAddCurBlock >= _blocksToAddTotal)
      _blocksToAddTotal = 0;

    // Add to planner
    _blocksToAddCommandArgs.pt = nextBlockDest;
    addToPlanner(_blocksToAddCommandArgs);

    // Enable motors
    _motionIO.enableMotors(true, false);
  }
}

// Called regularly to allow the MotionHelper to do background work such as
// adding split-up blocks to the pipeline and checking if motors should be
// disabled after a period of no motion
void MotionHelper::service(bool processPipeline)
{
  // Check if we should process the movement pipeline
  if (processPipeline)
  {
    _motionActuator.process();
  }

  // Process any split-up blocks to be added to the pipeline
  blocksToAddProcess();

  // Service MotionIO
  if (_motionPipeline.count() > 0)
    _motionIO.motionIsActive();
  _motionIO.service();
}

// Reset coordinates
void MotionHelper::setCurPositionAsHome(AxisFloats& pt)
{
  _axesParams.setHomePosition(pt, _curAxisPosition._stepsFromHome);
}

// Reset coordinates
void MotionHelper::setCurPositionAsHome(bool xIsHome, bool yIsHome, bool zIsHome)
{
  AxisFloats pt(0, 0, 0, xIsHome, yIsHome, zIsHome);
  _axesParams.setHomePosition(pt, _curAxisPosition._stepsFromHome);
}

// Debug helper methods
void MotionHelper::debugShowBlocks()
{
  _motionPipeline.debugShowBlocks(_axesParams);
}

void MotionHelper::debugShowTiming()
{
  _motionActuator.showDebug();
}

int MotionHelper::testGetPipelineCount()
{
  return _motionPipeline.count();
}

bool MotionHelper::testGetPipelineBlock(int elIdx, MotionBlock& block)
{
  if ((int) _motionPipeline.count() <= elIdx)
    return false;
  block = *_motionPipeline.peekNthFromPut(_motionPipeline.count() - 1 - elIdx);
  return true;
}
