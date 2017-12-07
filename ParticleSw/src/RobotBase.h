// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "RobotCommandArgs.h"
#include "AxisParams.h"

class RobotBase
{
protected:
    String _robotTypeName;

public:
    RobotBase(const char* pRobotTypeName) :
        _robotTypeName(pRobotTypeName)
    {
    }

    virtual ~RobotBase()
    {
    }

    // Pause (or un-pause) all motion
    virtual void pause(bool pauseIt)
    {
    }

    // Check if paused
    virtual bool isPaused()
    {
        return false;
    }

    // Stop
    virtual void stop()
    {
    }

    virtual bool init(const char* robotConfigStr)
    {
        return false;
    }

    virtual bool canAcceptCommand()
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

    virtual void setMotionParams(RobotCommandArgs& args)
    {
    }

    virtual void getCurStatus(RobotCommandArgs& args)
    {
    }

    // Homing commands
    virtual void goHome(RobotCommandArgs& args)
    {
    }

    virtual void setHome(RobotCommandArgs& args)
    {
    }

    virtual bool wasActiveInLastNSeconds(unsigned int nSeconds)
    {
        return false;
    }

};
