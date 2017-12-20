// RBotFirmware
// Rob Dobson 2016

#include "application.h"
#include "ConfigPinMap.h"
#include "math.h"
#include "MotionHelper.h"
#include "Utils.h"

MotionHelper::MotionHelper()
{
	// Init
	_isPaused = false;
	_wasPaused = false;
    _xMaxMM = 0;
    _yMaxMM = 0;
    _maxMotionDistanceMM = 0;
	_blockDistanceMM = blockDistanceMM_default;
	for (int i = 0; i < MAX_AXES; i++)
{
        _stepperMotors[i] = NULL;
        _servoMotors[i] = NULL;
        for (int j = 0; j < MAX_ENDSTOPS_PER_AXIS; j++)
            _endStops[i][j] = NULL;
}
	_axisMotion.clear();
	_stepEnablePin = -1;
	_stepEnableActiveLevel = true;
	_stepDisableSecs = 60.0;
	_ptToActuatorFn = NULL;
	_actuatorToPtFn = NULL;
	_correctStepOverflowFn = NULL;
	_motorEnLastMillis = 0;
	_motorEnLastUnixTime = 0;
	_moveRelative = false;
	_numRobotAxes = 0;
	_junctionDeviation = 0.05f;
	_minimumPlannerSpeedMMps = 0;
	_prevPipelineElemValid = false;
	// Configure the motion pipeline
	configMotionPipeline();
	_motionPlanner.setPipeline(&_motionPipeline);
}

MotionHelper::~MotionHelper()
{
	deinit();
}

void MotionHelper::deinit()
{
#ifndef TEST_IN_GCPP
	// disable
	if (_stepEnablePin != -1)
		pinMode(_stepEnablePin, INPUT);

	// remove motors and end stops
	for (int i = 0; i < MAX_AXES; i++)
	{
		delete _stepperMotors[i];
		_stepperMotors[i] = NULL;
		if (_servoMotors[i])
			_servoMotors[i]->detach();
		delete _servoMotors[i];
		_servoMotors[i] = NULL;
		for (int j = 0; j < MAX_ENDSTOPS_PER_AXIS; j++)
		{
			delete _endStops[i][j];
			_endStops[i][j] = NULL;
		}
	}
#endif
}

void MotionHelper::setTransforms(ptToActuatorFnType ptToActuatorFn, actuatorToPtFnType actuatorToPtFn,
	correctStepOverflowFnType correctStepOverflowFn)
{
	// Store callbacks
	_ptToActuatorFn = ptToActuatorFn;
	_actuatorToPtFn = actuatorToPtFn;
	_correctStepOverflowFn = correctStepOverflowFn;
}

void MotionHelper::setAxisParams(const char* robotConfigJSON)
{
	// Deinitialise
	deinit();

	// Axes
	for (int i = 0; i < MAX_AXES; i++)
	{
		if (configureAxis(robotConfigJSON, i))
			_numRobotAxes = i + 1;
	}

	// Configure robot
	configure(robotConfigJSON);

	// Clear motion info
	_axisMotion.clear();
}

bool MotionHelper::configure(const char* robotConfigJSON)
{
	// Get motor enable info
    WiringString stepEnablePinName = RdJson::getString("stepEnablePin", "-1", robotConfigJSON);
    _stepEnableActiveLevel = RdJson::getLong("stepEnableActiveLevel", 1, robotConfigJSON);
    _stepEnablePin = ConfigPinMap::getPinFromName(stepEnablePinName.c_str());
    _stepDisableSecs = float(RdJson::getDouble("stepDisableSecs", stepDisableSecs_default, robotConfigJSON));
    _xMaxMM = float(RdJson::getDouble("xMaxMM", 0, robotConfigJSON));
    _yMaxMM = float(RdJson::getDouble("yMaxMM", 0, robotConfigJSON));
    _maxMotionDistanceMM = sqrt(_xMaxMM * _xMaxMM + _yMaxMM * _yMaxMM);
    _blockDistanceMM = float(RdJson::getDouble("blockDistanceMM", blockDistanceMM_default, robotConfigJSON));
    Log.info("MotorEnable (pin %d, actLvl %d, disableAfter %0.2fs)", _stepEnablePin, _stepEnableActiveLevel, _stepDisableSecs);
	configMotionPipeline();

    // Enable pin - initially disable
    pinMode(_stepEnablePin, OUTPUT);
    digitalWrite(_stepEnablePin, !_stepEnableActiveLevel);
	return true;
}

