// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "StepperMotor.h"
#include "AxisParams.h"
#include "EndStop.h"
#include "ConfigPinMap.h"
#include "MotionPipeline.h"
#include "math.h"

typedef bool (*ptToActuatorFnType) (PointND& pt, PointND& actuatorCoords, AxisParams axisParams[], int numAxes);
typedef void (*actuatorToPtFnType) (PointND& actuatorCoords, PointND& xy, AxisParams axisParams[], int numAxes);
typedef void (*correctStepOverflowFnType) (AxisParams axisParams[], int numAxes);

class MotionController
{
public:
    static const int MAX_ENDSTOPS_PER_AXIS = 2;
    static constexpr double stepDisableSecs_default = 60.0;

private:
    // Robot dimensions
    double _xMaxMM;
    double _yMaxMM;
    double _maxMotionDistanceMM;
    // Stepper motors
    StepperMotor* _stepperMotors[MAX_AXES];
    // Servo motors
    Servo* _servoMotors[MAX_AXES];
    // Axis parameters
    AxisParams _axisParams[MAX_AXES];
    // Step enable
    int _stepEnablePin;
    bool _stepEnableActiveLevel = true;
    // Motor enable
    double _stepDisableSecs;
    bool _motorsAreEnabled;
    unsigned long _motorEnLastMillis;
    // End stops
    EndStop* _endStops[MAX_AXES][MAX_ENDSTOPS_PER_AXIS];
    // Callbacks for coordinate conversion etc
    ptToActuatorFnType _ptToActuatorFn;
    actuatorToPtFnType _actuatorToPtFn;
    correctStepOverflowFnType _correctStepOverflowFn;
    // Number of actual axes of motion
    int _numRobotAxes;
    // Motion pipeline
    MotionPipeline _motionPipeline;
    // Block distance
    double _blockDistanceMM;

    // Debug
    unsigned long _debugLastPosDispMs;

private:

    bool isInBounds(double v, double b1, double b2)
    {
        return (v > min(b1, b2) && v < max(b1,b2));
    }

    bool configureMotorEnable(const char* robotConfigJSON)
    {
        // Get motor enable info
        String stepEnablePinName = ConfigManager::getString("stepEnablePin", "-1", robotConfigJSON);
        _stepEnableActiveLevel = ConfigManager::getLong("stepEnableActiveLevel", 1, robotConfigJSON);
        _stepEnablePin = ConfigPinMap::getPinFromName(stepEnablePinName.c_str());
        _stepDisableSecs = ConfigManager::getDouble("stepDisableSecs", stepDisableSecs_default, robotConfigJSON);
        _xMaxMM = ConfigManager::getDouble("xMaxMM", 0, robotConfigJSON);
        _yMaxMM = ConfigManager::getDouble("yMaxMM", 0, robotConfigJSON);
        _maxMotionDistanceMM = sqrt(_xMaxMM * _xMaxMM + _yMaxMM * _yMaxMM);
        Log.info("MotorEnable (pin %d, actLvl %d, disableAfter %0.2fs)", _stepEnablePin, _stepEnableActiveLevel, _stepDisableSecs);

        // Enable pin - initially disable
        pinMode(_stepEnablePin, OUTPUT);
        digitalWrite(_stepEnablePin, !_stepEnableActiveLevel);
    }

