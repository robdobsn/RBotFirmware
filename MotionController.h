// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "StepperMotor.h"

class MotionController
{
private:
    StepperMotor* _pStepper1;
    StepperMotor* _pStepper2;
    int _stepEnablePin;
    bool _enableActiveLevel = true;
    // Motor enable
    double _motorDisableSecs;
    bool _motorsAreEnabled;
    unsigned long _motorEnLastMillis;

protected:
    bool isBusy()
    {
        // bool steppersRunning = false;
        // if (_pRotationStepper)
        //     steppersRunning |= (_pRotationStepper->distanceToGo() != 0);
        // if (_pXLinearStepper)
        //     steppersRunning |= (_pXLinearStepper->distanceToGo() != 0);
        // return steppersRunning;
        return false;
    }

public:
    MotionController()
    {
        // Init
        _pStepper1 = _pStepper2 = NULL;
        _stepEnablePin = -1;
        _enableActiveLevel = true;
    }

    ~MotionController()
    {
        deinit();
    }

    void deinit()
    {
        // remove steppers
        delete _pStepper1;
        _pStepper1 = NULL;
        delete _pStepper2;
        _pStepper2 = NULL;
        // disable
        if (_stepEnablePin != -1)
            pinMode(_stepEnablePin, INPUT);
    }

    void enableMotors(bool en)
    {
        if (en)
        {
            if (_stepEnablePin != -1)
                digitalWrite(_stepEnablePin, _enableActiveLevel);
            _motorsAreEnabled = true;
            _motorEnLastMillis = millis();
        }
        else
        {
            if (_stepEnablePin != -1)
                digitalWrite(_stepEnablePin, !_enableActiveLevel);
            _motorsAreEnabled = false;
        }
    }

    void setOrbitalParams(int orbitalStepperStepPin, int orbitalStepperDirnPin,
                int linearStepperStepPin, int linearStepperDirnPin,
                int stepEnablePin, int motorDisableSecs)
    {
        // Deinitialise
        deinit();

        // Construct stepper motors
        _pStepper1 = new StepperMotor(StepperMotor::MOTOR_TYPE_DRIVER, orbitalStepperStepPin, orbitalStepperDirnPin);
        _pStepper2 = new StepperMotor(StepperMotor::MOTOR_TYPE_DRIVER, linearStepperStepPin, linearStepperDirnPin);

        // Enable pin - initially disable
        _stepEnablePin = stepEnablePin;
        pinMode(_stepEnablePin, OUTPUT);
        digitalWrite(_stepEnablePin, !_enableActiveLevel);
    }

    void step1Orbital(bool direction)
    {
        enableMotors(true);
        if (_pStepper1)
            _pStepper1->step(direction);
    }

    void step1Linear(bool direction)
    {
        enableMotors(true);
        if (_pStepper2)
            _pStepper2->step(direction);
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
        // if (args.xValid && _pXLinearStepper)
        // {
        //     Log.info("Linear to %ld", (long)args.xVal);
        //     _pXLinearStepper->moveTo(args.xVal);
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
        if (_motorsAreEnabled && Utils::isTimeout(millis(), _motorEnLastMillis, _motorDisableSecs * 1000))
        {
            enableMotors(false);
        }
        //
        // // Run the steppers
        // _pRotationStepper->run();
        // _pXLinearStepper->run();

    }
};
