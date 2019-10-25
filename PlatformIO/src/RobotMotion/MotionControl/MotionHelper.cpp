// RBotFirmware
// Rob Dobson 2016-18

#include "ConfigPinMap.h"
#include "math.h"
#include "MotionHelper.h"
#include "Utils.h"
#include "AxisValues.h"

// #define MOTION_LOG_DEBUG 1
// #define DEBUG_MOTION_HELPER 1

static const char* MODULE_PREFIX = "MotionHelper: ";

MotionHelper::MotionHelper() : 
            _trinamicsController(_axesParams, _motionPipeline),
            _rampGenerator(&_motionPipeline),
            _motionHoming(this)
{
    // Init
    _isPaused = false;
    _moveRelative = false;
    _blockDistanceMM = 0;
    _allowAllOutOfBounds = false;
    // Clear axis current location
    _lastCommandedAxisPos.clear();
    _rampGenerator.resetTotalStepPosition();
    _trinamicsController.resetTotalStepPosition();
    // Coordinate conversion management
    _ptToActuatorFn = NULL;
    _actuatorToPtFn = NULL;
    _correctStepOverflowFn = NULL;
    // Handling of splitting-up of motion into smaller blocks
    _blocksToAddTotal = 0;    
    // Init callbacks
    _ptToActuatorFn = nullptr;
    _actuatorToPtFn = nullptr;
    _correctStepOverflowFn = nullptr;
    _convertCoordsFn = nullptr;
    _setRobotAttributes = nullptr;
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
                                 correctStepOverflowFnType correctStepOverflowFn,
                                 convertCoordsFnType convertCoordsFn, setRobotAttributesFnType setRobotAttributes)
{
    // Store callbacks
    _ptToActuatorFn = ptToActuatorFn;
    _actuatorToPtFn = actuatorToPtFn;
    _correctStepOverflowFn = correctStepOverflowFn;
    _convertCoordsFn = convertCoordsFn;
    _setRobotAttributes = setRobotAttributes;
}

// Configure the robot and pipeline parameters using a JSON input string
void MotionHelper::configure(const char *robotConfigJSON)
{
    // Stop motion actuator
    _rampGenerator.stop();
    _trinamicsController.stop();
    
    // Config geometry
    String robotGeom = RdJson::getString("robotGeom", "NONE", robotConfigJSON);

    // Config settings
    int pipelineLen = int(RdJson::getLong("pipelineLen", pipelineLen_default, robotGeom.c_str()));
    _blockDistanceMM = float(RdJson::getDouble("blockDistanceMM", blockDistanceMM_default, robotGeom.c_str()));
    _allowAllOutOfBounds = bool(RdJson::getLong("allowOutOfBounds", false, robotGeom.c_str()));
    float junctionDeviation = float(RdJson::getDouble("junctionDeviation", junctionDeviation_default, robotGeom.c_str()));
    Log.notice("%sconfigMotionPipeline len %d, blockDistMM %F (0=no-max), allowOoB %s, jnDev %F\n", MODULE_PREFIX,
               pipelineLen, _blockDistanceMM, _allowAllOutOfBounds ? "Y" : "N", junctionDeviation);

    // Pipeline length and block size
    _motionPipeline.init(pipelineLen);

    // Motion Pipeline and Planner
    _motionPlanner.configure(junctionDeviation);

    // Clean up previous
    _trinamicsController.deinit();
    _rampGenerator.deinit();
    _motorEnabler.deinit();

    // Configure Axes
    _axesParams.clearAxes();
    String axisJSON;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        if (_axesParams.configureAxis(robotGeom.c_str(), axisIdx, axisJSON))
        {
            // Configure ramp generator - motors and end-stops
            _rampGenerator.configureAxis(axisIdx, axisJSON.c_str());
            // Configure ramp generator - motors and end-stops
            _trinamicsController.configureAxis(axisIdx, axisJSON.c_str());
        }
    }

    // Set the robot attributes
    if (_setRobotAttributes)
        _setRobotAttributes(_axesParams, _robotAttributes);

    // Homing
    _motionHoming.configure(robotGeom.c_str());    

    // Trinamic controller
    _trinamicsController.configure(robotGeom.c_str());

    // Motor enabler
    _motorEnabler.configure(robotGeom.c_str());

    // Start motion actuator
    _rampGenerator.configure(!_trinamicsController.isRampGenerator());

    // Clear motion info
    _lastCommandedAxisPos.clear();
    _rampGenerator.resetTotalStepPosition();
    _trinamicsController.resetTotalStepPosition();
}

