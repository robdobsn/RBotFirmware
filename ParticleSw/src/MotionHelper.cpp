// RBotFirmware
// Rob Dobson 2016

#include "application.h"
#include "ConfigPinMap.h"
#include "math.h"
#include "MotionHelper.h"
#include "Utils.h"

#ifdef USE_MOTION_ISR_MANAGER
#include "MotionISRManager.h"
static MotionISRManager motionISRManager;
#endif

bool MotionHelper::configure(const char* robotConfigJSON)
{
    // Get motor enable info
    String stepEnablePinName = RdJson::getString("stepEnablePin", "-1", robotConfigJSON);
    _stepEnableActiveLevel = RdJson::getLong("stepEnableActiveLevel", 1, robotConfigJSON);
    _stepEnablePin = ConfigPinMap::getPinFromName(stepEnablePinName.c_str());
    _stepDisableSecs = RdJson::getDouble("stepDisableSecs", stepDisableSecs_default, robotConfigJSON);
    _xMaxMM = RdJson::getDouble("xMaxMM", 0, robotConfigJSON);
    _yMaxMM = RdJson::getDouble("yMaxMM", 0, robotConfigJSON);
    _maxMotionDistanceMM = sqrt(_xMaxMM * _xMaxMM + _yMaxMM * _yMaxMM);
    _blockDistanceMM = RdJson::getDouble("blockDistanceMM", blockDistanceMM_default, robotConfigJSON);
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
    String axisIdStr = "axis" + String(axisIdx);
    String axisJSON = RdJson::getString(axisIdStr, "{}", robotConfigJSON);
    if (axisJSON.length() == 0 || axisJSON.equals("{}"))
        return false;

    // Set the axis parameters
    _axisParams[axisIdx].setFromJSON(axisJSON.c_str());
    Log.info("Axis%d params maxSpeed %02.f, acceleration %0.2f, minNsBetweenSteps %0.2f stepsPerRotation %0.2f, unitsPerRotation %0.2f",
            axisIdx, _axisParams[axisIdx]._maxSpeed, _axisParams[axisIdx]._acceleration, _axisParams[axisIdx]._minNsBetweenSteps,
            _axisParams[axisIdx]._stepsPerRotation, _axisParams[axisIdx]._unitsPerRotation);
    Log.info("Axis%d params minVal %02.f (%d), maxVal %0.2f (%d), isDominant %d, isServo %d, homeOffVal %0.2f, homeOffSteps %ld",
            axisIdx, _axisParams[axisIdx]._minVal, _axisParams[axisIdx]._minValValid,
            _axisParams[axisIdx]._maxVal, _axisParams[axisIdx]._maxValValid,
            _axisParams[axisIdx]._isDominantAxis, _axisParams[axisIdx]._isServoAxis,
            _axisParams[axisIdx]._homeOffsetVal, _axisParams[axisIdx]._homeOffsetSteps);

    // Check the kind of motor to use
    bool isValid = false;
    String stepPinName = RdJson::getString("stepPin", "-1", axisJSON, isValid);
    if (isValid)
    {
        // Create the stepper motor for the axis
        String dirnPinName = RdJson::getString("dirnPin", "-1", axisJSON);
        long stepPin = ConfigPinMap::getPinFromName(stepPinName.c_str());
        long dirnPin = ConfigPinMap::getPinFromName(dirnPinName.c_str());
        Log.info("Axis%d (step pin %ld, dirn pin %ld)", axisIdx, stepPin, dirnPin);
        if ((stepPin != -1 && dirnPin != -1))
            _stepperMotors[axisIdx] = new StepperMotor(StepperMotor::MOTOR_TYPE_DRIVER, stepPin, dirnPin);

#ifdef USE_MOTION_ISR_MANAGER
        motionISRManager.setAxis(axisIdx, stepPin, dirnPin, NULL);
#endif
    }
    else
    {
        // Create a servo motor for the axis
        String servoPinName = RdJson::getString("servoPin", "-1", axisJSON);
        long servoPin = ConfigPinMap::getPinFromName(servoPinName.c_str());
        Log.info("Axis%d (servo pin %ld)", axisIdx, servoPin);
        if ((servoPin != -1))
        {
            _servoMotors[axisIdx] = new Servo();
            if (_servoMotors[axisIdx])
                _servoMotors[axisIdx]->attach(servoPin);

            // For servos go home now
            jumpHome(axisIdx);

#ifdef USE_MOTION_ISR_MANAGER
            motionISRManager.setAxis(axisIdx, -1, -1, _servoMotors[axisIdx]);
#endif
        }
    }

    // End stops
    for (int endStopIdx =0; endStopIdx < MAX_ENDSTOPS_PER_AXIS; endStopIdx++)
    {
        // Get the config for endstop if present
        String endStopIdStr = "endStop" + String(endStopIdx);
        String endStopJSON = RdJson::getString(endStopIdStr, "{}", axisJSON);
        if (endStopJSON.length() == 0 || endStopJSON.equals("{}"))
            continue;

        // Create endStop from JSON
        _endStops[axisIdx][endStopIdx] = new EndStop(axisIdx, endStopIdx, endStopJSON);
    }

    // TEST Major axis
    if (axisIdx == 0)
        _axisParams[0]._isDominantAxis = true;

    // Other data
    _axisParams[axisIdx]._stepsFromHome = 0;

#ifdef USE_MOTION_ISR_MANAGER
    motionISRManager.resetZero(axisIdx);
#endif
    return true;
}

