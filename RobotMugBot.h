// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "Utils.h"
#include "RobotBase.h"
#include "AccelStepper.h"
#include "ConfigPinMap.h"

class RobotMugBot : public RobotBase
{
private:
    // Consts
    static constexpr double motorDisableSecs_default = 60.0;
    static constexpr double rotateMaxSpeed_default = 100.0;
    static constexpr double rotateAccel_default = 100.0;
    static constexpr double xLinearMaxSpeed_default = 100.0;
    static constexpr double xLinearAccel_default = 100.0;
    // Motor enable
    int _motorEnPin;
    int _motorEnOnVal;
    double _motorDisableSecs;
    // Motors
    AccelStepper* _pMugRotationStepper;
    int _rotateStepPin, _rotateDirnPin;
    double _rotateMaxSpeed, _rotateAccel;
    AccelStepper* _pXLinearStepper;
    int _xLinearStepPin, _xLinearDirnPin;
    double _xLinearMaxSpeed, _xLinearAccel;
    Servo _penLiftServo;

    // Motor disable timing
    bool _motorsAreEnabled;
    unsigned long _motorEnLastMillis;

private:
    void enableMotors(bool en)
    {
        if (en)
        {
            if (_motorEnPin != -1)
                digitalWrite(_motorEnPin, _motorEnOnVal);
            _motorsAreEnabled = true;
            _motorEnLastMillis = millis();
        }
        else
        {
            if (_motorEnPin != -1)
                digitalWrite(_motorEnPin, !_motorEnOnVal);
            _motorsAreEnabled = false;
        }
    }

public:
    RobotMugBot()
    {
        _motorEnPin = -1;
        _motorEnOnVal = 1;
        _motorDisableSecs = motorDisableSecs_default;
        _pMugRotationStepper = NULL;
        _pXLinearStepper = NULL;
        _rotateStepPin = -1;
        _rotateDirnPin = -1;
        _rotateMaxSpeed = -1;
        _rotateAccel = -1;
        _xLinearStepPin = -1;
        _xLinearDirnPin = -1;
        _xLinearMaxSpeed = -1;
        _xLinearAccel = -1;
        _motorsAreEnabled = false;
        _motorEnLastMillis = 0;
    }

    ~RobotMugBot()
    {
        delete _pMugRotationStepper;
        delete _pXLinearStepper;
        if (_motorEnPin != -1)
            pinMode(_motorEnPin, INPUT);
    }

    // Set config
    bool init(const char* robotConfigStr)
    {
        // Free up any previous configuration
        delete _pMugRotationStepper;
        _pMugRotationStepper = NULL;
        delete _pXLinearStepper;
        _pXLinearStepper = NULL;
        if (_motorEnPin != -1)
        {
            pinMode(_motorEnPin, INPUT);
        }

        // Info
        Log.info("Constructing MugBot from %s", robotConfigStr);

        // Get motor enable info
        String motorEnPinName = ConfigManager::getString("motorEnPin", "-1", robotConfigStr);
        _motorEnOnVal = ConfigManager::getLong("motorEnOnVal", 1, robotConfigStr);
        _motorEnPin = ConfigPinMap::getPinFromName(motorEnPinName.c_str());
        _motorDisableSecs = ConfigManager::getDouble("motorDisableSecs", motorDisableSecs_default, robotConfigStr);
        Log.info("MugBot MotorEnable (pin %d, onVal %d, %0.2f secs)", _motorEnPin, _motorEnOnVal, _motorDisableSecs);
        if (_motorEnPin == -1)
            return false;

        // Get mug rotation params
        String mugRotationJSON = ConfigManager::getString("mugRotation", "{}", robotConfigStr);
        String rotateStepPinName = ConfigManager::getString("stepPin", "-1", mugRotationJSON);
        String rotateDirnPinName = ConfigManager::getString("dirnPin", "-1", mugRotationJSON);
        _rotateMaxSpeed = ConfigManager::getDouble("maxSpeed", rotateMaxSpeed_default, mugRotationJSON);
        _rotateAccel = ConfigManager::getDouble("accel", rotateAccel_default, mugRotationJSON);
        _rotateStepPin = ConfigPinMap::getPinFromName(rotateStepPinName.c_str());
        _rotateDirnPin = ConfigPinMap::getPinFromName(rotateDirnPinName.c_str());
        Log.info("MugBot Rotation (step %d, dirn %d)", _rotateStepPin, _rotateDirnPin);
        if (_rotateStepPin == -1 || _rotateDirnPin == -1)
            return false;

        // Get linear params
        String xLinearJSON = ConfigManager::getString("xLinear", "{}", robotConfigStr);
        String xLinearStepPinName = ConfigManager::getString("stepPin", "-1", xLinearJSON);
        String xLinearDirnPinName = ConfigManager::getString("dirnPin", "-1", xLinearJSON);
        _xLinearMaxSpeed = ConfigManager::getDouble("maxSpeed", xLinearMaxSpeed_default, xLinearJSON);
        _xLinearAccel = ConfigManager::getDouble("accel", xLinearAccel_default, xLinearJSON);
        _xLinearStepPin = ConfigPinMap::getPinFromName(xLinearStepPinName.c_str());
        _xLinearDirnPin = ConfigPinMap::getPinFromName(xLinearDirnPinName.c_str());
        Log.info("MugBot Linear (step %d, dirn %d)", _xLinearStepPin, _xLinearDirnPin);
        if (_xLinearStepPin == -1 || _xLinearDirnPin == -1)
            return false;

        // Stepper Motor enable pin
        pinMode(_motorEnPin, OUTPUT);
        digitalWrite(_motorEnPin, !_motorEnOnVal);
        _motorsAreEnabled = false;

        // Create stepper objetcs
        _pMugRotationStepper = new AccelStepper(AccelStepper::DRIVER, _rotateStepPin, _rotateDirnPin);
        if (_pMugRotationStepper)
        {
            _pMugRotationStepper->setMaxSpeed(_rotateMaxSpeed);
            _pMugRotationStepper->setAcceleration(_rotateAccel);
        }
        _pXLinearStepper = new AccelStepper(AccelStepper::DRIVER, _xLinearStepPin, _xLinearDirnPin);
        if (_pXLinearStepper)
        {
            _pXLinearStepper->setMaxSpeed(_xLinearMaxSpeed);
            _pXLinearStepper->setAcceleration(_xLinearAccel);
        }

        // Log.info("MugBot Rot(%d,%d), Lin(%d,%d)", _rotateStepPin, _rotateDirnPin,
        //             _xLinearStepPin, _xLinearDirnPin);

        return true;
    }

    // Movement commands
    void actuator(double value)
    {
        Log.info("MugBot actuator %f", value);
    }

    void moveTo(RobotCommandArgs& args)
    {
        // Info
        Log.info("MugBot moveTo %f(%d),%f(%d),%f(%d)", args.xVal, args.xValid,
                    args.yVal, args.yValid, args.zVal, args.zValid);

        // Enable motors
        enableMotors(true);

        // Start moving
        if (args.yValid && _pMugRotationStepper)
        {
            Log.info("Rotation to %ld", (long)args.yVal);
            _pMugRotationStepper->moveTo((long)args.yVal);
        }
        if (args.xValid && _pXLinearStepper)
        {
            Log.info("Linear to %ld", (long)args.xVal);
            _pXLinearStepper->moveTo(args.xVal);
        }
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

        // Run the steppers
        _pMugRotationStepper->run();
        _pXLinearStepper->run();
    }
};