bool MotionHelper::configureAxis(const char* robotConfigJSON, int axisIdx)
{
	if (axisIdx < 0 || axisIdx >= MAX_AXES)
		return false;

    // Get params
    WiringString axisIdStr = "axis" + WiringString(axisIdx);
    WiringString axisJSON = RdJson::getString(axisIdStr, "{}", robotConfigJSON);
    if (axisJSON.length() == 0 || axisJSON.equals("{}"))
        return false;

	// Set the axis parameters
    _axisParams[axisIdx].setFromJSON(axisJSON.c_str());
	Log.info("Axis%d params maxSpeed %02.f, acceleration %0.2f, minNsBetweenSteps %0.2f stepsPerRotation %0.2f, unitsPerRotation %0.2f",
		axisIdx, _axisParams[axisIdx]._maxSpeed, _axisParams[axisIdx]._maxAcceleration, _axisParams[axisIdx]._minNsBetweenSteps,
		_axisParams[axisIdx]._stepsPerRotation, _axisParams[axisIdx]._unitsPerRotation);
	Log.info("Axis%d params minVal %02.f (%d), maxVal %0.2f (%d), isDominant %d, isServo %d, homeOffVal %0.2f, homeOffSteps %ld",
		axisIdx, _axisParams[axisIdx]._minVal, _axisParams[axisIdx]._minValValid,
		_axisParams[axisIdx]._maxVal, _axisParams[axisIdx]._maxValValid,
		_axisParams[axisIdx]._isDominantAxis, _axisParams[axisIdx]._isServoAxis,
		_axisParams[axisIdx]._homeOffsetVal, _axisParams[axisIdx]._homeOffsetSteps);

    // Check the kind of motor to use
    bool isValid = false;
    WiringString stepPinName = RdJson::getString("stepPin", "-1", axisJSON, isValid);
    if (isValid)
    {
        // Create the stepper motor for the axis
        WiringString dirnPinName = RdJson::getString("dirnPin", "-1", axisJSON);
        int stepPin = ConfigPinMap::getPinFromName(stepPinName.c_str());
        int dirnPin = ConfigPinMap::getPinFromName(dirnPinName.c_str());
        Log.info("Axis%d (step pin %ld, dirn pin %ld)", axisIdx, stepPin, dirnPin);
        if ((stepPin != -1 && dirnPin != -1))
            _stepperMotors[axisIdx] = new StepperMotor(StepperMotor::MOTOR_TYPE_DRIVER, stepPin, dirnPin);
    }
    else
    {
        // Create a servo motor for the axis
        WiringString servoPinName = RdJson::getString("servoPin", "-1", axisJSON);
        long servoPin = ConfigPinMap::getPinFromName(servoPinName.c_str());
        Log.info("Axis%d (servo pin %ld)", axisIdx, servoPin);
        if ((servoPin != -1))
        {
            _servoMotors[axisIdx] = new Servo();
            if (_servoMotors[axisIdx])
                _servoMotors[axisIdx]->attach(servoPin);

            // For servos go home now
            jumpHome(axisIdx);
        }
    }

    // End stops
    for (int endStopIdx =0; endStopIdx < MAX_ENDSTOPS_PER_AXIS; endStopIdx++)
    {
        // Get the config for endstop if present
        WiringString endStopIdStr = "endStop" + WiringString(endStopIdx);
        WiringString endStopJSON = RdJson::getString(endStopIdStr, "{}", axisJSON);
        if (endStopJSON.length() == 0 || endStopJSON.equals("{}"))
            continue;

        // Create endStop from JSON
        _endStops[axisIdx][endStopIdx] = new EndStop(axisIdx, endStopIdx, endStopJSON);
    }

    // TEST Major axis
    if (axisIdx == 0)
        _axisParams[0]._isDominantAxis = true;

    return true;
}

bool MotionHelper::canAcceptCommand()
{
	// Check that at the motion pipeline can accept new data
    return _motionPipeline.canAccept();
}

// Stop
void MotionHelper::stop()
{
	_motionPipeline.clear();
	_isPaused = false;
}

void MotionHelper::configMotionPipeline()
{
	// Calculate max blocks that a movement command can translate into
	int maxPossibleBlocksInMove = int(ceil(_maxMotionDistanceMM / _blockDistanceMM));
	Log.trace("MotionHelper configMotionPipeline _maxMotionDistanceMM %0.2f _blockDistanceMM %0.2f",
		_maxMotionDistanceMM, _blockDistanceMM);
	_motionPipeline.init(maxPossibleBlocksInMove);
}