// Check if a command can be accepted into the motion pipeline
bool MotionHelper::canAccept()
{
    // Check if homing in progress
    if (_motionHoming.isHomingInProgress())
        return false;
    // Check that the motion pipeline can accept new data
    return (_blocksToAddTotal == 0) && _motionPipeline.canAccept();
}

// Pause (or un-pause) all motion
void MotionHelper::pause(bool pauseIt)
{
    _rampGenerator.pause(pauseIt);
    _trinamicsController.pause(pauseIt);
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
    _blocksToAddTotal = 0;
    _stopRequested = true;
    _stopRequestTimeMs = millis();
    _rampGenerator.stop();
    _trinamicsController.stop();
    _motionPipeline.clear();
    pause(false);
    setCurPosActualPosition();
 }

// Check if idle
bool MotionHelper::isIdle()
{
    return !_motionPipeline.canGet();
}

void MotionHelper::setCurPosActualPosition()
{
    // Get final position of actuator after a short delay to attempt to
    // ensure any final step is completed
    delayMicroseconds(100);
    AxisInt32s actuatorPos;
    if (_trinamicsController.isRampGenerator())
        _trinamicsController.getTotalStepPosition(actuatorPos);
    else
        _rampGenerator.getTotalStepPosition(actuatorPos);
    AxisFloats curPosMM;
    if (_actuatorToPtFn)
        _actuatorToPtFn(actuatorPos, curPosMM, _lastCommandedAxisPos, _axesParams);
    _lastCommandedAxisPos._axisPositionMM = curPosMM;
    _lastCommandedAxisPos._stepsFromHome = actuatorPos;
#ifdef DEBUG_MOTION_HELPER
    Log.trace("%sstop absAxes X%F Y%F Z%F steps %d,%d,%d\n", MODULE_PREFIX,
                _lastCommandedAxisPos._axisPositionMM.getVal(0),
                _lastCommandedAxisPos._axisPositionMM.getVal(1),
                _lastCommandedAxisPos._axisPositionMM.getVal(2),
                _lastCommandedAxisPos._stepsFromHome.getVal(0),
                _lastCommandedAxisPos._stepsFromHome.getVal(1),
                _lastCommandedAxisPos._stepsFromHome.getVal(2));
#endif
}

// Set parameters such as relative vs absolute motion
void MotionHelper::setMotionParams(RobotCommandArgs &args)
{
    // Check for relative movement specified and set accordingly
    if (args.getMoveType() != RobotMoveTypeArg_None)
        _moveRelative = (args.getMoveType() == RobotMoveTypeArg_Relative);
}

// Get current status of robot
void MotionHelper::getCurStatus(RobotCommandArgs &args)
{
    // Get current position
    AxisInt32s curActuatorPos;
    if (_trinamicsController.isRampGenerator())
        _trinamicsController.getTotalStepPosition(curActuatorPos);
    else
        _rampGenerator.getTotalStepPosition(curActuatorPos);
    args.setPointSteps(curActuatorPos);
    // Use reverse kinematics to get location
    AxisFloats curMMPos;
    if (_actuatorToPtFn)
        _actuatorToPtFn(curActuatorPos, curMMPos, _lastCommandedAxisPos, _axesParams);
    args.setPointMM(curMMPos);
    // Get end-stop values
    AxisMinMaxBools endstops;
    _rampGenerator.getEndStopStatus(endstops);
    args.setEndStops(endstops);
    // Absolute/Relative movement
    args.setMoveType(_moveRelative ? RobotMoveTypeArg_Relative : RobotMoveTypeArg_Absolute);
    // flags
    args.setPause(_isPaused);
    args.setIsHoming(_motionHoming.isHomingInProgress());
    args.setHasHomed(_motionHoming.isHomedOk());
    // Queue length
    args.setNumQueued(_motionPipeline.count());
}

// Get attributes of robot
void MotionHelper::getRobotAttributes(String& robotAttrs)
{
    robotAttrs = _robotAttributes;
}

// Command the robot to home one or more axes
void MotionHelper::goHome(RobotCommandArgs &args)
{
    _motionHoming.homingStart(args);
}

