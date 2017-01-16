// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "RobotCommandArgs.h"

class RobotBase
{
protected:
    String _robotTypeName;

public:
    RobotBase(const char* pRobotTypeName) :
        _robotTypeName(pRobotTypeName)
    {
    }

    ~RobotBase()
    {
    }

    virtual bool init(const char* robotConfigStr)
    {
        return false;
    }

    virtual bool isBusy()
    {
        return false;
    }

    virtual void service()
    {
    }

    // Movement commands
    virtual void actuator(double value)
    {
    }

    virtual void moveTo(RobotCommandArgs& args)
    {
    }

    // Homing commands
    virtual void home(RobotCommandArgs& args)
    {
    }
};