void MotionHelper::enableMotors(bool en, bool timeout)
{
//        Log.trace("Enable %d, disable level %d, disable after time %0.2f", en, !_stepEnableActiveLevel, _stepDisableSecs);
    if (en)
    {
        if (_stepEnablePin != -1)
        {
            if (!_motorsAreEnabled)
                Log.info("MotionHelper motors enabled, disable after time %0.2f", _stepDisableSecs);
            digitalWrite(_stepEnablePin, _stepEnableActiveLevel);
        }
        _motorsAreEnabled = true;
        _motorEnLastMillis = millis();
        _motorEnLastUnixTime = Time.now();
    }
    else
    {
        if (_stepEnablePin != -1)
        {
            if (_motorsAreEnabled)
                Log.info("MotionHelper motors disabled by %s", timeout ? "timeout" : "command");
            digitalWrite(_stepEnablePin, !_stepEnableActiveLevel);
        }
        _motorsAreEnabled = false;
    }
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
	args.pt = _axisMotion._axisPositionMM;
	// Absolute/Relative movement
	args.moveType = _moveRelative ? RobotMoveTypeArg_Relative : RobotMoveTypeArg_Absolute;
}

void MotionHelper::service(bool processPipeline)
{
	// Check if we should process the movement pipeline
	if (processPipeline)
	{
		if (_isPaused)
		{
			_wasPaused = true;
		}
		else
		{
			pipelineService(_wasPaused);
			_wasPaused = false;
		}
	}
}

void MotionHelper::pipelineService(bool hasBeenPaused)
{
	//// Check if any axis is moving
	//bool anyAxisMoving = false;
	//for (int i = 0; i < MAX_AXES; i++)
	//{
	//	// Check if movement required
	//	if (_axisParams[i]._targetStepsFromHome == _axisParams[i]._stepsFromHome)
	//		continue;
	//	anyAxisMoving = true;
	//}

	//// If nothing moving then prep the next pipeline element (if there is one)
	//if (!anyAxisMoving)
	//{
	//	MotionPipelineElem motionElem;
	//	if (_motionPipeline.get(motionElem))
	//	{
	//		// Correct for any overflows in stepper values (may occur with rotational robots)
	//		_correctStepOverflowFn(_axisParams, _numRobotAxes);

	//		// Check if a real distance
	//		double distToTravelMM = motionElem.delta();
	//		bool valid = true;
	//		if (distToTravelMM < distToTravelMM_ignoreBelow)
	//			valid = false;

	//		// Get absolute step position to move to
	//		PointND actuatorCoords;
	//		if (valid)
	//			valid = _ptToActuatorFn(motionElem, actuatorCoords, _axisParams, _numRobotAxes);

	//		// Activate motion if valid - otherwise ignore
	//		if (valid)
	//		{
	//			// Get steps from home for each axis
	//			for (int i = 0; i < MAX_AXES; i++)
	//				_axisParams[i]._targetStepsFromHome = actuatorCoords.getVal(i);

	//			// Balance the time for each direction
	//			// double calcMotionTime = calcMotionTimeUs(motionElem, axisParams);
	//			double speedTargetMMps = _axisParams[0]._maxSpeed;
	//			double timeToTargetS = distToTravelMM / speedTargetMMps;
	//			long stepsAxis[MAX_AXES];
	//			double timePerStepAxisNs[MAX_AXES];
	//			Log.trace("motionHelper speedTargetMMps %0.2f distToTravelMM %0.2f timeToTargetMS %0.2f",
	//				speedTargetMMps, distToTravelMM, timeToTargetS*1000.0);
	//			for (int i = 0; i < MAX_AXES; i++)
	//			{
	//				stepsAxis[i] = labs(_axisParams[i]._targetStepsFromHome - _axisParams[i]._stepsFromHome);
	//				if (stepsAxis[i] == 0)
	//					timePerStepAxisNs[i] = 1000000000;
	//				else
	//					timePerStepAxisNs[i] = timeToTargetS * 1000000000 / stepsAxis[i];
	//				if (timePerStepAxisNs[i] < _axisParams[i]._minNsBetweenSteps)
	//					timePerStepAxisNs[i] = _axisParams[i]._minNsBetweenSteps;
	//				Log.trace("motionHelper axis%d target %ld fromHome %ld timerPerStepNs %f",
	//					i, _axisParams[i]._targetStepsFromHome, _axisParams[i]._stepsFromHome,
	//					timePerStepAxisNs[i]);
	//			}

	//			// Check if there is little difference from current timePerStep on the
	//			// dominant axis
	//			if (_axisParams[0]._isDominantAxis && isInBounds(timePerStepAxisNs[0],
	//				_axisParams[0]._betweenStepsNs*0.66, _axisParams[0]._betweenStepsNs*1.33))
	//			{
	//				// In that case use the dominant axis time with a correction factor
	//				double speedCorrectionFactor = _axisParams[0]._betweenStepsNs / timePerStepAxisNs[0];
	//				_axisParams[1]._betweenStepsNs = timePerStepAxisNs[1] * speedCorrectionFactor;
	//			}
	//			else if (_axisParams[1]._isDominantAxis && isInBounds(timePerStepAxisNs[1],
	//				_axisParams[1]._betweenStepsNs*0.66, _axisParams[1]._betweenStepsNs*1.33))
	//			{
	//				// In that case use the dominant axis time with a correction factor
	//				double speedCorrectionFactor = _axisParams[1]._betweenStepsNs / timePerStepAxisNs[1];
	//				_axisParams[0]._betweenStepsNs = timePerStepAxisNs[0] * speedCorrectionFactor;
	//			}
	//			else
	//			{
	//				// allow each axis to use a different stepping time
	//				_axisParams[0]._betweenStepsNs = timePerStepAxisNs[0];
	//				_axisParams[1]._betweenStepsNs = timePerStepAxisNs[1];
	//			}
	//		}

	//		// Debug
	//		Log.info("MotionHelper StepNS X %ld Y %ld Z %ld)", _axisParams[0]._betweenStepsNs,
	//			_axisParams[1]._betweenStepsNs, _axisParams[2]._betweenStepsNs);
	//		motionElem._pt1MM.logDebugStr("MotionFrom");
	//		motionElem._pt2MM.logDebugStr("MotionTo");

	//		// Log.trace("Move to %sx %0.2f y %0.2f -> ax0Tgt %0.2f Ax1Tgt %0.2f (stpNSXY %ld %ld)",
	//		//             valid?"":"INVALID ", motionElem._pt2MM._pt[0], motionElem._pt2MM._pt[1],
	//		//             actuatorCoords._pt[0], actuatorCoords._pt[1],
	//		//             _axisParams[0]._betweenStepsNs, _axisParams[1]._betweenStepsNs);
	//	}
	//}

	//// Make the next step on each axis as requred
	//for (int i = 0; i < MAX_AXES; i++)
	//{
	//	// Check if a move is required
	//	if (_axisParams[i]._targetStepsFromHome == _axisParams[i]._stepsFromHome)
	//		continue;

	//	// Check for servo driven axis (doesn't need individually stepping)
	//	if (_axisParams[i]._isServoAxis)
	//	{
	//		Log.trace("Servo(ax#%d) jump to %ld", i, _axisParams[i]._targetStepsFromHome);
	//		jump(i, _axisParams[i]._targetStepsFromHome);
	//		_axisParams[i]._stepsFromHome = _axisParams[i]._targetStepsFromHome;
	//		continue;
	//	}

	//	//// Check if time to move
	//	//if (hasBeenPaused || (Utils::isTimeout(micros(), _axisParams[i]._lastStepMicros, _axisParams[i]._betweenStepsNs / 1000)))
	//	//{
	//	//	step(i, _axisParams[i]._targetStepsFromHome > _axisParams[i]._stepsFromHome);
	//	//	// Log.trace("Step %d %d", i, _axisParams[i]._targetStepsFromHome > _axisParams[i]._stepsFromHome);
	//	//	_axisParams[i]._betweenStepsNs += _axisParams[i]._betweenStepsNsChangePerStep;
	//	//}
	//}

}

