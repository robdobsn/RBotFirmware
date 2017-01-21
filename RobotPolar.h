// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "Utils.h"
#include "RobotBase.h"
#include "ConfigPinMap.h"
#include "MotionController.h"

class RobotPolar : public RobotBase
{
private:
    // Consts
    static constexpr double motorDisableSecs_default = 60.0;
    static constexpr double rotateMaxSpeed_default = 100.0;
    static constexpr double rotateAccel_default = 100.0;
    static constexpr double xLinearMaxSpeed_default = 100.0;
    static constexpr double xLinearAccel_default = 100.0;
    double _rotateMaxSpeed, _rotateAccel;
    double _xLinearMaxSpeed, _xLinearAccel;
    Servo _penLiftServo;
    // MotionController for the polar motion
    MotionController _motionController;

public:
    RobotPolar(const char* pRobotTypeName) :
        RobotBase(pRobotTypeName)
    {
        _rotateMaxSpeed = -1;
        _rotateAccel = -1;
        _xLinearMaxSpeed = -1;
        _xLinearAccel = -1;
    }

    ~RobotPolar()
    {
    }

    // Set config
    bool init(const char* robotConfigStr)
    {
        // Info
        Log.info("Constructing RobotPolar from %s", robotConfigStr);

        // Get motor enable info
        String motorEnPinName = ConfigManager::getString("motorEnPin", "-1", robotConfigStr);
        long motorEnOnVal = ConfigManager::getLong("motorEnOnVal", 1, robotConfigStr);
        long motorEnPin = ConfigPinMap::getPinFromName(motorEnPinName.c_str());
        long motorDisableSecs = ConfigManager::getDouble("motorDisableSecs", motorDisableSecs_default, robotConfigStr);
        Log.info("%s MotorEnable (pin %d, onVal %d, %0.2f secs)", _robotTypeName.c_str(), motorEnPin, motorEnOnVal, motorDisableSecs);
        if (motorEnPin == -1)
            return false;

        // Get mug rotation params
        String mugRotationJSON = ConfigManager::getString("mugRotation", "{}", robotConfigStr);
        String rotateStepPinName = ConfigManager::getString("stepPin", "-1", mugRotationJSON);
        String rotateDirnPinName = ConfigManager::getString("dirnPin", "-1", mugRotationJSON);
        _rotateMaxSpeed = ConfigManager::getDouble("maxSpeed", rotateMaxSpeed_default, mugRotationJSON);
        _rotateAccel = ConfigManager::getDouble("accel", rotateAccel_default, mugRotationJSON);
        long rotateStepPin = ConfigPinMap::getPinFromName(rotateStepPinName.c_str());
        long rotateDirnPin = ConfigPinMap::getPinFromName(rotateDirnPinName.c_str());
        Log.info("%s Rotation (step %d, dirn %d)", _robotTypeName.c_str(), rotateStepPin, rotateDirnPin);
        if (rotateStepPin == -1 || rotateDirnPin == -1)
            return false;

        // Get linear params
        String xLinearJSON = ConfigManager::getString("xLinear", "{}", robotConfigStr);
        String xLinearStepPinName = ConfigManager::getString("stepPin", "-1", xLinearJSON);
        String xLinearDirnPinName = ConfigManager::getString("dirnPin", "-1", xLinearJSON);
        _xLinearMaxSpeed = ConfigManager::getDouble("maxSpeed", xLinearMaxSpeed_default, xLinearJSON);
        _xLinearAccel = ConfigManager::getDouble("accel", xLinearAccel_default, xLinearJSON);
        long xLinearStepPin = ConfigPinMap::getPinFromName(xLinearStepPinName.c_str());
        long xLinearDirnPin = ConfigPinMap::getPinFromName(xLinearDirnPinName.c_str());
        Log.info("%s Linear (step %d, dirn %d)", _robotTypeName.c_str(), xLinearStepPin, xLinearDirnPin);
        if (xLinearStepPin == -1 || xLinearDirnPin == -1)
            return false;

        // Init motion controller
        _motionController.setOrbitalParams(rotateStepPin, rotateDirnPin,
                    xLinearStepPin, xLinearDirnPin,
                    motorEnPin, motorEnOnVal);

        // if (_pRotationStepper)
        // {
        //     _pRotationStepper->setMaxSpeed(_rotateMaxSpeed);
        //     _pRotationStepper->setAcceleration(_rotateAccel);
        // }
        // if (_pXLinearStepper)
        // {
        //     _pXLinearStepper->setMaxSpeed(_xLinearMaxSpeed);
        //     _pXLinearStepper->setAcceleration(_xLinearAccel);
        // }

        // Log.info("%s Rot(%d,%d), Lin(%d,%d)", _robotTypeName.c_str(), _rotateStepPin, _rotateDirnPin,
        //             _xLinearStepPin, _xLinearDirnPin);

        return true;
    }

    // Movement commands
    void actuator(double value)
    {
        Log.info("%s actuator %f", _robotTypeName.c_str(), value);
    }

    void service()
    {
        _motionController.service();
    }
};
