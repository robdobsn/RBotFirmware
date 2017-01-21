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

protected:
    // MotionController for the polar motion
    MotionController _motionController;

public:
    RobotPolar(const char* pRobotTypeName) :
        RobotBase(pRobotTypeName)
    {
    }

    ~RobotPolar()
    {
    }

    // Set config
    bool init(const char* robotConfigStr)
    {
        // Info
        Log.info("Constructing RobotPolar from %s", robotConfigStr);

        // Init motion controller from config
        _motionController.setOrbitalParams(robotConfigStr);

        return true;
    }

    void service()
    {
        _motionController.service();
    }
};