//bool MotionHelper::isMoving()
//{
//	for (int i = 0; i < MAX_AXES; i++)
//	{
//		// Check if movement required - if so we are busy
//		if (_axisParams[i]._targetStepsFromHome != _axisParams[i]._stepsFromHome)
//			return true;
//	}
//	return false;
//}

//void MotionHelper::axisSetHome(int axisIdx)
//{
//	if (axisIdx < 0 || axisIdx >= MAX_AXES)
//		return;
//	if (_axisParams[axisIdx]._isServoAxis)
//	{
//		_axisParams[axisIdx]._homeOffsetSteps = _axisParams[axisIdx]._stepsFromHome;
//	}
//	else
//	{
//		_axisParams[axisIdx]._stepsFromHome = _axisParams[axisIdx]._homeOffsetSteps;
//		_axisParams[axisIdx]._targetStepsFromHome = _axisParams[axisIdx]._homeOffsetSteps;
//	}
//	Log.trace("Setting axis#%d home %lu", axisIdx, _axisParams[axisIdx]._homeOffsetSteps);
//#ifdef USE_MOTION_ISR_MANAGER
//	motionISRManager.resetZero(axisIdx);
//#endif
//}

void MotionHelper::step(int axisIdx, bool direction)
{
	if (axisIdx < 0 || axisIdx >= MAX_AXES)
		return;

}

void MotionHelper::jump(int axisIdx, long targetStepsFromHome)
{
	if (axisIdx < 0 || axisIdx >= MAX_AXES)
		return;

}

void MotionHelper::jumpHome(int axisIdx)
{
	if (axisIdx < 0 || axisIdx >= MAX_AXES)
		return;

}

