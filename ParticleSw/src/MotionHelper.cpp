// RBotFirmware
// Rob Dobson 2016-18

#include "application.h"
#include "ConfigPinMap.h"
#include "math.h"
#include "MotionHelper.h"
#include "Utils.h"

MotionHelper::MotionHelper() :
  _motionActuator(_motionIO, _motionPipeline),
  _motionHoming(this)
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
  // Pipeline length and block size
  int pipelineLen = int(RdJson::getLong("pipelineLen", pipelineLen_default, robotConfigJSON));
  _blockDistanceMM = float(RdJson::getDouble("blockDistanceMM", blockDistanceMM_default, robotConfigJSON));
  Log.info("MotionHelper configMotionPipeline len %d, _blockDistanceMM %0.2f (0=no-max)",
           pipelineLen, _blockDistanceMM);
  _motionPipeline.init(pipelineLen);

  // Motion Pipeline and Planner
  float junctionDeviation = float(RdJson::getDouble("junctionDeviation", junctionDeviation_default, robotConfigJSON));
  _motionPlanner.configure(junctionDeviation);

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
  _xMaxMM = float(RdJson::getDouble("xMaxMM", 0, robotConfigJSON));
  _yMaxMM = float(RdJson::getDouble("yMaxMM", 0, robotConfigJSON));

  // Homing
  _motionHoming.configure(robotConfigJSON);

  // MotionIO
  _motionIO.configureMotors(robotConfigJSON);

  // Give the MotionActuator access to raw motionIO info
  // this enables ISR based motion to be faster
  RobotConsts::RawMotionHwInfo_t rawMotionHwInfo;
  _motionIO.getRawMotionHwInfo(rawMotionHwInfo);
  _motionActuator.setRawMotionHwInfo(rawMotionHwInfo);

  // Clear motion info
  _curAxisPosition.clear();
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
  pause(false);
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
  if (args.getMoveType() != RobotMoveTypeArg_None)
    _moveRelative = (args.getMoveType() == RobotMoveTypeArg_Relative);
}

// Get current status of robot
void MotionHelper::getCurStatus(RobotCommandArgs& args)
{
  // Get current position
  args.setPointMM(_curAxisPosition._axisPositionMM);
  args.setPointSteps(_curAxisPosition._stepsFromHome);
  // Get end-stop values
  args.setEndStops(_motionIO.getEndStopVals());
  // Absolute/Relative movement
  args.setMoveType(_moveRelative ? RobotMoveTypeArg_Relative : RobotMoveTypeArg_Absolute);
  // paused
  args.setPause(_isPaused);
  // Queue length
  args.setNumQueued(_motionPipeline.count());
}

// Command the robot to home one or more axes
void MotionHelper::goHome(RobotCommandArgs& args)
{
    _motionHoming.homingStart(args);
}

// Command the robot to move (adding a command to the pipeline of motion)
bool MotionHelper::moveTo(RobotCommandArgs& args)
{
  // Handle stepwise motion
  if (args.isStepwise())
  {
    _motionPlanner.moveToStepwise(args, _curAxisPosition, _axesParams, _motionPipeline);
  }
  // Fill in the destPos for axes for which values not specified
  // Handle relative motion override if present
  // Don't use servo values for computing distance to travel
  AxisFloats destPos = args.getPointMM();
  bool       includeDist[RobotConsts::MAX_AXES];
  for (int i = 0; i < RobotConsts::MAX_AXES; i++)
  {
    if (!args.isValid(i))
    {
      destPos.setVal(i, _curAxisPosition._axisPositionMM.getVal(i));
      // Log.info("MOVE TO ax %d, pos %d", i, _curAxisPosition._axisPositionMM.getVal(i));
    }
    else
    {
      // Check relative motion - override current options if this command
      // explicitly states a moveType
      bool moveRelative = _moveRelative;
      if (args.getMoveType() != RobotMoveTypeArg_None)
        moveRelative = (args.getMoveType() == RobotMoveTypeArg_Relative);
      if (moveRelative)
        destPos.setVal(i, _curAxisPosition._axisPositionMM.getVal(i) + args.getValMM(i));
    }
    includeDist[i] = _axesParams.isPrimaryAxis(i);
  }

  // Split up into blocks of maximum length
  double lineLen = destPos.distanceTo(_curAxisPosition._axisPositionMM, includeDist);

  // Ensure at least one block
  int numBlocks = 1;
  if (_blockDistanceMM > 0.01f && !args.getDontSplitMove())
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
    _blocksToAddCommandArgs.setPointMM(nextBlockDest);
    addToPlanner(_blocksToAddCommandArgs);

    // Enable motors
    _motionIO.enableMotors(true, false);
  }
}

// Add a movement to the pipeline using the planner which computes suitable motion
bool MotionHelper::addToPlanner(RobotCommandArgs& args)
{
  // Convert the move to actuator coordinates
  AxisFloats actuatorCoords;
  _ptToActuatorFn(args.getPointMM(), actuatorCoords, _curAxisPosition, _axesParams, args.getAllowOutOfBounds());

  // Plan the move
  bool moveOk = _motionPlanner.moveTo(args, actuatorCoords, _curAxisPosition, _axesParams, _motionPipeline);
  if (moveOk)
  {
    // Update axisMotion
    _curAxisPosition._axisPositionMM = args.getPointMM();
  }
  return moveOk;
}

// Called regularly to allow the MotionHelper to do background work such as
// adding split-up blocks to the pipeline and checking if motors should be
// disabled after a period of no motion
void MotionHelper::service()
{
  // Call process on motion actuator - only really used for testing as
  // motion is handled by ISR
  _motionActuator.process();

  // Process any split-up blocks to be added to the pipeline
  blocksToAddProcess();

  // Service MotionIO
  if (_motionPipeline.count() > 0)
    _motionIO.motionIsActive();
  _motionIO.service();

  // Service homing
  _motionHoming.service(_axesParams);
  if (_motionHoming.isHomingInProgress())
    _motionIO.motionIsActive();

}

// Set home coordinates
void MotionHelper::setCurPositionAsHome(int axisIdx)
{
  if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
    return;
  _curAxisPosition._axisPositionMM.setVal(axisIdx, _axesParams.getHomeOffsetVal(axisIdx));
  _curAxisPosition._stepsFromHome.setVal(axisIdx, _axesParams.gethomeOffSteps(axisIdx));
}

// Debug helper methods
void MotionHelper::debugShowBlocks()
{
  _motionPipeline.debugShowBlocks(_axesParams);
}

String MotionHelper::getDebugStr()
{
  return _motionActuator.getDebugStr();
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
