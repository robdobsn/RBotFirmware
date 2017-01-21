// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "StepperMotor.h"

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
        // Serial.printlnf("EndStop %d, %d, %d", _pin, _activeLevel, _inputType);
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

class AxisParams
{
public:
    static constexpr double maxSpeed_default = 100.0;
    static constexpr double acceleration_default = 100.0;
    static constexpr double stepsPerUnit_default = 100.0;
    double _maxSpeed;
    double _acceleration;
    double _stepsPerUnit;

public:
    AxisParams()
    {
        _maxSpeed = maxSpeed_default;
        _acceleration = acceleration_default;
        _stepsPerUnit = stepsPerUnit_default;
    }
};

class MotionController
{
public:
    static const int MAX_AXES = 3;
    static const int MAX_ENDSTOPS_PER_AXIS = 2;
    static constexpr double motorDisableSecs_default = 60.0;

private:
    // Motors
    StepperMotor* _stepperMotors[MAX_AXES];
    // Axis parameters
    AxisParams _axisParams[MAX_AXES];
    // Step enable
    int _stepEnablePin;
    bool _stepEnableActiveLevel = true;
    // Motor enable
    double _motorDisableSecs;
    bool _motorsAreEnabled;
    unsigned long _motorEnLastMillis;
    // End stops
    EndStop* _endStops[MAX_AXES][MAX_ENDSTOPS_PER_AXIS];
    // Stepping times
    unsigned long _axisLastStepMicros[MAX_AXES];

private:

    bool configureMotorEnable(const char* robotConfigJSON)
    {
        // Get motor enable info
        String motorEnPinName = ConfigManager::getString("motorEnPin", "-1", robotConfigJSON);
        long motorEnOnVal = ConfigManager::getLong("motorEnOnVal", 1, robotConfigJSON);
        long motorEnPin = ConfigPinMap::getPinFromName(motorEnPinName.c_str());
        double motorDisableSecs = ConfigManager::getDouble("motorDisableSecs", motorDisableSecs_default, robotConfigJSON);
        Log.info("MotorEnable (pin %d, onVal %d, %0.2f secs)", motorEnPin, motorEnOnVal, motorDisableSecs);
        if (motorEnPin == -1)
            return false;
        return true;
    }