bool MotionHelper::moveTo(RobotCommandArgs& args)
{
	Log.trace("+++++++MotionHelper moveTo x %0.2f y %0.2f", args.pt._pt[0], args.pt._pt[1]);

	// Handle any motion parameters (such as relative movement, feedrate, etc)
	setMotionParams(args);

	// Find axis deltas and sum of squares of motion on primary axes
	float deltas[MAX_AXES];
	bool isAMove = false;
	bool isAPrimaryMove = false;
	float squareSum = 0;
	for (int i = 0; i < _numRobotAxes; i++)
	{
		deltas[i] = args.pt._pt[i] - _axisMotion._axisPositionMM._pt[i];
		if (deltas[i] != 0)
		{
			isAMove = true;
			if (_axisParams[i]._isPrimaryAxis)
			{
				squareSum += powf(deltas[i], 2);
				isAPrimaryMove = true;
			}
		}
	}

	// Distance being moved
	float moveDist = sqrtf(squareSum);

	// Ignore if there is no real movement
	if (!isAMove || (isAPrimaryMove && moveDist < MINIMUM_PRIMARY_MOVE_DIST_MM))
		return false;

	Log.trace("Moving %0.3f mm", moveDist);

	// Find the unit vectors
	AxisFloats unitVec;
	for (int i = 0; i < _numRobotAxes; i++)
	{
		if (_axisParams[i]._isPrimaryAxis)
		{
			unitVec._pt[i] = deltas[i] / moveDist;
			// Check that the move speed doesn't exceed max
			if (_axisParams[i]._maxSpeed > 0)
			{
				if (args.feedrateValid)
				{
					float axisSpeed = fabsf(unitVec._pt[i] * args.feedrateVal);
					if (axisSpeed > _axisParams[i]._maxSpeed)
					{
						args.feedrateVal *= axisSpeed / _axisParams[i]._maxSpeed;
						args.feedrateValid = true;
					}
				}
				else
				{
					args.feedrateVal = _axisParams[i]._maxSpeed;
					args.feedrateValid = true;
				}
			}
		}
	}

	// Calculate move time
	float reciprocalTime = args.feedrateVal / moveDist;
	Log.trace("Feedrate %0.3f, reciprocalTime %0.3f", args.feedrateVal, reciprocalTime);

	// Use default acceleration for dominant axis (or 1st primary) to start with
	float acceleration = _axisParams[0].acceleration_default;
	int dominantIdx = -1;
	int firstPrimaryIdx = -1;
	for (int i = 0; i < _numRobotAxes; i++)
	{
		if (_axisParams[i]._isDominantAxis)
		{
			dominantIdx = i;
			break;
		}
		if (firstPrimaryIdx = -1 && _axisParams[i]._isPrimaryAxis)
		{
			firstPrimaryIdx = i;
		}
	}
	if (dominantIdx != -1)
		acceleration = _axisParams[dominantIdx]._maxAcceleration;
	else if (firstPrimaryIdx != -1)
		acceleration = _axisParams[firstPrimaryIdx]._maxAcceleration;

	// Check speed limits for each axis individually
	for (int i = 0; i < _numRobotAxes; i++)
	{
		// Speed and time
		float axisDist = fabsf(args.pt._pt[i] - _axisMotion._axisPositionMM._pt[i]);
		if (axisDist == 0)
			continue;
		float axisReqdAcc = axisDist * reciprocalTime;
		if (axisReqdAcc > _axisParams[i]._maxAcceleration)
		{
			args.feedrateVal *= _axisParams[i]._maxAcceleration / axisReqdAcc;
			reciprocalTime = args.feedrateVal / moveDist;
		}
	}

	Log.trace("Feedrate %0.3f, reciprocalTime %0.3f", args.feedrateVal, reciprocalTime);

	// Create a motion pipeline element for this movement
	MotionPipelineElem elem(_axisMotion._axisPositionMM, args.pt);

	Log.trace("MotionPipelineElem delta %0.3f", elem.delta());

	// Convert to actuator coords
	AxisFloats actuatorCoords;
	_ptToActuatorFn(elem, actuatorCoords, _axisParams, _numRobotAxes);
	elem._destActuatorCoords = actuatorCoords;
	elem._speedMMps = args.feedrateVal;
	elem._accMMpss = acceleration;
	elem._primaryAxisMove = isAPrimaryMove;
	elem._unitVec = unitVec;
	elem._moveDistMM = moveDist;

	// Add the block to the planner queue
	return addBlockToPlanner(elem);

	//// Get the starting position - this is either the destination of the
	//// last block in the pipeline or the current target location of the
	//// robot (whether currently moving or idle)
	//// Peek at the last block in the pipeline if there is one
	//PointND startPos;
	//MotionPipelineElem lastPipelineEntry;
	//bool queueValid = _motionPipeline.peekLast(lastPipelineEntry);
	//if (queueValid)
	//{
	//	startPos = lastPipelineEntry._pt2MM;
	//	Log.trace("MotionHelper using queue end, startPos X %0.2f Y %0.2f",
	//		startPos._pt[0], startPos._pt[1]);
	//}
	//else
	//{
	//	PointND axisPosns;
	//	for (int i = 0; i < MAX_AXES; i++)
	//		axisPosns._pt[i] = _axisParams[i]._targetStepsFromHome;
	//	PointND curXy;
	//	_actuatorToPtFn(axisPosns, curXy, _axisParams, _numRobotAxes);
	//	startPos = curXy;
	//	Log.trace("MotionHelper queue empty startPos X %0.2f Y%0.2f",
	//		startPos._pt[0], startPos._pt[1]);
	//}

	//// Handle reltative motion and fill in the destPos for axes for
	//// which values not specified
	//// Don't use servo values for computing distance to travel
	//PointND destPos = args.pt;
	//bool includeDist[MAX_AXES];
	//for (int i = 0; i < MAX_AXES; i++)
	//{
	//	if (!args.valid.isValid(i))
	//	{
	//		destPos.setVal(i, startPos.getVal(i));
	//	}
	//	else
	//	{
	//		if (_moveRelative)
	//			destPos.setVal(i, startPos.getVal(i) + args.pt.getVal(i));
	//	}
	//	includeDist[i] = !_axisParams[i]._isServoAxis;
	//}

	//// Split up into blocks of maximum length
	//double lineLen = destPos.distanceTo(startPos, includeDist);

	//// Ensure at least one block
	//int numBlocks = int(lineLen / _blockDistanceMM);
	//if (numBlocks == 0)
	//	numBlocks = 1;
	//Log.trace("MotionHelper numBlocks %d (lineLen %0.2f / blockDistMM %02.f)",
	//	numBlocks, lineLen, _blockDistanceMM);
	//PointND steps = (destPos - startPos) / numBlocks;
	//for (int i = 0; i < numBlocks; i++)
	//{
	//	// Create block
	//	PointND pt1 = startPos + steps * i;
	//	PointND pt2 = startPos + steps * (i + 1);
	//	// Fix any axes which are non-stepping
	//	for (int j = 0; j < MAX_AXES; j++)
	//	{
	//		if (_axisParams[j]._isServoAxis)
	//		{
	//			pt1.setVal(j, (i == 0) ? startPos.getVal(j) : destPos.getVal(j));
	//			pt2.setVal(j, destPos.getVal(j));
	//		}
	//	}
	//	// If last block then just use end point coords
	//	if (i == numBlocks - 1)
	//		pt2 = destPos;
	//	// Log.trace("Adding x %0.2f y %0.2f", blkX, blkY);
	//	// Add to pipeline
	//	if (!_motionPipeline.add(pt1, pt2))
	//		break;
	//}

}