#ifdef USE_MOTION_ISR_MANAGER
void MotionHelper::pipelineService(bool hasBeenPaused)
{
    // Check if we can add to the motionISRManager
    if (!motionISRManager.canAdd())
        return;

    // Get a motion element from the queue if any available
    MotionPipelineElem motionElem;
    if (_motionPipeline.get(motionElem))
    {
        // Correct for any overflows in stepper values (may occur with rotational robots)
        _correctStepOverflowFn(_axisParams, _numRobotAxes);

        // Check if a real distance
        double distToTravelMM = motionElem.delta();
        bool valid = true;
        if (distToTravelMM < distToTravelMM_ignoreBelow)
            valid = false;

        // Get absolute step position to move to
        PointND actuatorCoords;
        if (valid)
            valid = _ptToActuatorFn(motionElem, actuatorCoords, _axisParams, _numRobotAxes);

        // Activate motion if valid - otherwise ignore
        if (valid)
        {
            // Get steps from home for each axis
            for (int i = 0; i < MAX_AXES; i++)
                _axisParams[i]._targetStepsFromHome = actuatorCoords.getVal(i);

            // Balance the time for each direction
            double speedTargetMMps = _axisParams[0]._maxSpeed;
            double timeToTargetS = distToTravelMM / speedTargetMMps;
            long stepsAxis[MAX_AXES];
            double timePerStepAxisNs[MAX_AXES];
            Log.trace("motionHelper speedTargetMMps %0.2f distToTravelMM %0.2f timeToTargetMS %0.2f",
                    speedTargetMMps, distToTravelMM, timeToTargetS*1000.0);
            for (int i = 0; i < MAX_AXES; i++)
            {
                stepsAxis[i] = labs(_axisParams[i]._targetStepsFromHome - _axisParams[i]._stepsFromHome);
                if (stepsAxis[i] == 0)
                    timePerStepAxisNs[i] = 1000000000;
                else
                    timePerStepAxisNs[i] = timeToTargetS * 1000000000 / stepsAxis[i];
                if (timePerStepAxisNs[i] < _axisParams[i]._minNsBetweenSteps)
                    timePerStepAxisNs[i] = _axisParams[i]._minNsBetweenSteps;
                Log.trace("motionHelper axis%d target %ld fromHome %ld timerPerStepNs %f",
                            i, _axisParams[i]._targetStepsFromHome, _axisParams[i]._stepsFromHome,
                            timePerStepAxisNs[i]);
            }

            // Check if there is little difference from current timePerStep on the
            // dominant axis
            if (_axisParams[0]._isDominantAxis && isInBounds(timePerStepAxisNs[0],
                        _axisParams[0]._betweenStepsNs*0.66, _axisParams[0]._betweenStepsNs*1.33))
            {
                // In that case use the dominant axis time with a correction factor
                double speedCorrectionFactor = _axisParams[0]._betweenStepsNs / timePerStepAxisNs[0];
                _axisParams[1]._betweenStepsNs = timePerStepAxisNs[1] * speedCorrectionFactor;
            }
            else if (_axisParams[1]._isDominantAxis && isInBounds(timePerStepAxisNs[1],
                        _axisParams[1]._betweenStepsNs*0.66, _axisParams[1]._betweenStepsNs*1.33))
            {
                // In that case use the dominant axis time with a correction factor
                double speedCorrectionFactor = _axisParams[1]._betweenStepsNs / timePerStepAxisNs[1];
                _axisParams[0]._betweenStepsNs = timePerStepAxisNs[0] * speedCorrectionFactor;
            }
            else
            {
                // allow each axis to use a different stepping time
                _axisParams[0]._betweenStepsNs = timePerStepAxisNs[0];
                _axisParams[1]._betweenStepsNs = timePerStepAxisNs[1];
            }

            // Add to the motion ISR
            for (int i = 0; i < MAX_AXES; i++)
            {
                bool stepDirn = _axisParams[i]._targetStepsFromHome > _axisParams[i]._stepsFromHome;
                uint32_t steps = _axisParams[i]._targetStepsFromHome - _axisParams[i]._stepsFromHome;
                if (!stepDirn)
                    steps = _axisParams[i]._stepsFromHome - _axisParams[i]._targetStepsFromHome;
                motionISRManager.addAxisSteps(i, steps, stepDirn, _axisParams[i]._betweenStepsNs);
            }
            motionISRManager.addComplete();

            // Debug
            Log.trace("MotionHelper StepNS X %ld Y %ld Z %ld)", _axisParams[0]._betweenStepsNs,
                        _axisParams[1]._betweenStepsNs, _axisParams[2]._betweenStepsNs);
            motionElem._pt1MM.logDebugStr("MotionFrom");
            motionElem._pt2MM.logDebugStr("MotionTo");
        }
        // Log.trace("Move to %sx %0.2f y %0.2f -> ax0Tgt %0.2f Ax1Tgt %0.2f (stpNSXY %ld %ld)",
        //             valid?"":"INVALID ", motionElem._pt2MM._pt[0], motionElem._pt2MM._pt[1],
        //             actuatorCoords._pt[0], actuatorCoords._pt[1],
        //             _axisParams[0]._betweenStepsNs, _axisParams[1]._betweenStepsNs);
    }
}