    bool configureAxis(const char* robotConfigJSON, int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return false;

        // Get rotation params
        String motorIdStr = "axis" + String(axisIdx);
        String axisJSON = ConfigManager::getString(motorIdStr, "{}", robotConfigJSON);
        if (axisJSON.length() == 0 || axisJSON.equals("{}"))
            return false;
        String stepPinName = ConfigManager::getString("stepPin", "-1", axisJSON);
        String dirnPinName = ConfigManager::getString("dirnPin", "-1", axisJSON);
        _axisParams[axisIdx]._maxSpeed = ConfigManager::getDouble("maxSpeed", AxisParams::maxSpeed_default, axisJSON);
        _axisParams[axisIdx]._acceleration = ConfigManager::getDouble("acceleration", AxisParams::acceleration_default, axisJSON);
        _axisParams[axisIdx]._stepsPerUnit = ConfigManager::getDouble("stepsPerUnit", AxisParams::stepsPerUnit_default, axisJSON);
        long stepPin = ConfigPinMap::getPinFromName(stepPinName.c_str());
        long dirnPin = ConfigPinMap::getPinFromName(dirnPinName.c_str());
        Log.info("Axis%d (step pin %d, dirn pin %d)", axisIdx, stepPin, dirnPin);
        if (stepPin == -1 || dirnPin == -1)
            return false;
    //
    //     new StepperMotor(StepperMotor::MOTOR_TYPE_DRIVER, orbitalStepperStepPin, orbitalStepperDirnPin);
    //
    //     // Get rotation end stop - can operate without a rotation end stop
    //     String axis1EndStop1JSON = ConfigManager::getString("axis1EndStop1", "{}", robotConfigStr);
    //     String axis1EndStop1PinName = ConfigManager::getString("sensePin", "-1", axis1EndStop1JSON);
    //     long axis1EndStop1ActiveLevel = ConfigManager::getLong("activeLevel", 1, axis1EndStop1JSON);
    //     String axis1EndStop1InputTypeStr = ConfigManager::getString("inputType", "", axis1EndStop1JSON);
    //     long axis1EndStop1Pin = ConfigPinMap::getPinFromName(axis1EndStop1PinName.c_str());
    //     int axis1EndStop1InputType = ConfigPinMap::getInputType(axis1EndStop1InputTypeStr.c_str());
    //     Log.info("%s Rotation EndStop 1 (sense %d, level %d, type %d)", _robotTypeName.c_str(), axis1EndStop1Pin,
    //                 axis1EndStop1ActiveLevel, axis1EndStop1InputType);
    //
    //
    //
    // }
    //
    //
    // // Get axis2 params
    // String axis2MotorJSON = ConfigManager::getString("axis2Motor", "{}", robotConfigStr);
    // String axis2StepPinName = ConfigManager::getString("stepPin", "-1", axis2MotorJSON);
    // String axis2DirnPinName = ConfigManager::getString("dirnPin", "-1", axis2MotorJSON);
    // _axis2MaxSpeed = ConfigManager::getDouble("maxSpeed", axis2MaxSpeed_default, axis2MotorJSON);
    // _axis2Accel = ConfigManager::getDouble("accel", axis2Accel_default, axis2MotorJSON);
    // _axis2StepsPerUnit = ConfigManager::getDouble("stepsPerUnit", axis2StepsPerUnit_default, axis2MotorJSON);
    // long axis2StepPin = ConfigPinMap::getPinFromName(axis2StepPinName.c_str());
    // long axis2DirnPin = ConfigPinMap::getPinFromName(axis2DirnPinName.c_str());
    // Log.info("%s axis2 (step %d, dirn %d)", _robotTypeName.c_str(), axis2StepPin, axis2DirnPin);
    // if (axis2StepPin == -1 || axis2DirnPin == -1)
    //     return false;
    //
    // // Get axis2 end stop - can't operate without a axis2 end stop
    // String axis2EndStop1JSON = ConfigManager::getString("axis2EndStop1", "{}", robotConfigStr);
    // String axis2EndStop1PinName = ConfigManager::getString("sensePin", "-1", axis2EndStop1JSON);
    // long axis2EndStop1ActiveLevel = ConfigManager::getLong("activeLevel", 1, axis2EndStop1JSON);
    // String axis2EndStop1InputTypeStr = ConfigManager::getString("inputType", "", axis2EndStop1JSON);
    // long axis2EndStop1Pin = ConfigPinMap::getPinFromName(axis2EndStop1PinName.c_str());
    // int axis2EndStop1InputType = ConfigPinMap::getInputType(axis2EndStop1InputTypeStr.c_str());
    // Log.info("%s axis2 EndStop 1 (sense %d, level %d, type %d)", _robotTypeName.c_str(), axis2EndStop1Pin,
    //             axis2EndStop1ActiveLevel, axis2EndStop1InputType);
}

protected:
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
        _motorDisableSecs = 60.0;
    }

    ~MotionController()
    {
        deinit();
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
        //Serial.printlnf("Enable %d, disable level %d, disable after time %0.2f", en, !_stepEnableActiveLevel, _motorDisableSecs);
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

        // // Construct stepper motors
        // _stepperMotors[0] = new StepperMotor(StepperMotor::MOTOR_TYPE_DRIVER, orbitalStepperStepPin, orbitalStepperDirnPin);
        // _stepperMotors[1] = new StepperMotor(StepperMotor::MOTOR_TYPE_DRIVER, linearStepperStepPin, linearStepperDirnPin);
        //
        // // Enable pin - initially disable
        // _stepEnablePin = stepEnablePin;
        // _stepEnableActiveLevel = stepEnableActiveLevel;
        // _motorDisableSecs = motorDisableSecs;
        // pinMode(_stepEnablePin, OUTPUT);
        // digitalWrite(_stepEnablePin, !_stepEnableActiveLevel);
        //
        // // End stops
        // _endStops[0][0] = new EndStop(orbitalEndStop1Pin, orbitalEndStop1ActiveLevel, orbitalEndStop1InputType);
        // _endStops[1][0] = new EndStop(linearEndStop1Pin, linearEndStop1ActiveLevel, linearEndStop1InputType);
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
        _axisLastStepMicros[axisIdx] = micros();
    }

    unsigned long getAxisLastStepMicros(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= MAX_AXES)
            return 0;
        return _axisLastStepMicros[axisIdx];
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

    // Homing command
    void home(RobotCommandArgs& args)
    {
    }

    void service()
    {
        /// NOTE:
        /// Perhaps need to only disable motors after timeout from when motors stopped moving?
        /// How to stop motors on emergency
        /// How to convert coordinates
        /// How to go home

        // Check for motor enable timeout
        if (_motorsAreEnabled && Utils::isTimeout(millis(), _motorEnLastMillis, (unsigned long)(_motorDisableSecs * 1000)))
        {
            enableMotors(false);
        }
        //
        // // Run the steppers
        // _pRotationStepper->run();
        // _pXaxis2Stepper->run();

    }
};
