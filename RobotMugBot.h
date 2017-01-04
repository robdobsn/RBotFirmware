// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "RobotBase.h"
#include "AccelStepper.h"
#include "ConfigPinMap.h"

class RobotMugBot : public RobotBase
{
private:
    AccelStepper* _pMugRotationStepper;
    int _rotateStepPin, _rotateDirnPin;
    AccelStepper* _pXLinearStepper;
    int _xLinearStepPin, _xLinearDirnPin;
    Servo _penLiftServo;

public:
    RobotMugBot()
    {
        _pMugRotationStepper = NULL;
        _pXLinearStepper = NULL;
        _rotateStepPin = -1;
        _rotateDirnPin = -1;
        _xLinearStepPin = -1;
        _xLinearDirnPin = -1;
    }

    ~RobotMugBot()
    {
        delete _pMugRotationStepper;
        delete _pXLinearStepper;
    }

    // Set config
    bool init(const char* robotConfigStr)
    {
        Log.info("Constructing MugBot from %s", robotConfigStr);

        // Get mug rotation params
        String mugRotationJSON = ConfigManager::getString("mugRotation", "{}", robotConfigStr);
        String rotateStepPinName = ConfigManager::getString("stepPin", "-1", mugRotationJSON);
        String rotateDirnPinName = ConfigManager::getString("dirnPin", "-1", mugRotationJSON);
        _rotateStepPin = ConfigPinMap::getPinFromName(rotateStepPinName.c_str());
        _rotateDirnPin = ConfigPinMap::getPinFromName(rotateDirnPinName.c_str());
        Log.info("MugBot Rot(%d,%d)", _rotateStepPin, _rotateDirnPin);
        if (_rotateStepPin == -1 || _rotateDirnPin == -1)
            return false;

        // Get linear params
        String xLinearJSON = ConfigManager::getString("xLinear", "{}", robotConfigStr);
        String xLinearStepPinName = ConfigManager::getString("stepPin", "-1", xLinearJSON);
        String xLinearDirnPinName = ConfigManager::getString("dirnPin", "-1", xLinearJSON);
        _xLinearStepPin = ConfigPinMap::getPinFromName(xLinearStepPinName.c_str());
        _xLinearDirnPin = ConfigPinMap::getPinFromName(xLinearDirnPinName.c_str());
        Log.info("MugBot Lin(%d,%d)", _xLinearStepPin, _xLinearDirnPin);
        if (_xLinearStepPin == -1 || _xLinearDirnPin == -1)
            return false;

        // Create stepper objetcs
        delete _pMugRotationStepper;
        _pMugRotationStepper = new AccelStepper(AccelStepper::DRIVER, _rotateStepPin, _rotateDirnPin);
        delete _pXLinearStepper;
        _pXLinearStepper = new AccelStepper(AccelStepper::DRIVER, _xLinearStepPin, _xLinearDirnPin);

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
        Log.info("MugBot moveTo %f(%d),%f(%d),%f(%d)", args.xVal, args.xValid,
                    args.yVal, args.yValid, args.zVal, args.zValid);
    }
};