#else

// void MotionHelper::calcMotionTimeUs(MotionPipelineElem& motionElem, AxisParams& axisParams)
// {
//
// }

void MotionHelper::pipelineService(bool hasBeenPaused)
{
    // Check if any axis is moving
    bool anyAxisMoving = false;
    for (int i = 0; i < MAX_AXES; i++)
    {
        // Check if movement required
        if (_axisParams[i]._targetStepsFromHome == _axisParams[i]._stepsFromHome)
            continue;
        anyAxisMoving = true;
    }

    // If nothing moving then prep the next pipeline element (if there is one)
    if (!anyAxisMoving)
    {
        MotionPipelineElem motionElem;
        if (_motionPipeline.get(motionElem))
        {
            // Correct for any overflows in stepper values (may occur with rotational robots)
            _correctStepOverflowFn(_axisParams, _numRobotAxes);

            // Check if a real distance
            double distToTravelMM = motionElem.delta();
            bool valid = true;
            if (distToTravelMM < distToTravelMM_ignoreBelow)
                valid = false;

            // Get absolute step position to move to
            PointND actuatorCoords;
            if (valid)
                valid = _ptToActuatorFn(motionElem, actuatorCoords, _axisParams, _numRobotAxes);

            // Activate motion if valid - otherwise ignore
            if (valid)
            {
                // Get steps from home for each axis
                for (int i = 0; i < MAX_AXES; i++)
                    _axisParams[i]._targetStepsFromHome = actuatorCoords.getVal(i);

                // Balance the time for each direction
                // double calcMotionTime = calcMotionTimeUs(motionElem, axisParams);
                double speedTargetMMps = _axisParams[0]._maxSpeed;
                double timeToTargetS = distToTravelMM / speedTargetMMps;
                long stepsAxis[MAX_AXES];
                double timePerStepAxisNs[MAX_AXES];
                Log.trace("motionHelper speedTargetMMps %0.2f distToTravelMM %0.2f timeToTargetMS %0.2f",
                        speedTargetMMps, distToTravelMM, timeToTargetS*1000.0);
                for (int i = 0; i < MAX_AXES; i++)
                {
                    stepsAxis[i] = labs(_axisParams[i]._targetStepsFromHome - _axisParams[i]._stepsFromHome);
                    if (stepsAxis[i] == 0)
                        timePerStepAxisNs[i] = 1000000000;
                    else
                        timePerStepAxisNs[i] = timeToTargetS * 1000000000 / stepsAxis[i];
                    if (timePerStepAxisNs[i] < _axisParams[i]._minNsBetweenSteps)
                        timePerStepAxisNs[i] = _axisParams[i]._minNsBetweenSteps;
                    Log.trace("motionHelper axis%d target %ld fromHome %ld timerPerStepNs %f",
                                i, _axisParams[i]._targetStepsFromHome, _axisParams[i]._stepsFromHome,
                                timePerStepAxisNs[i]);
                }

                // Check if there is little difference from current timePerStep on the
                // dominant axis
                if (_axisParams[0]._isDominantAxis && isInBounds(timePerStepAxisNs[0],
                            _axisParams[0]._betweenStepsNs*0.66, _axisParams[0]._betweenStepsNs*1.33))
                {
                    // In that case use the dominant axis time with a correction factor
                    double speedCorrectionFactor = _axisParams[0]._betweenStepsNs / timePerStepAxisNs[0];
                    _axisParams[1]._betweenStepsNs = timePerStepAxisNs[1] * speedCorrectionFactor;
                }
                else if (_axisParams[1]._isDominantAxis && isInBounds(timePerStepAxisNs[1],
                            _axisParams[1]._betweenStepsNs*0.66, _axisParams[1]._betweenStepsNs*1.33))
                {
                    // In that case use the dominant axis time with a correction factor
                    double speedCorrectionFactor = _axisParams[1]._betweenStepsNs / timePerStepAxisNs[1];
                    _axisParams[0]._betweenStepsNs = timePerStepAxisNs[0] * speedCorrectionFactor;
                }
                else
                {
                    // allow each axis to use a different stepping time
                    _axisParams[0]._betweenStepsNs = timePerStepAxisNs[0];
                    _axisParams[1]._betweenStepsNs = timePerStepAxisNs[1];
                }
            }

            // Debug
            Log.info("MotionHelper StepNS X %ld Y %ld Z %ld)", _axisParams[0]._betweenStepsNs,
                        _axisParams[1]._betweenStepsNs, _axisParams[2]._betweenStepsNs);
            motionElem._pt1MM.logDebugStr("MotionFrom");
            motionElem._pt2MM.logDebugStr("MotionTo");

            // Log.trace("Move to %sx %0.2f y %0.2f -> ax0Tgt %0.2f Ax1Tgt %0.2f (stpNSXY %ld %ld)",
            //             valid?"":"INVALID ", motionElem._pt2MM._pt[0], motionElem._pt2MM._pt[1],
            //             actuatorCoords._pt[0], actuatorCoords._pt[1],
            //             _axisParams[0]._betweenStepsNs, _axisParams[1]._betweenStepsNs);
        }
    }

    // Make the next step on each axis as requred
    for (int i = 0; i < MAX_AXES; i++)
    {
        // Check if a move is required
        if (_axisParams[i]._targetStepsFromHome == _axisParams[i]._stepsFromHome)
            continue;

        // Check for servo driven axis (doesn't need individually stepping)
        if (_axisParams[i]._isServoAxis)
        {
            Log.trace("Servo(ax#%d) jump to %ld", i, _axisParams[i]._targetStepsFromHome);
            jump(i, _axisParams[i]._targetStepsFromHome);
            _axisParams[i]._stepsFromHome = _axisParams[i]._targetStepsFromHome;
            continue;
        }

        // Check if time to move
        if (hasBeenPaused || (Utils::isTimeout(micros(), _axisParams[i]._lastStepMicros, _axisParams[i]._betweenStepsNs / 1000)))
        {
            step(i, _axisParams[i]._targetStepsFromHome > _axisParams[i]._stepsFromHome);
            // Log.trace("Step %d %d", i, _axisParams[i]._targetStepsFromHome > _axisParams[i]._stepsFromHome);
            _axisParams[i]._betweenStepsNs += _axisParams[i]._betweenStepsNsChangePerStep;
        }
    }

    // Debug
    if (Utils::isTimeout(millis(), _debugLastPosDispMs, 1000))
    {
        if  (anyAxisMoving)
            Log.info("MotionHelper pipelineService isMoving fromHome x %ld y %ld z %ld",
                        _axisParams[0]._stepsFromHome, _axisParams[1]._stepsFromHome, _axisParams[2]._stepsFromHome);
        // else
        //     Log.info("-------> Nothing moving");
        _debugLastPosDispMs = millis();
    }
}
#endif

