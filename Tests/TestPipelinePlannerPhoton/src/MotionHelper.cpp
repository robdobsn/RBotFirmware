// RBotFirmware
// Rob Dobson 2016-8

#include "application.h"
#include "ConfigPinMap.h"
#include "math.h"
#include "MotionHelper.h"
#include "Utils.h"

MotionHelper::MotionHelper() :
		_motionActuator(_motionIO, _motionPipeline)
{
	// Init
	_isPaused = false;
	_moveRelative = false;
    _xMaxMM = 0;
    _yMaxMM = 0;
	_blockDistanceMM = 0;
	// Clear axis current location
	_curAxisPosition.clear();
	// Coordinate conversion management
	_ptToActuatorFn = NULL;
	_actuatorToPtFn = NULL;
	_correctStepOverflowFn = NULL;
}

MotionHelper::~MotionHelper()
{
}

void MotionHelper::setTransforms(ptToActuatorFnType ptToActuatorFn, actuatorToPtFnType actuatorToPtFn,
	correctStepOverflowFnType correctStepOverflowFn)
{
	// Store callbacks
	_ptToActuatorFn = ptToActuatorFn;
	_actuatorToPtFn = actuatorToPtFn;
	_correctStepOverflowFn = correctStepOverflowFn;
}

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

	// Clear motion info
	_curAxisPosition.clear();
}

bool MotionHelper::configureRobot(const char* robotConfigJSON)
{
	// Get motor enable info
	String stepEnablePinName = RdJson::getString("stepEnablePin", "-1", robotConfigJSON);
	_xMaxMM = float(RdJson::getDouble("xMaxMM", 0, robotConfigJSON));
	_yMaxMM = float(RdJson::getDouble("yMaxMM", 0, robotConfigJSON));

	// Motion Pipeline and Planner
	_blockDistanceMM = float(RdJson::getDouble("blockDistanceMM", blockDistanceMM_default, robotConfigJSON));
	float junctionDeviation = float(RdJson::getDouble("junctionDeviation", junctionDeviation_default, robotConfigJSON));
	int pipelineLen = int(RdJson::getLong("pipelineLen", pipelineLen_default, robotConfigJSON));
	// Pipeline length and block size
	Log.trace("MotionHelper configMotionPipeline len %d, _blockDistanceMM %0.2f",
					pipelineLen, _blockDistanceMM);
	_motionPipeline.init(pipelineLen);
	_motionPlanner.configure(junctionDeviation);

	// MotionIO
	_motionIO.configureMotors(robotConfigJSON);
	return true;
}
#
bool MotionHelper::canAccept()
{
	// Check that at the motion pipeline can accept new data
	return _motionPipeline.canAccept();
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

void MotionHelper::setMotionParams(RobotCommandArgs& args)
{
	// Check for relative movement specified and set accordingly
	if (args.moveType != RobotMoveTypeArg_None)
		_moveRelative = (args.moveType == RobotMoveTypeArg_Relative);
}

void MotionHelper::getCurStatus(RobotCommandArgs& args)
{
	// Get current position
	AxisFloats axisPosns;
	args.pt = _curAxisPosition._axisPositionMM;
	// Absolute/Relative movement
	args.moveType = _moveRelative ? RobotMoveTypeArg_Relative : RobotMoveTypeArg_Absolute;
}

bool MotionHelper::moveTo(RobotCommandArgs& args)
{
	Log.trace("+++++++MotionHelper moveTo x %0.2f y %0.2f", args.pt._pt[0], args.pt._pt[1]);

	// Handle any motion parameters (such as relative movement, feedrate, etc)
	setMotionParams(args);

	// Convert the move to actuator coordinates
	AxisFloats actuatorCoords;
	_ptToActuatorFn(args.pt, actuatorCoords, _axesParams.getAxisParamsArray(), RobotConsts::MAX_AXES);

	// Plan the move
	bool moveOk = _motionPlanner.moveTo(args, actuatorCoords, _curAxisPosition, _axesParams, _motionPipeline);
	if (moveOk)
	{
		// Update axisMotion
		_curAxisPosition._axisPositionMM = args.pt;
		_curAxisPosition._stepsFromHome = actuatorCoords;
	}
	return moveOk;

}

void MotionHelper::service(bool processPipeline)
{
	// Check if we should process the movement pipeline
	if (processPipeline)
	{
		_motionActuator.process();
	}
}

void MotionHelper::debugShowBlocks()
{
	int elIdx = 0;
	bool headShown = false;
	for (int i = _motionPipeline.count() - 1; i >= 0; i--)
	{
		MotionBlock* pBlock = _motionPipeline.peekNthFromPut(i);
		if (pBlock)
		{
			if (!headShown)
			{
				pBlock->debugShowBlkHead();
				headShown = true;
			}
			pBlock->debugShowBlock(elIdx++);
		}
	}
}

void MotionHelper::debugShowTiming()
{
	_motionActuator.showDebug();
}

int MotionHelper::testGetPipelineCount()
{
	return _motionPipeline.count();
}
void MotionHelper::testGetPipelineBlock(int elIdx, MotionBlock& block)
{
	block = *_motionPipeline.peekNthFromPut(_motionPipeline.count() - 1 - elIdx);
}
