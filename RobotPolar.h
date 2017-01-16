// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "Utils.h"
#include "RobotBase.h"
#include "AccelStepper.h"
#include "ConfigPinMap.h"

class RobotPolar : public RobotBase
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
    AccelStepper* _pRotationStepper;
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

protected:
    bool isBusy()
    {
        bool steppersRunning = false;
        if (_pRotationStepper)
            steppersRunning |= (_pRotationStepper->distanceToGo() != 0);
        if (_pXLinearStepper)
            steppersRunning |= (_pXLinearStepper->distanceToGo() != 0);
        return steppersRunning;
    }

public:
    RobotPolar(const char* pRobotTypeName) :
        RobotBase(pRobotTypeName)
    {
        _motorEnPin = -1;
        _motorEnOnVal = 1;
        _motorDisableSecs = motorDisableSecs_default;
        _pRotationStepper = NULL;
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

    ~RobotPolar()
    {
        delete _pRotationStepper;
        delete _pXLinearStepper;
        if (_motorEnPin != -1)
            pinMode(_motorEnPin, INPUT);
    }

    // Set config
    bool init(const char* robotConfigStr)
    {
        // Free up any previous configuration
        delete _pRotationStepper;
        _pRotationStepper = NULL;
        delete _pXLinearStepper;
        _pXLinearStepper = NULL;
        if (_motorEnPin != -1)
        {
            pinMode(_motorEnPin, INPUT);
        }

        // Info
        Log.info("Constructing RobotPolar from %s", robotConfigStr);

        // Get motor enable info
        String motorEnPinName = ConfigManager::getString("motorEnPin", "-1", robotConfigStr);
        _motorEnOnVal = ConfigManager::getLong("motorEnOnVal", 1, robotConfigStr);
        _motorEnPin = ConfigPinMap::getPinFromName(motorEnPinName.c_str());
        _motorDisableSecs = ConfigManager::getDouble("motorDisableSecs", motorDisableSecs_default, robotConfigStr);
        Log.info("%s MotorEnable (pin %d, onVal %d, %0.2f secs)", _robotTypeName.c_str(), _motorEnPin, _motorEnOnVal, _motorDisableSecs);
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
        Log.info("%s Rotation (step %d, dirn %d)", _robotTypeName.c_str(), _rotateStepPin, _rotateDirnPin);
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
        Log.info("%s Linear (step %d, dirn %d)", _robotTypeName.c_str(), _xLinearStepPin, _xLinearDirnPin);
        if (_xLinearStepPin == -1 || _xLinearDirnPin == -1)
            return false;

        // Stepper Motor enable pin
        pinMode(_motorEnPin, OUTPUT);
        digitalWrite(_motorEnPin, !_motorEnOnVal);
        _motorsAreEnabled = false;

        // Create stepper objetcs
        _pRotationStepper = new AccelStepper(AccelStepper::DRIVER, _rotateStepPin, _rotateDirnPin);
        if (_pRotationStepper)
        {
            _pRotationStepper->setMaxSpeed(_rotateMaxSpeed);
            _pRotationStepper->setAcceleration(_rotateAccel);
        }
        _pXLinearStepper = new AccelStepper(AccelStepper::DRIVER, _xLinearStepPin, _xLinearDirnPin);
        if (_pXLinearStepper)
        {
            _pXLinearStepper->setMaxSpeed(_xLinearMaxSpeed);
            _pXLinearStepper->setAcceleration(_xLinearAccel);
        }

        // Log.info("%s Rot(%d,%d), Lin(%d,%d)", _robotTypeName.c_str(), _rotateStepPin, _rotateDirnPin,
        //             _xLinearStepPin, _xLinearDirnPin);

        return true;
    }

    // Movement commands
    void actuator(double value)
    {
        Log.info("%s actuator %f", _robotTypeName.c_str(), value);
    }

    void moveTo(RobotCommandArgs& args)
    {
        // Check if steppers are running - if so ignore the command
        bool steppersMoving = isBusy();

        // Info
        Log.info("%s moveTo %f(%d),%f(%d),%f(%d) %s", _robotTypeName.c_str(), args.xVal, args.xValid,
                    args.yVal, args.yValid, args.zVal, args.zValid, steppersMoving ? "BUSY" : "");

        // Check if busy
        if (steppersMoving)
            return;

        // Enable motors
        enableMotors(true);

        // Start moving
        if (args.yValid && _pRotationStepper)
        {
            Log.info("Rotation to %ld", (long)args.yVal);
            _pRotationStepper->moveTo((long)args.yVal);
        }
        if (args.xValid && _pXLinearStepper)
        {
            Log.info("Linear to %ld", (long)args.xVal);
            _pXLinearStepper->moveTo(args.xVal);
        }
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

        // Run the steppers
        _pRotationStepper->run();
        _pXLinearStepper->run();
    }
};