#ifdef USE_MOTION_ISR_MANAGER
bool MotionHelper::isMoving()
{
    return motionISRManager.isMoving();
}
#else
bool MotionHelper::isMoving()
{
    for (int i = 0; i < MAX_AXES; i++)
    {
        // Check if movement required - if so we are busy
        if (_axisParams[i]._targetStepsFromHome != _axisParams[i]._stepsFromHome)
            return true;
    }
    return false;
}
#endif

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
    int maxPossibleBlocksInMove = _maxMotionDistanceMM / _blockDistanceMM;
    // Log.trace("MotionHelper configMotionPipeline _maxMotionDistanceMM %0.2f _blockDistanceMM %0.2f",
    //         _maxMotionDistanceMM, _blockDistanceMM);
    _motionPipeline.setParameters(maxPossibleBlocksInMove);
}

MotionHelper::MotionHelper()
{
    // Init
    _isPaused = false;
    _wasPaused = false;
    _xMaxMM = 0;
    _yMaxMM = 0;
    _maxMotionDistanceMM = 0;
    _blockDistanceMM = blockDistanceMM_default;
    _numRobotAxes = 0;
    for (int i = 0; i < MAX_AXES; i++)
    {
        _stepperMotors[i] = NULL;
        _servoMotors[i] = NULL;
        for (int j = 0; j < MAX_ENDSTOPS_PER_AXIS; j++)
            _endStops[i][j] = NULL;
        _axisParams[i]._lastStepMicros = 0;
        _axisParams[i]._stepsFromHome = 0;
        _axisParams[i]._targetStepsFromHome = 0;
        _axisParams[i]._betweenStepsNs = 100 * 1000;
    }
    _stepEnablePin = -1;
    _stepEnableActiveLevel = true;
    _stepDisableSecs = 60.0;
    _ptToActuatorFn = NULL;
    _actuatorToPtFn = NULL;
    _correctStepOverflowFn = NULL;
    _motorEnLastMillis = 0;
    _motorEnLastUnixTime = 0;
    _moveRelative = false;
    configMotionPipeline();

#ifdef USE_MOTION_ISR_MANAGER
    motionISRManager.clear();
#endif
    // // TESTCODE
    // _axisParams[0]._betweenStepsNs = 5000 * 1000;
    // _axisParams[1]._betweenStepsNs = 100 * 1000;
}