    bool configureAxis(const char* robotConfigJSON, int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return false;

        // Get params
        String axisIdStr = "axis" + String(axisIdx);
        String axisJSON = ConfigManager::getString(axisIdStr, "{}", robotConfigJSON);
        if (axisJSON.length() == 0 || axisJSON.equals("{}"))
            return false;

        // Set the axis parameters
        _axisParams[axisIdx].setFromJSON(axisJSON.c_str());

        // Check the kind of motor to use
        bool isValid = false;
        String stepPinName = ConfigManager::getString("stepPin", "-1", axisJSON, isValid);
        if (isValid)
        {
            // Create the stepper motor for the axis
            String dirnPinName = ConfigManager::getString("dirnPin", "-1", axisJSON);
            long stepPin = ConfigPinMap::getPinFromName(stepPinName.c_str());
            long dirnPin = ConfigPinMap::getPinFromName(dirnPinName.c_str());
            Log.info("Axis%d (step pin %d, dirn pin %d)", axisIdx, stepPin, dirnPin);
            if ((stepPin != -1 && dirnPin != -1))
                _stepperMotors[axisIdx] = new StepperMotor(StepperMotor::MOTOR_TYPE_DRIVER, stepPin, dirnPin);
        }
        else
        {
            // Create a servo motor for the axis
            String servoPinName = ConfigManager::getString("servoPin", "-1", axisJSON);
            long servoPin = ConfigPinMap::getPinFromName(servoPinName.c_str());
            Log.info("Axis%d (servo pin %d)", axisIdx, servoPin);
            if ((servoPin != -1))
            {
                _servoMotors[axisIdx] = new Servo();
                if (_servoMotors[axisIdx])
                    _servoMotors[axisIdx]->attach(servoPin);

            }
        }

        // End stops
        for (int endStopIdx =0; endStopIdx < MAX_ENDSTOPS_PER_AXIS; endStopIdx++)
        {
            // Get the config for endstop if present
            String endStopIdStr = "endStop" + String(endStopIdx);
            String endStopJSON = ConfigManager::getString(endStopIdStr, "{}", axisJSON);
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
    }

    void pipelineService()
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

                // Get absolute step position to move to
                PointND actuatorCoords;
                bool valid = _ptToActuatorFn(motionElem._pt2MM, actuatorCoords, _axisParams, _numRobotAxes);

                // Activate motion if valid - otherwise ignore
                if (valid)
                {
                    for (int i = 0; i < MAX_AXES; i++)
                        _axisParams[i]._targetStepsFromHome = actuatorCoords.getVal(i);

                    // Balance the time for each direction
                    double speedTargetMMps = _axisParams[0]._maxSpeed;
                    double distToTravelMM = motionElem.delta();
                    double timeToTargetS = distToTravelMM / speedTargetMMps;
                    unsigned long stepsAxis[MAX_AXES];
                    double timePerStepAxisNs[MAX_AXES];
                    for (int i = 0; i < MAX_AXES; i++)
                    {
                        stepsAxis[i] = labs(_axisParams[i]._targetStepsFromHome - _axisParams[i]._stepsFromHome);
                        timePerStepAxisNs[i] = timeToTargetS * 1000000000 / stepsAxis[i];
                        Log.trace("Bing %ld %ld %ld %f", _axisParams[i]._targetStepsFromHome, _axisParams[i]._stepsFromHome, stepsAxis[i], timePerStepAxisNs[i]);
                    }

                    // Check if there is little difference from current timePerStep on the
                    // dominant axis
                    if (_axisParams[0]._isDominantAxis && isInBounds(timePerStepAxisNs[0],
                                _axisParams[0]._betweenStepsNs*0.66, _axisParams[0]._betweenStepsNs*1.33))
                    {
                        double speedCorrectionFactor = _axisParams[0]._betweenStepsNs / timePerStepAxisNs[0];
                        _axisParams[1]._betweenStepsNs = timePerStepAxisNs[1] * speedCorrectionFactor;
                    }
                    else if (_axisParams[1]._isDominantAxis && isInBounds(timePerStepAxisNs[1],
                                _axisParams[1]._betweenStepsNs*0.66, _axisParams[1]._betweenStepsNs*1.33))
                    {
                        double speedCorrectionFactor = _axisParams[1]._betweenStepsNs / timePerStepAxisNs[1];
                        _axisParams[0]._betweenStepsNs = timePerStepAxisNs[0] * speedCorrectionFactor;
                    }
                    else
                    {
                        _axisParams[0]._betweenStepsNs = timePerStepAxisNs[0];
                        _axisParams[1]._betweenStepsNs = timePerStepAxisNs[1];
                    }
                }

                Log.trace("MoveTo (stpNS XYZ %ld %ld %ld)", _axisParams[1]._betweenStepsNs,
                            _axisParams[1]._betweenStepsNs, _axisParams[2]._betweenStepsNs);
                motionElem._pt1MM.logDebugStr();
                motionElem._pt2MM.logDebugStr();

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
                Log.info("Servo jump to %ld", _axisParams[i]._targetStepsFromHome);
                jump(i, _axisParams[i]._targetStepsFromHome);
                _axisParams[i]._stepsFromHome = _axisParams[i]._targetStepsFromHome;
                continue;
            }

