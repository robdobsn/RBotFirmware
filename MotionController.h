// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "StepperMotor.h"
#include "AxisParams.h"
#include "ConfigPinMap.h"
#include "MotionPipeline.h"
#include "math.h"

typedef bool (*xyToActuatorFnType) (double xy[], double actuatorCoords[], AxisParams axisParams[], int numAxes);
typedef void (*actuatorToXyFnType) (double actuatorCoords[], double xy[], AxisParams axisParams[], int numAxes);
typedef void (*correctStepOverflowFnType) (AxisParams axisParams[], int numAxes);

class EndStop
{
private:
    int _pin;
    bool _activeLevel;
    int _inputType;

public:
    EndStop(int pin, bool activeLevel, int inputType)
    {
        _pin = pin;
        _activeLevel = activeLevel;
        _inputType = inputType;
        if (_pin != -1)
        {
            pinMode(_pin, (PinMode) _inputType);
        }
        Serial.printlnf("EndStop %d, %d, %d", _pin, _activeLevel, _inputType);
    }
    ~EndStop()
    {
        // Restore pin to input (may have had pullup)
        if (_pin != -1)
            pinMode(_pin, INPUT);
    }
    bool isAtEndStop()
    {
        if (_pin != -1)
        {
            bool val = digitalRead(_pin);
            return val == _activeLevel;
        }
        return true;
    }
};

class MotionController
{
public:
    static const int MAX_AXES = 3;
    static const int MAX_ENDSTOPS_PER_AXIS = 2;
    static constexpr double stepDisableSecs_default = 60.0;

private:
    // Robot dimensions
    double _xMaxMM;
    double _yMaxMM;
    double _maxMotionDistanceMM;
    // Motors
    StepperMotor* _stepperMotors[MAX_AXES];
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
    xyToActuatorFnType _xyToActuatorFn;
    actuatorToXyFnType _actuatorToXyFn;
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
        Log.info("MotorEnable (pin %d, onVal %d, %0.2f secs)", _stepEnablePin, _stepEnableActiveLevel, _stepDisableSecs);

