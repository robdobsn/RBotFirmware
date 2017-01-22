// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "StepperMotor.h"
#include "AxisParams.h"
#include "ConfigPinMap.h"

typedef void (*xyToActuatorFnType) (double xy[], double actuatorCoords[], AxisParams axisParams[], int numAxes);
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
    // Stepping times
    unsigned long _axisLastStepMicros[MAX_AXES];
    // Callbacks for coordinate conversion etc
    xyToActuatorFnType _xyToActuatorFn;
    actuatorToXyFnType _actuatorToXyFn;
    correctStepOverflowFnType _correctStepOverflowFn;

private:

    bool configureMotorEnable(const char* robotConfigJSON)
    {
        // Get motor enable info
        String stepEnablePinName = ConfigManager::getString("stepEnablePin", "-1", robotConfigJSON);
        _stepEnableActiveLevel = ConfigManager::getLong("stepEnableActiveLevel", 1, robotConfigJSON);
        _stepEnablePin = ConfigPinMap::getPinFromName(stepEnablePinName.c_str());
        _stepDisableSecs = ConfigManager::getDouble("stepDisableSecs", stepDisableSecs_default, robotConfigJSON);
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

public:
    bool isBusy()
    {
        // bool steppersRunning = false;
        // if (_pRotationStepper)
        //     steppersRunning |= (_pRotationStepper->distanceToGo() != 0);
        // if (_pXaxis2Stepper)
        //     steppersRunning |= (_pXaxis2Stepper->distanceToGo() != 0);
        // return steppersRunning;
        return false;
    }

public:
    MotionController()
    {
        // Init
        for (int i = 0; i < MAX_AXES; i++)
        {
            _stepperMotors[i] = NULL;
            for (int j = 0; j < MAX_ENDSTOPS_PER_AXIS; j++)
                _endStops[i][j] = NULL;
            _axisLastStepMicros[i] = 0;
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
            configureAxis(robotConfigJSON, i);
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
        _axisLastStepMicros[axisIdx] = micros();
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
        return _axisLastStepMicros[axisIdx];
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
        // // Check if steppers are running - if so ignore the command
        // bool steppersMoving = isBusy();
        //
        // // Info
        // Log.info("%s moveTo %f(%d),%f(%d),%f(%d) %s", _robotTypeName.c_str(), args.xVal, args.xValid,
        //             args.yVal, args.yValid, args.zVal, args.zValid, steppersMoving ? "BUSY" : "");
        //
        // // Check if busy
        // if (steppersMoving)
        //     return;
        //
        // // Enable motors
        // enableMotors(true);
        //
        // // Start moving
        // if (args.yValid && _pRotationStepper)
        // {
        //     Log.info("Rotation to %ld", (long)args.yVal);
        //     _pRotationStepper->moveTo((long)args.yVal);
        // }
        // if (args.xValid && _pXaxis2Stepper)
        // {
        //     Log.info("axis2 to %ld", (long)args.xVal);
        //     _pXaxis2Stepper->moveTo(args.xVal);
        // }
    }

    void service()
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

        // Avoid any overflows in stepper positions


        //
        // // Run the steppers
        // _pRotationStepper->run();
        // _pXaxis2Stepper->run();

    }
};