            // Check if time to move
            if (Utils::isTimeout(micros(), _axisParams[i]._lastStepMicros, _axisParams[i]._betweenStepsNs / 1000))
            {
                step(i, _axisParams[i]._targetStepsFromHome > _axisParams[i]._stepsFromHome);
                // Serial.printlnf("Step %d %d", i, _axisParams[i]._targetStepsFromHome > _axisParams[i]._stepsFromHome);
                _axisParams[i]._betweenStepsNs += _axisParams[i]._betweenStepsNsChangePerStep;
            }
        }

        // Debug
        if (Utils::isTimeout(millis(), _debugLastPosDispMs, 1000))
        {
            if  (anyAxisMoving)
                Log.info("-------> %ld %ld %ld", _axisParams[0]._stepsFromHome, _axisParams[1]._stepsFromHome, _axisParams[2]._stepsFromHome);
            // else
            //     Log.info("-------> Nothing moving");
            _debugLastPosDispMs = millis();
        }
    }

public:
    bool isMoving()
    {
        for (int i = 0; i < MAX_AXES; i++)
        {
            // Check if movement required - if so we are busy
            if (_axisParams[i]._targetStepsFromHome != _axisParams[i]._stepsFromHome)
                return true;
        }
        return false;
    }

    bool canAcceptCommand()
    {
        // Check if there is enough room in the pipeline for the maximum possible number of
        // blocks that a movement command can translate into
        int maxPossibleBlocks = _maxMotionDistanceMM / _blockDistanceMM;
        return _motionPipeline.slotsEmpty() >= maxPossibleBlocks;
    }