bool MotionHelper::addBlockToPlanner(MotionPipelineElem& elem)
{
	// Find if there are any steps
	bool hasSteps = false;
	uint32_t axisMaxSteps = 0;
	for (int i = 0; i < _numRobotAxes; i++)
	{
		// Check if any actual steps to perform
		int steps = int(std::ceil(elem._destActuatorCoords._pt[i] - _axisMotion._stepsFromHome._pt[i]));
		if (steps != 0)
			hasSteps = true;
		// Direction
		elem._stepDirn._valid[i] = (steps < 0);
		elem._absSteps.vals[i] = labs(steps);
		if (axisMaxSteps < elem._absSteps.vals[i])
			axisMaxSteps = elem._absSteps.vals[i];
	}

	// Check there are some actual steps
	if (!hasSteps)
		return false;

	// Update axisMotion
	_axisMotion._axisPositionMM = elem._pt2MM;
	_axisMotion._stepsFromHome = elem._destActuatorCoords;

	// Max steps for an axis
	elem._axisMaxSteps = axisMaxSteps;

	// Check movement distance
	if (elem._moveDistMM > 0.0f)
	{
		elem._nominalSpeedMMps = elem._speedMMps;
		elem._nominalStepRatePerSec = axisMaxSteps * elem._speedMMps / elem._moveDistMM;
	}
	else
	{
		elem._nominalSpeedMMps = 0;
		elem._nominalStepRatePerSec = 0;
	}

	Log.trace("maxSteps %lu, nomSpeed %0.3f mm/s, nomRate %0.3f steps/s",
		axisMaxSteps, elem._nominalSpeedMMps, elem._nominalStepRatePerSec);

	// Comments from Smoothieware!
	// Compute the acceleration rate for the trapezoid generator. Depending on the slope of the line
	// average travel per step event changes. For a line along one axis the travel per step event
	// is equal to the travel/step in the particular axis. For a 45 degree line the steppers of both
	// axes might step for every step event. Travel per step event is then sqrt(travel_x^2+travel_y^2).

	// Compute maximum allowable entry speed at junction by centripetal acceleration approximation.
	// Let a circle be tangent to both previous and current path line segments, where the junction
	// deviation is defined as the distance from the junction to the closest edge of the circle,
	// colinear with the circle center. The circular segment joining the two paths represents the
	// path of centripetal acceleration. Solve for max velocity based on max acceleration about the
	// radius of the circle, defined indirectly by junction deviation. This may be also viewed as
	// path width or max_jerk in the previous grbl version. This approach does not actually deviate
	// from path, but used as a robust way to compute cornering speeds, as it takes into account the
	// nonlinearities of both the junction angle and junction velocity.

	// NOTE however it does not take into account independent axis, in most cartesian X and Y and Z are totally independent
	// and this allows one to stop with little to no decleration in many cases. This is particualrly bad on leadscrew based systems that will skip steps.

	// Junction deviation
	float junctionDeviation = _junctionDeviation;
	float vmaxJunction = _minimumPlannerSpeedMMps;

	// For primary axis moves compute the vmaxJunction
	// But only if there are still elements in the queue
	// If not assume the robot is idle and start the move
	// from scratch
	if (elem._primaryAxisMove && _motionPipeline.canGet())
	{
		float prevNominalSpeed = _prevPipelineElem._primaryAxisMove ? _prevPipelineElem._nominalSpeedMMps : 0;
		if (junctionDeviation > 0.0f && prevNominalSpeed > 0.0f)
		{
			// Compute cosine of angle between previous and current path. (prev_unit_vec is negative)
			// NOTE: Max junction velocity is computed without sin() or acos() by trig half angle identity.
			float cosTheta = - _prevPipelineElem._unitVec.X() * elem._unitVec.X()
						- _prevPipelineElem._unitVec.Y() * elem._unitVec.Y()
						- _prevPipelineElem._unitVec.Z() * elem._unitVec.Z();

			// Skip and use default max junction speed for 0 degree acute junction.
			if (cosTheta < 0.95F) {
				vmaxJunction = std::min(prevNominalSpeed, elem._nominalSpeedMMps);
				// Skip and avoid divide by zero for straight junctions at 180 degrees. Limit to min() of nominal speeds.
				if (cosTheta > -0.95F) {
					// Compute maximum junction velocity based on maximum acceleration and junction deviation
					float sinThetaD2 = sqrtf(0.5F * (1.0F - cosTheta)); // Trig half angle identity. Always positive.
					vmaxJunction = std::min(vmaxJunction, sqrtf(elem._accMMpss * junctionDeviation * sinThetaD2 / (1.0F - sinThetaD2)));
				}
			}

		}
	}
	elem._maxEntrySpeedMMps = vmaxJunction;

	Log.trace("PrevMoveInQueue %d, JunctionDeviation %0.3f, VmaxJunction %0.3f", _motionPipeline.canGet(), junctionDeviation, vmaxJunction);

	// Calculate max allowable speed using v^2 = u^2 - 2as
	// Was acceleration*60*60*distance, in case this breaks, but here we prefer to use seconds instead of minutes
	float maxAllowableSpeedMMps = sqrtf(_minimumPlannerSpeedMMps * _minimumPlannerSpeedMMps - 2.0F * (-elem._accMMpss) * elem._moveDistMM);
	elem._entrySpeedMMps = std::min(vmaxJunction, maxAllowableSpeedMMps);

	// Initialize planner efficiency flags
	// Set flag if block will always reach maximum junction speed regardless of entry/exit speeds.
	// If a block can de/ac-celerate from nominal speed to zero within the length of the block, then
	// the current block and next block junction speeds are guaranteed to always be at their maximum
	// junction speeds in deceleration and acceleration, respectively. This is due to how the current
	// block nominal speed limits both the current and next maximum junction speeds. Hence, in both
	// the reverse and forward planners, the corresponding block junction speed will always be at the
	// the maximum junction speed and may always be ignored for any speed reduction checks.
	elem._nominalLengthFlag = (elem._nominalSpeedMMps <= maxAllowableSpeedMMps);

	Log.trace("MaxAllowableSpeed %0.3f, entrySpeedMMps %0.3f, nominalLengthFlag %d", maxAllowableSpeedMMps, elem._entrySpeedMMps, elem._nominalLengthFlag);

	// Recalculation required
	elem._recalcFlag = true;

	// Store the element in the queue and remember previous element
	_motionPipeline.add(elem);
	_prevPipelineElem = elem;
	_prevPipelineElemValid = true;

	// Recalculate the whole queue
	recalculatePipeline();

	return true;
}

