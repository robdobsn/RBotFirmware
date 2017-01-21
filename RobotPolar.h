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
    static constexpr double axis1MaxSpeed_default = 100.0;
    static constexpr double axis1Accel_default = 100.0;
    static constexpr double axis2MaxSpeed_default = 100.0;
    static constexpr double axis2Accel_default = 100.0;
    double _axis1MaxSpeed, _axis1Accel;
    double _axis2MaxSpeed, _axis2Accel;
    Servo _penLiftServo;

protected:
    // MotionController for the polar motion
    MotionController _motionController;

public:
    RobotPolar(const char* pRobotTypeName) :
        RobotBase(pRobotTypeName)
    {
        _axis1MaxSpeed = -1;
        _axis1Accel = -1;
        _axis2MaxSpeed = -1;
        _axis2Accel = -1;
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
        double motorDisableSecs = ConfigManager::getDouble("motorDisableSecs", motorDisableSecs_default, robotConfigStr);
        Log.info("%s MotorEnable (pin %d, onVal %d, %0.2f secs)", _robotTypeName.c_str(), motorEnPin, motorEnOnVal, motorDisableSecs);
        if (motorEnPin == -1)
            return false;

        // Get rotation params
        String axis1MotorJSON = ConfigManager::getString("axis1Motor", "{}", robotConfigStr);
        String axis1StepPinName = ConfigManager::getString("stepPin", "-1", axis1MotorJSON);
        String axis1DirnPinName = ConfigManager::getString("dirnPin", "-1", axis1MotorJSON);
        _axis1MaxSpeed = ConfigManager::getDouble("maxSpeed", axis1MaxSpeed_default, axis1MotorJSON);
        _axis1Accel = ConfigManager::getDouble("accel", axis1Accel_default, axis1MotorJSON);
        long axis1StepPin = ConfigPinMap::getPinFromName(axis1StepPinName.c_str());
        long axis1DirnPin = ConfigPinMap::getPinFromName(axis1DirnPinName.c_str());
        Log.info("%s Rotation (step %d, dirn %d)", _robotTypeName.c_str(), axis1StepPin, axis1DirnPin);
        if (axis1StepPin == -1 || axis1DirnPin == -1)
            return false;

        // Get rotation end stop - can operate without a rotation end stop
        String axis1EndStop1JSON = ConfigManager::getString("axis1EndStop1", "{}", robotConfigStr);
        String axis1EndStop1PinName = ConfigManager::getString("sensePin", "-1", axis1EndStop1JSON);
        long axis1EndStop1ActiveLevel = ConfigManager::getLong("activeLevel", 1, axis1EndStop1JSON);
        String axis1EndStop1InputTypeStr = ConfigManager::getString("inputType", "", axis1EndStop1JSON);
        long axis1EndStop1Pin = ConfigPinMap::getPinFromName(axis1EndStop1PinName.c_str());
        int axis1EndStop1InputType = ConfigPinMap::getInputType(axis1EndStop1InputTypeStr.c_str());
        Log.info("%s Rotation EndStop 1 (sense %d, level %d, type %d)", _robotTypeName.c_str(), axis1EndStop1Pin,
                    axis1EndStop1ActiveLevel, axis1EndStop1InputType);

        // Get axis2 params
        String axis2MotorJSON = ConfigManager::getString("axis2Motor", "{}", robotConfigStr);
        String axis2StepPinName = ConfigManager::getString("stepPin", "-1", axis2MotorJSON);
        String axis2DirnPinName = ConfigManager::getString("dirnPin", "-1", axis2MotorJSON);
        _axis2MaxSpeed = ConfigManager::getDouble("maxSpeed", axis2MaxSpeed_default, axis2MotorJSON);
        _axis2Accel = ConfigManager::getDouble("accel", axis2Accel_default, axis2MotorJSON);
        long axis2StepPin = ConfigPinMap::getPinFromName(axis2StepPinName.c_str());
        long axis2DirnPin = ConfigPinMap::getPinFromName(axis2DirnPinName.c_str());
        Log.info("%s axis2 (step %d, dirn %d)", _robotTypeName.c_str(), axis2StepPin, axis2DirnPin);
        if (axis2StepPin == -1 || axis2DirnPin == -1)
            return false;

        // Get axis2 end stop - can't operate without a axis2 end stop
        String axis2EndStop1JSON = ConfigManager::getString("axis2EndStop1", "{}", robotConfigStr);
        String axis2EndStop1PinName = ConfigManager::getString("sensePin", "-1", axis2EndStop1JSON);
        long axis2EndStop1ActiveLevel = ConfigManager::getLong("activeLevel", 1, axis2EndStop1JSON);
        String axis2EndStop1InputTypeStr = ConfigManager::getString("inputType", "", axis2EndStop1JSON);
        long axis2EndStop1Pin = ConfigPinMap::getPinFromName(axis2EndStop1PinName.c_str());
        int axis2EndStop1InputType = ConfigPinMap::getInputType(axis2EndStop1InputTypeStr.c_str());
        Log.info("%s axis2 EndStop 1 (sense %d, level %d, type %d)", _robotTypeName.c_str(), axis2EndStop1Pin,
                    axis2EndStop1ActiveLevel, axis2EndStop1InputType);

        // Init motion controller
        _motionController.setOrbitalParams(axis1StepPin, axis1DirnPin,
                    axis2StepPin, axis2DirnPin,
                    motorEnPin, motorEnOnVal, motorDisableSecs,
                    axis1EndStop1Pin, axis1EndStop1ActiveLevel, axis1EndStop1InputType,
                    axis2EndStop1Pin, axis2EndStop1ActiveLevel, axis2EndStop1InputType);

        // if (_pRotationStepper)
        // {
        //     _pRotationStepper->setMaxSpeed(_axis1MaxSpeed);
        //     _pRotationStepper->setAcceleration(_axis1Accel);
        // }
        // if (_paxis2Stepper)
        // {
        //     _paxis2Stepper->setMaxSpeed(_axis2MaxSpeed);
        //     _paxis2Stepper->setAcceleration(_axis2Accel);
        // }

        // Log.info("%s Rot(%d,%d), Lin(%d,%d)", _robotTypeName.c_str(), _axis1StepPin, _axis1DirnPin,
        //             _axis2StepPin, _axis2DirnPin);

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
