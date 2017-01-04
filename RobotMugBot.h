// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "RobotBase.h"

class RobotMugBot : public RobotBase
{
public:
    RobotMugBot(const char* robotConfigStr)
    {
        Log.info("Constructing MugBot from %s", robotConfigStr);
    }

    ~RobotMugBot()
    {
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