// Command the robot to move (adding a command to the pipeline of motion)
bool MotionHelper::moveTo(RobotCommandArgs &args)
{
    // Handle stepwise motion
    if (args.isStepwise())
    {
        return _motionPlanner.moveToStepwise(args, _lastCommandedAxisPos, _axesParams, _motionPipeline);
    }
    // Convert coordinates if required
    // Convert coords to MM (in-place conversion)
    if (_convertCoordsFn)
        _convertCoordsFn(args, _axesParams);
    // Fill in the destPos for axes for which values not specified
    // Handle relative motion override if present
    // Don't use servo values for computing distance to travel
    AxisFloats destPos = args.getPointMM();
    bool includeDist[RobotConsts::MAX_AXES];
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
        if (!args.isValid(i))
        {
            destPos.setVal(i, _lastCommandedAxisPos._axisPositionMM.getVal(i));
#ifdef DEBUG_MOTION_HELPER
            Log.notice("%smoveTo ax %d, pos %F NoMovementOnThisAxis\n", MODULE_PREFIX, 
                    i, 
                    destPos.getVal(i));
#endif
        }
        else
        {
            // Check relative motion - override current options if this command
            // explicitly states a moveType
            bool moveRelative = _moveRelative;
            if (args.getMoveType() != RobotMoveTypeArg_None)
                moveRelative = (args.getMoveType() == RobotMoveTypeArg_Relative);
            if (moveRelative)
                destPos.setVal(i, _lastCommandedAxisPos._axisPositionMM.getVal(i) + args.getValMM(i));
#ifdef DEBUG_MOTION_HELPER
            Log.notice("%smoveTo ax %d, pos %F relative %s\n", MODULE_PREFIX, 
                    i, 
                    destPos.getVal(i), 
                    moveRelative ? "Y" : "N");
#endif
        }
        includeDist[i] = _axesParams.isPrimaryAxis(i);
    }

    // Split up into blocks of maximum length
    double lineLen = destPos.distanceTo(_lastCommandedAxisPos._axisPositionMM, includeDist);

    // Ensure at least one block
    int numBlocks = 1;
    if (_blockDistanceMM > 0.01f && !args.getDontSplitMove())
        numBlocks = int(ceil(lineLen / _blockDistanceMM));
    if (numBlocks == 0)
        numBlocks = 1;
#ifdef DEBUG_MOTION_HELPER
    Log.trace("%smoveTo curX %F(%d) curY %F(%d) curZ %F(%d) newX %F newY %F newZ %F numBlocks %d (lineLen %F / blockDistMM %F)\n", MODULE_PREFIX,
                _lastCommandedAxisPos._axisPositionMM.getVal(0),
                _lastCommandedAxisPos._stepsFromHome.getVal(0),
                _lastCommandedAxisPos._axisPositionMM.getVal(1),
                _lastCommandedAxisPos._stepsFromHome.getVal(1),
                _lastCommandedAxisPos._axisPositionMM.getVal(2),
                _lastCommandedAxisPos._stepsFromHome.getVal(2),
                destPos.getVal(0),
                destPos.getVal(1),
                destPos.getVal(2),
                numBlocks, 
                lineLen, 
                _blockDistanceMM);
#endif

    // Setup for adding blocks to the pipe
    _blocksToAddCommandArgs = args;
    _blocksToAddStartPos = _lastCommandedAxisPos._axisPositionMM;
    _blocksToAddDelta = (destPos - _lastCommandedAxisPos._axisPositionMM) / float(numBlocks);
    _blocksToAddEndPos = destPos;
    _blocksToAddCurBlock = 0;
    _blocksToAddTotal = numBlocks;

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

        // Prepare add to planner
        _blocksToAddCommandArgs.setPointMM(nextBlockDest);
        _blocksToAddCommandArgs.setMoreMovesComing(_blocksToAddTotal != 0);


        // Add to planner
        addToPlanner(_blocksToAddCommandArgs);

        // Enable motors
        _motorEnabler.enableMotors(true, false);
    }
}

