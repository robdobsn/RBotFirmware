// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "RobotCommandArgs.h"

class RobotBase
{
public:
    RobotBase()
    {
    }

    ~RobotBase()
    {
    }

    virtual bool init(const char* robotConfigStr)
    {
        return false;
    }

    // Movement commands
    virtual void actuator(double value)
    {

    }

    virtual void moveTo(RobotCommandArgs& args)
    {

    }
};