MotionHelper::~MotionHelper()
{
    deinit();
}

void MotionHelper::setTransforms(ptToActuatorFnType ptToActuatorFn, actuatorToPtFnType actuatorToPtFn,
         correctStepOverflowFnType correctStepOverflowFn)
{
    // Store callbacks
    _ptToActuatorFn = ptToActuatorFn;
    _actuatorToPtFn = actuatorToPtFn;
    _correctStepOverflowFn = correctStepOverflowFn;
}

void MotionHelper::deinit()
{
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

#ifdef USE_MOTION_ISR_MANAGER
    motionISRManager.clear();
#endif
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

void MotionHelper::setAxisParams(const char* robotConfigJSON)
{
    // Deinitialise
    deinit();

    // Configure robot
    configure(robotConfigJSON);

    // Axes
    for (int i = 0; i < MAX_AXES; i++)
    {
        if (configureAxis(robotConfigJSON, i))
            _numRobotAxes = i+1;
    }

#ifdef USE_MOTION_ISR_MANAGER
    motionISRManager.start();
#endif

}

void MotionHelper::axisSetHome(int axisIdx)
{
    if (axisIdx < 0 || axisIdx >= MAX_AXES)
        return;
    if (_axisParams[axisIdx]._isServoAxis)
    {
        _axisParams[axisIdx]._homeOffsetSteps = _axisParams[axisIdx]._stepsFromHome;
    }
    else
    {
        _axisParams[axisIdx]._stepsFromHome = _axisParams[axisIdx]._homeOffsetSteps;
        _axisParams[axisIdx]._targetStepsFromHome = _axisParams[axisIdx]._homeOffsetSteps;
    }
    Log.trace("Setting axis#%d home %lu", axisIdx, _axisParams[axisIdx]._homeOffsetSteps);
#ifdef USE_MOTION_ISR_MANAGER
    motionISRManager.resetZero(axisIdx);
#endif
}

void MotionHelper::step(int axisIdx, bool direction)
{
    if (axisIdx < 0 || axisIdx >= MAX_AXES)
        return;

    enableMotors(true, false);
    if (_stepperMotors[axisIdx])
    {
        _stepperMotors[axisIdx]->step(direction);
    }
    _axisParams[axisIdx]._stepsFromHome += (direction ? 1 : -1);
    _axisParams[axisIdx]._lastStepMicros = micros();
}

void MotionHelper::jump (int axisIdx, long targetStepsFromHome)
{
    if (axisIdx < 0 || axisIdx >= MAX_AXES)
        return;

    if (_servoMotors[axisIdx])
    {
        Log.trace("Servo(ax#%d) target %ld", axisIdx, _axisParams[axisIdx]._targetStepsFromHome);
        _servoMotors[axisIdx]->writeMicroseconds(targetStepsFromHome);
    }
    _axisParams[axisIdx]._stepsFromHome = targetStepsFromHome;
}

void MotionHelper::jumpHome(int axisIdx)
{
    if (axisIdx < 0 || axisIdx >= MAX_AXES)
        return;

    if (_servoMotors[axisIdx])
    {
        Log.trace("Servo(ax#%d) jumpHome to %ld", axisIdx, _axisParams[axisIdx]._homeOffsetSteps);
        _servoMotors[axisIdx]->writeMicroseconds(_axisParams[axisIdx]._homeOffsetSteps);
    }
    _axisParams[axisIdx]._stepsFromHome = _axisParams[axisIdx]._homeOffsetSteps;
    _axisParams[axisIdx]._targetStepsFromHome = _axisParams[axisIdx]._homeOffsetSteps;
}

long MotionHelper::getAxisStepsFromHome(int axisIdx)
{
#ifdef USE_MOTION_ISR_MANAGER
    return motionISRManager.getStepsFromZero(axisIdx);
#else
    if (axisIdx < 0 || axisIdx >= MAX_AXES)
        return 0;
    return _axisParams[axisIdx]._stepsFromHome;
#endif
}

// Endstops
bool MotionHelper::isEndStopValid(int axisIdx, int endStopIdx)
{
    if (axisIdx < 0 || axisIdx >= MAX_AXES)
        return false;
    if (endStopIdx < 0 || endStopIdx >= MAX_ENDSTOPS_PER_AXIS)
        return false;
    return true;
}

bool MotionHelper::isAtEndStop(int axisIdx, int endStopIdx)
{
    // For safety return true in these cases
    if (axisIdx < 0 || axisIdx >= MAX_AXES)
        return true;
    if (endStopIdx < 0 || endStopIdx >= MAX_ENDSTOPS_PER_AXIS)
        return true;

    // Test endstop
    if(_endStops[axisIdx][endStopIdx])
        return _endStops[axisIdx][endStopIdx]->isAtEndStop();

    // All other cases return true (as this might be safer)
    return true;
}

void MotionHelper::moveTo(RobotCommandArgs& args)
{
    Log.trace("MotionHelper moveTo x %0.2f y %0.2f", args.pt._pt[0], args.pt._pt[1]);

    // Handle any motion parameters (such as relative movement, feedrate, etc)
    setMotionParams(args);

    // Get the starting position - this is either the destination of the
    // last block in the pipeline or the current target location of the
    // robot (whether currently moving or idle)
    // Peek at the last block in the pipeline if there is one
    PointND startPos;
    MotionPipelineElem lastPipelineEntry;
    bool queueValid = _motionPipeline.peekLast(lastPipelineEntry);
    if (queueValid)
    {
        startPos = lastPipelineEntry._pt2MM;
        Log.trace("MotionHelper using queue end, startPos X %0.2f Y %0.2f",
                startPos._pt[0], startPos._pt[1]);
    }
    else
    {
        PointND axisPosns;
        for (int i = 0; i < MAX_AXES; i++)
            axisPosns._pt[i] = _axisParams[i]._targetStepsFromHome;
        PointND curXy;
        _actuatorToPtFn(axisPosns, curXy, _axisParams, _numRobotAxes);
        startPos = curXy;
        Log.trace("MotionHelper queue empty startPos X %0.2f Y%0.2f",
                startPos._pt[0], startPos._pt[1]);
    }

    // Handle reltative motion and fill in the destPos for axes for
    // which values not specified
    // Don't use servo values for computing distance to travel
    PointND destPos = args.pt;
    bool includeDist[MAX_AXES];
    for (int i = 0; i < MAX_AXES; i++)
    {
        if (!args.valid.isValid(i))
        {
            destPos.setVal(i, startPos.getVal(i));
        }
        else
        {
            if (_moveRelative)
                destPos.setVal(i, startPos.getVal(i) + args.pt.getVal(i));
        }
        includeDist[i] = !_axisParams[i]._isServoAxis;
    }

    // Split up into blocks of maximum length
    double lineLen = destPos.distanceTo(startPos, includeDist);

    // Ensure at least one block
    int numBlocks = int(lineLen / _blockDistanceMM);
    if (numBlocks == 0)
        numBlocks = 1;
    Log.trace("MotionHelper numBlocks %d (lineLen %0.2f / blockDistMM %02.f)",
                numBlocks, lineLen, _blockDistanceMM);
    PointND steps = (destPos - startPos) / numBlocks;
    for (int i = 0; i < numBlocks; i++)
    {
        // Create block
        PointND pt1 = startPos + steps * i;
        PointND pt2 = startPos + steps * (i+1);
        // Fix any axes which are non-stepping
        for (int j = 0; j < MAX_AXES; j++)
        {
            if (_axisParams[j]._isServoAxis)
            {
                pt1.setVal(j, (i==0) ? startPos.getVal(j) : destPos.getVal(j));
                pt2.setVal(j, destPos.getVal(j));
            }
        }
        // If last block then just use end point coords
        if (i == numBlocks-1)
            pt2 = destPos;
        // Log.trace("Adding x %0.2f y %0.2f", blkX, blkY);
        // Add to pipeline
        if (!_motionPipeline.add(pt1, pt2))
            break;
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
    PointND axisPosns;
    for (int i = 0; i < MAX_AXES; i++)
        axisPosns._pt[i] = getAxisStepsFromHome(i);
    _actuatorToPtFn(axisPosns, args.pt, _axisParams, _numRobotAxes);
    PointNDValid validity(true, true, true);
    args.valid = validity;
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
        // Check for motor enable timeout
        if (_motorsAreEnabled && Utils::isTimeout(millis(), _motorEnLastMillis, (unsigned long)(_stepDisableSecs * 1000)))
            enableMotors(false, true);
    }
}