void MotionHelper::recalculatePipeline()
{
	
	// A newly added block is decel limited
	// We find its max entry speed given its exit speed
	//
	// For each block, walking backwards in the queue :
	//    if max entry speed == current entry speed
	//    then we can set recalculate to false, since clearly adding another block didn't allow us to enter faster
	//    and thus we don't need to check entry speed for this block any more
	//	
	// Once we find an accel limited block, we must find the max exit speed and walk the queue forwards
	//	
	// For each block, walking forwards in the queue :
	//
	//    given the exit speed of the previous block and our own max entry speed
	//    we can tell if we're accel or decel limited (or coasting)
	//	
	//    if prev_exit > max_entry
	//    then we're still decel limited. update previous trapezoid with our max entry for prev exit
	//    if max_entry >= prev_exit
	//    then we're accel limited. set recalculate to false, work out max exit speed
	//	
	// Finally, work out trapezoid for the final(and newest) block.

	Log.trace("------recalculatePipeline");
	// Step 1:
	// For each block, given the exit speed and acceleration, find the maximum entry speed
	float entrySpeed = _minimumPlannerSpeedMMps;
	int elemIdxFromPutPos = 0;
	MotionPipelineElem* pElem = NULL;
	MotionPipelineElem* pPrevElem = NULL;
	MotionPipelineElem* pNext = NULL;
	if (_motionPipeline.peekNthFromPut(elemIdxFromPutPos) != NULL)
	{
		pElem = _motionPipeline.peekNthFromPut(elemIdxFromPutPos);
		while (pElem && pElem->_recalcFlag)
		{
			// Get the max entry speed
			pElem->calcMaxSpeedReverse(entrySpeed);
			Log.trace("Backwards pass #%d element %08x, tryEntrySpeed %0.3f, newEntrySpeed %0.3f", elemIdxFromPutPos, pElem, entrySpeed, pElem->_entrySpeedMMps);
			entrySpeed = pElem->_entrySpeedMMps;
			// Next elem
			pNext = _motionPipeline.peekNthFromPut(elemIdxFromPutPos+1);
			if (!pNext)
				break;
			pElem = pNext;
			// The exit speed for the previous block up the chain is the entry speed we've just calculated
			pElem->_exitSpeedMMps = entrySpeed;
			elemIdxFromPutPos++;
		}

		// Calc exit speed of first block
		pElem->maximizeExitSpeed();

		// Step 2:
		// Pointing to the first block that doesn't need recalculating (or the get block)
		while (true)
		{
			// Save previous
			pPrevElem = pElem;
			// Next element
			if (elemIdxFromPutPos == 0)
				break;
			elemIdxFromPutPos--;
			pElem = _motionPipeline.peekNthFromPut(elemIdxFromPutPos);
			if (!pElem)
				break;
			// Pass the exit speed of the previous block
			// so this block can decide if it's accel or decel limited and update its fields as appropriate
			pElem->calcMaxSpeedForward(pPrevElem->_exitSpeedMMps);
			Log.trace("Forward pass #%d element %08x, prev En %0.3f Ex %0.3f. cur En %0.3f Ex %0.3f", elemIdxFromPutPos, pElem, 
				pPrevElem->_entrySpeedMMps, pPrevElem->_exitSpeedMMps, pElem->_entrySpeedMMps, pElem->_exitSpeedMMps);

			// Set exit speed and calculate trapezoid on this block
			pPrevElem->calculateTrapezoid();
			Log.trace("Forward pass #%d after trapezoid entrySpeed %0.3f, exitSpeed %0.3f", elemIdxFromPutPos, pElem->_entrySpeedMMps, pElem->_exitSpeedMMps);
		}

	}
	// Step 3:
	// work out trapezoid for final (and newest) block
	// now current points to the item most recently inserted
	// which has not had calculate_trapezoid run yet
	//pElem = _motionPipeline.peekNthFromPut(0);
	//if (pElem)
	//{
	//	pElem->_exitSpeedMMps = _minimumPlannerSpeedMMps;
 //		pElem->calculateTrapezoid();
	//	Log.trace("Final (most recent) #%d after trapezoid entrySpeed %0.3f, exitSpeed %0.3f", elemIdxFromPutPos, pElem->_entrySpeedMMps, pElem->_exitSpeedMMps);
	//}
}

void MotionHelper::debugShowBlocks()
{
	int elIdx = 0;
	for (int i = _motionPipeline.count() - 1; i >= 0; i--)
	{
		MotionPipelineElem* pElem = _motionPipeline.peekNthFromPut(i);
		if (pElem)
		{
			Log.trace("#%d En %0.3f Ex %0.3f", elIdx++, pElem->_entrySpeedMMps, pElem->_exitSpeedMMps);
		}
	}

}
