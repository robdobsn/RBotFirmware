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

    // Movement commands
    virtual void actuator(double value)
    {

    }

    virtual void moveTo(RobotCommandArgs& args)
    {

    }
};