        // Enable pin - initially disable
        pinMode(_stepEnablePin, OUTPUT);
        digitalWrite(_stepEnablePin, !_stepEnableActiveLevel);
    }

    bool configureAxis(const char* robotConfigJSON, int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return false;

        // Get rotation params
        String axisIdStr = "axis" + String(axisIdx);
        String axisJSON = ConfigManager::getString(axisIdStr, "{}", robotConfigJSON);
        if (axisJSON.length() == 0 || axisJSON.equals("{}"))
            return false;

        // Stepper motor
        String stepPinName = ConfigManager::getString("stepPin", "-1", axisJSON);
        String dirnPinName = ConfigManager::getString("dirnPin", "-1", axisJSON);
        _axisParams[axisIdx]._maxSpeed = ConfigManager::getDouble("maxSpeed", AxisParams::maxSpeed_default, axisJSON);
        _axisParams[axisIdx]._acceleration = ConfigManager::getDouble("acceleration", AxisParams::acceleration_default, axisJSON);
        _axisParams[axisIdx]._stepsPerRotation = ConfigManager::getDouble("stepsPerRotation", AxisParams::stepsPerRotation_default, axisJSON);
        _axisParams[axisIdx]._unitsPerRotation = ConfigManager::getDouble("unitsPerRotation", AxisParams::unitsPerRotation_default, axisJSON);
        _axisParams[axisIdx]._minVal = ConfigManager::getDouble("minVal", 0, _axisParams[axisIdx]._minValValid, axisJSON);
        _axisParams[axisIdx]._maxVal = ConfigManager::getDouble("maxVal", 0, _axisParams[axisIdx]._maxValValid, axisJSON);
        long stepPin = ConfigPinMap::getPinFromName(stepPinName.c_str());
        long dirnPin = ConfigPinMap::getPinFromName(dirnPinName.c_str());
        Log.info("Axis%d (step pin %d, dirn pin %d)", axisIdx, stepPin, dirnPin);
        if ((stepPin != -1 && dirnPin != -1))
          _stepperMotors[axisIdx] = new StepperMotor(StepperMotor::MOTOR_TYPE_DRIVER, stepPin, dirnPin);

        // End stops
        for (int endStopIdx =0; endStopIdx < MAX_ENDSTOPS_PER_AXIS; endStopIdx++)
        {
            // Get the config for endstop if present
            String endStopIdStr = "endStop" + String(endStopIdx);
            String endStopJSON = ConfigManager::getString(endStopIdStr, "{}", axisJSON);
            if (endStopJSON.length() == 0 || endStopJSON.equals("{}"))
                continue;

            // Get end stop
            String pinName = ConfigManager::getString("sensePin", "-1", endStopJSON);
            long activeLevel = ConfigManager::getLong("activeLevel", 1, endStopJSON);
            String inputTypeStr = ConfigManager::getString("inputType", "", endStopJSON);
            long pinId = ConfigPinMap::getPinFromName(pinName.c_str());
            int inputType = ConfigPinMap::getInputType(inputTypeStr.c_str());
            Log.info("Axis%dEndStop%d (sense %d, level %d, type %d)", axisIdx, endStopIdx, pinId,
                    activeLevel, inputType);

            // Create end stop
            _endStops[axisIdx][endStopIdx] = new EndStop(pinId, activeLevel, inputType);
        }

        // Other data
        _axisParams[axisIdx]._stepsFromHome = 0;
    }

    void pipelineService()
    {
        // Check if we are ready for the next step on each axis
        bool anyAxisMoving = false;
        for (int i = 0; i < MAX_AXES; i++)
        {
            // Check if movement required
            if (_axisParams[i]._targetStepsFromHome == _axisParams[i]._stepsFromHome)
                continue;
            anyAxisMoving = true;

            // Check if time to move
            if (Utils::isTimeout(micros(), _axisParams[i]._lastStepMicros, _axisParams[i]._betweenStepsUs))
            {
                step(i, _axisParams[i]._targetStepsFromHome > _axisParams[i]._stepsFromHome);
                // Serial.printlnf("Step %d %d", i, _axisParams[i]._targetStepsFromHome > _axisParams[i]._stepsFromHome);
            }
        }

        // If nothing moving then prep the next pipeline element
        if (!anyAxisMoving)
        {
            // Get element
            MotionPipelineElem motionElem;
             if (_motionPipeline.get(motionElem))
             {
                 // Correct for any overflows in stepper values (may occur with rotational robots)
                 _correctStepOverflowFn(_axisParams, _numRobotAxes);

                // Get steps to move
                double xy[MAX_AXES] = { motionElem._xPos, motionElem._yPos };
                double actuatorCoords[MAX_AXES];
                bool valid = _xyToActuatorFn(xy, actuatorCoords, _axisParams, _numRobotAxes);

                // Activate motion if valid - otherwise ignore
                if (valid)
                {
                    _axisParams[0]._targetStepsFromHome = (int)(actuatorCoords[0]);
                    _axisParams[1]._targetStepsFromHome = (int)(actuatorCoords[1]);
                }

                Serial.printlnf("Move to %sx %0.2f y %0.2f -> ax0Tgt %0.2f Ax1Tgt %0.2f",
                            valid?"":"INVALID ", xy[0], xy[1], actuatorCoords[0], actuatorCoords[1]);
            }
        }

        // Debug
        if (Utils::isTimeout(millis(), _debugLastPosDispMs, 1000) && anyAxisMoving)
        {
            Log.info("-------> %0d %0d", _axisParams[0]._stepsFromHome, _axisParams[1]._stepsFromHome);
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
            for (int j = 0; j < MAX_ENDSTOPS_PER_AXIS; j++)
                _endStops[i][j] = NULL;
            _axisParams[i]._lastStepMicros = 0;
            _axisParams[i]._stepsFromHome = 0;
            _axisParams[i]._targetStepsFromHome = 0;
            _axisParams[i]._betweenStepsUs = 100;
        }
        _stepEnablePin = -1;
        _stepEnableActiveLevel = true;
        _stepDisableSecs = 60.0;
        _xyToActuatorFn = NULL;
        _actuatorToXyFn = NULL;
        _correctStepOverflowFn = NULL;
    }

    ~MotionController()
    {
        deinit();
    }

    void setTransforms(xyToActuatorFnType xyToActuatorFn, actuatorToXyFnType actuatorToXyFn,
             correctStepOverflowFnType correctStepOverflowFn)
    {
        // Store callbacks
        _xyToActuatorFn = xyToActuatorFn;
        _actuatorToXyFn = actuatorToXyFn;
        _correctStepOverflowFn = correctStepOverflowFn;
    }

    void deinit()
    {
        // remove steppers and end stops
        for (int i = 0; i < MAX_AXES; i++)
        {
            delete _stepperMotors[i];
            _stepperMotors[i] = NULL;
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

    void setOrbitalParams(const char* robotConfigJSON)
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
        if (!args.xValid || !args.yValid)
            return;

        // Get the starting position - this is either the last block in the pipeline or
        // the target location of the robot (if currently moving or idle)
        // Peek at the last block in the pipeline if there is one
        MotionPipelineElem startPos;
        bool queueValid = _motionPipeline.peekLast(startPos);
        if (!queueValid)
        {
            double axisPosns[MAX_AXES] = {
                _axisParams[0]._stepsFromHome,
                _axisParams[1]._stepsFromHome
            };
            double curXy[MAX_AXES];
            _actuatorToXyFn(axisPosns, curXy, _axisParams, _numRobotAxes);
            startPos._xPos = curXy[0];
            startPos._yPos = curXy[1];
        }

        _motionPipeline.add(args.xVal, args.yVal);
        //
        // // Split up into blocks of maximum length
        // double xDiff = args.xVal - startPos._xPos;
        // double yDiff = args.yVal - startPos._yPos;
        // int lineLen = sqrt(xDiff * xDiff + yDiff * yDiff);
        // // Ensure at least one block
        // int numBlocks = int(lineLen / _blockDistanceMM) + 1;
        // double xStep = xDiff / numBlocks;
        // double yStep = yDiff / numBlocks;
        // for (int i = 0; i < numBlocks; i++)
        // {
        //     // Create block
        //     double blkX = startPos._xPos + xStep * i;
        //     double blkY = startPos._yPos + yStep * i;
        //     // If last block then just use end point coords
        //     if (i == numBlocks-1)
        //     {
        //         blkX = args.xVal;
        //         blkY = args.yVal;
        //     }
        //     // Serial.printlnf("Adding x %0.2f y %0.2f", blkX, blkY);
        //     // Add to pipeline
        //     if (!_motionPipeline.add(blkX, blkY))
        //         break;
        // }

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