public:
    MotionController()
    {
        // Init
        _xMaxMM = 0;
        _yMaxMM = 0;
        _maxMotionDistanceMM = 0;
        _blockDistanceMM = 1;
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

        // // TESTCODE
        // _axisParams[0]._betweenStepsNs = 5000 * 1000;
        // _axisParams[1]._betweenStepsNs = 100 * 1000;
    }

    ~MotionController()
    {
        deinit();
    }

    void setTransforms(ptToActuatorFnType ptToActuatorFn, actuatorToPtFnType actuatorToPtFn,
             correctStepOverflowFnType correctStepOverflowFn)
    {
        // Store callbacks
        _ptToActuatorFn = ptToActuatorFn;
        _actuatorToPtFn = actuatorToPtFn;
        _correctStepOverflowFn = correctStepOverflowFn;
    }

    void deinit()
    {
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

        // disable
        if (_stepEnablePin != -1)
            pinMode(_stepEnablePin, INPUT);
    }

    void enableMotors(bool en)
    {
        //Serial.printlnf("Enable %d, disable level %d, disable after time %0.2f", en, !_stepEnableActiveLevel, _stepDisableSecs);
        if (en)
        {
            if (_stepEnablePin != -1)
                digitalWrite(_stepEnablePin, _stepEnableActiveLevel);
            _motorsAreEnabled = true;
            _motorEnLastMillis = millis();
        }
        else
        {
            if (_stepEnablePin != -1)
                digitalWrite(_stepEnablePin, !_stepEnableActiveLevel);
            _motorsAreEnabled = false;
        }
    }

    void setAxisParams(const char* robotConfigJSON)
    {
        // Deinitialise
        deinit();

        // Motor enables
        configureMotorEnable(robotConfigJSON);

        // Axes
        for (int i = 0; i < MAX_AXES; i++)
        {
            if (configureAxis(robotConfigJSON, i))
                _numRobotAxes = i+1;
        }
    }

    void axisIsHome(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return;
        _axisParams[axisIdx]._stepsFromHome = 0;
    }

    void step(int axisIdx, bool direction)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return;

        enableMotors(true);
        if (_stepperMotors[axisIdx])
        {
            _stepperMotors[axisIdx]->step(direction);
        }
        _axisParams[axisIdx]._stepsFromHome += (direction ? 1 : -1);
        _axisParams[axisIdx]._lastStepMicros = micros();
    }

    void jump (int axisIdx, long targetStepsFromHome)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return;

        if (_servoMotors[axisIdx])
        {
            Log.trace("SERVO %d target %ld", axisIdx, _axisParams[axisIdx]._targetStepsFromHome);
            _servoMotors[axisIdx]->writeMicroseconds(targetStepsFromHome);
        }
        _axisParams[axisIdx]._stepsFromHome = targetStepsFromHome;
    }

    void jumpHome(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return;

        if (_servoMotors[axisIdx])
        {
            Log.trace("SERVO %d jumpHome to %ld", axisIdx, _axisParams[axisIdx]._homeOffsetSteps);
            _servoMotors[axisIdx]->writeMicroseconds(_axisParams[axisIdx]._homeOffsetSteps);
        }
        _axisParams[axisIdx]._stepsFromHome = _axisParams[axisIdx]._homeOffsetSteps;
        _axisParams[axisIdx]._targetStepsFromHome = _axisParams[axisIdx]._homeOffsetSteps;
    }

    unsigned long getAxisStepsFromHome(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return 0;
        return _axisParams[axisIdx]._stepsFromHome;
    }

    double getStepsPerUnit(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return 0;
        return _axisParams[axisIdx].stepsPerUnit();
    }

    double getStepsPerRotation(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return 0;
        return _axisParams[axisIdx]._stepsPerRotation;
    }

    double getUnitsPerRotation(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return 0;
        return _axisParams[axisIdx]._unitsPerRotation;
    }

    unsigned long getAxisLastStepMicros(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return 0;
        return _axisParams[axisIdx]._lastStepMicros;
    }

    AxisParams* getAxisParamsArray()
    {
        return _axisParams;
    }

    // Endstops
    bool isEndStopValid(int axisIdx, int endStopIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return false;
        if (endStopIdx < 0 || endStopIdx >= MAX_ENDSTOPS_PER_AXIS)
            return false;
        return true;
    }

    bool isAtEndStop(int axisIdx, int endStopIdx)
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

    void moveTo(RobotCommandArgs& args)
    {
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
        }
        else
        {
            PointND axisPosns;
            for (int i = 0; i < MAX_AXES; i++)
                axisPosns._pt[i] = _axisParams[i]._stepsFromHome;
            PointND curXy;
            _actuatorToPtFn(axisPosns, curXy, _axisParams, _numRobotAxes);
            startPos = curXy;
        }

        // Fill in the destPos for axes for which values not specified
        // and don't use those values for computing distance to travel
        PointND destPos = args.pt;
        bool includeDist[MAX_AXES];
        for (int i = 0; i < MAX_AXES; i++)
        {
            if (!args.valid.isValid(i))
                destPos.setVal(i, startPos.getVal(i));
            includeDist[i] = !_axisParams[i]._isServoAxis;
        }

        // Split up into blocks of maximum length
        double lineLen = destPos.distanceTo(startPos, includeDist);

        // Ensure at least one block
        int numBlocks = int(lineLen / _blockDistanceMM);
        if (numBlocks == 0)
            numBlocks = 1;
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
            // Serial.printlnf("Adding x %0.2f y %0.2f", blkX, blkY);
            // Add to pipeline
            if (!_motionPipeline.add(pt1, pt2))
                break;
        }

    }

    void service(bool processPipeline)
    {
        /// NOTE:
        /// Perhaps need to only disable motors after timeout from when motors stopped moving?
        /// How to stop motors on emergency
        /// How to convert coordinates
        /// How to go home

        // Check for motor enable timeout
        if (_motorsAreEnabled && Utils::isTimeout(millis(), _motorEnLastMillis, (unsigned long)(_stepDisableSecs * 1000)))
        {
            enableMotors(false);
        }

        // Check if we should process the movement pipeline
        if (processPipeline)
            pipelineService();

        // Avoid any overflows in stepper positions


        //
        // // Run the steppers
        // _pRotationStepper->run();
        // _pXaxis2Stepper->run();

    }
};