// Add a movement to the pipeline using the planner which computes suitable motion
bool MotionHelper::addToPlanner(RobotCommandArgs &args)
{
    // Check we are not stopping
    if (_stopRequested)
        return false;
            
    // Convert the move to actuator coordinates
    AxisFloats actuatorCoords;
    bool moveOk = false;
    if (_ptToActuatorFn)
        moveOk = _ptToActuatorFn(args.getPointMM(), actuatorCoords, _lastCommandedAxisPos, _axesParams,
                    args.getAllowOutOfBounds() || _allowAllOutOfBounds);

    // Plan the move
    if (moveOk)
    {
        moveOk = _motionPlanner.moveTo(args, actuatorCoords, _lastCommandedAxisPos, _axesParams, _motionPipeline);
#ifdef MOTION_LOG_DEBUG
    Log.trace("~M%d %d %d %F %F OOB %d %d\n", millis(), int(actuatorCoords.getVal(0)), 
            int(actuatorCoords.getVal(1)), 
            args.getPointMM().getVal(0), args.getPointMM().getVal(1),
            args.getAllowOutOfBounds(), _allowAllOutOfBounds);
#endif
    }
    if (moveOk)
    {
        // Update axisMotion
        _lastCommandedAxisPos._axisPositionMM = args.getPointMM();
#ifdef DEBUG_MOTION_HELPER
        Log.trace("%saddToPlanner updatedAxisPos X%F Y%F Z%F\n", MODULE_PREFIX,
                        _lastCommandedAxisPos._axisPositionMM.getVal(0),
                        _lastCommandedAxisPos._axisPositionMM.getVal(1),
                        _lastCommandedAxisPos._axisPositionMM.getVal(2)
                    );
#endif
        // Correct overflows
        if (_correctStepOverflowFn)
        {
            _correctStepOverflowFn(_lastCommandedAxisPos, _axesParams);
#ifdef MOTION_LOG_DEBUG
    Log.trace("~A%d %d\n", _lastCommandedAxisPos._stepsFromHome.getVal(0), 
            _lastCommandedAxisPos._stepsFromHome.getVal(1));
#endif
        }            
    }
    return moveOk;
}

// Called regularly to allow the MotionHelper to do background work such as
// adding split-up blocks to the pipeline and checking if motors should be
// disabled after a period of no motion
void MotionHelper::service()
{
    // Check if stop requested
    if (_stopRequested)
    {
        if (Utils::isTimeout(millis(), _stopRequestTimeMs, MAX_TIME_BEFORE_STOP_COMPLETE_MS))
        {
            _blocksToAddTotal = 0;
            _rampGenerator.stop();
            _trinamicsController.stop();
            _motionPipeline.clear();
            pause(false);
            setCurPosActualPosition();
            _stopRequested = false;
        }
    }

    // Call process on motion actuator - only really used for testing as
    // motion is handled by ISR
    _rampGenerator.process();

    // Process for trinamic devices
    _trinamicsController.process();

    // Process any split-up blocks to be added to the pipeline
    blocksToAddProcess();

    // Service homing
    _motionHoming.service(_axesParams);

    // Ensure motors enabled when homing or moving
    if ((_motionPipeline.count() > 0) || _motionHoming.isHomingInProgress())
    {
        _motorEnabler.enableMotors(true, false);
    }
}

// Set home coordinates
void MotionHelper::setCurPositionAsHome(int axisIdx)
{
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
        return;
    _lastCommandedAxisPos._axisPositionMM.setVal(axisIdx, _axesParams.getHomeOffsetVal(axisIdx));
    _lastCommandedAxisPos._stepsFromHome.setVal(axisIdx, _axesParams.gethomeOffSteps(axisIdx));
    _rampGenerator.setTotalStepPosition(axisIdx, _axesParams.gethomeOffSteps(axisIdx));
    _trinamicsController.setTotalStepPosition(axisIdx, _axesParams.gethomeOffSteps(axisIdx));
#ifdef DEBUG_MOTION_HELPER
    Log.trace("%ssetCurPosAsHome curMM X%F Y%F Z%F steps %d,%d,%d\n", MODULE_PREFIX,
                _lastCommandedAxisPos._axisPositionMM.getVal(0),
                _lastCommandedAxisPos._axisPositionMM.getVal(1),
                _lastCommandedAxisPos._axisPositionMM.getVal(2),
                _lastCommandedAxisPos._stepsFromHome.getVal(0),
                _lastCommandedAxisPos._stepsFromHome.getVal(1),
                _lastCommandedAxisPos._stepsFromHome.getVal(2));
#endif
}

// Debug helper methods
void MotionHelper::debugShowBlocks()
{
    _motionPipeline.debugShowBlocks(_axesParams);
}

void MotionHelper::debugShowTopBlock()
{
    _motionPipeline.debugShowTopBlock(_axesParams);
}

String MotionHelper::getDebugStr()
{
    return _rampGenerator.getDebugStr();
}

int MotionHelper::testGetPipelineCount()
{
    return _motionPipeline.count();
}

bool MotionHelper::testGetPipelineBlock(int elIdx, MotionBlock &block)
{
    if ((int)_motionPipeline.count() <= elIdx)
        return false;
    block = *_motionPipeline.peekNthFromPut(_motionPipeline.count() - 1 - elIdx);
    return true;
}
