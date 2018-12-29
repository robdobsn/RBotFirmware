// RBotFirmware
// Rob Dobson 2016-18

#include <Arduino.h>
#include "RobotBase.h"
#include "../MotionControl/MotionHelper.h"
// #include <Time.h>
// #include "RobotCommandArgs.h"
// #include "../AxisParams.h"

RobotBase::RobotBase(const char* pRobotTypeName, MotionHelper& motionHelper) :
            _robotTypeName(pRobotTypeName), _motionHelper(motionHelper)
{
}

RobotBase::~RobotBase()
{
}

// Pause (or un-pause) all motion
void RobotBase::pause(bool pauseIt)
{
    _motionHelper.pause(pauseIt);
}

// Check if paused
bool RobotBase::isPaused()
{
    return _motionHelper.isPaused();
}

// Stop
void RobotBase::stop()
{
    _motionHelper.stop();
}

bool RobotBase::init(const char *robotConfigStr)
{
    // Init motion controller from config
    _motionHelper.configure(robotConfigStr);
    return true;
}

bool RobotBase::canAcceptCommand()
{
    // Check if motionHelper is can accept a command
    return _motionHelper.canAccept();
}

void RobotBase::service()
{
    // Service the motion controller
    _motionHelper.service();
}

// Movement commands
void RobotBase::actuator(double value)
{
}

void RobotBase::moveTo(RobotCommandArgs &args)
{
    _motionHelper.moveTo(args);
}

void RobotBase::setMotionParams(RobotCommandArgs &args)
{
    _motionHelper.setMotionParams(args);
}

void RobotBase::getCurStatus(RobotCommandArgs &args)
{
    _motionHelper.getCurStatus(args);
}

void RobotBase::getRobotAttributes(String& robotAttrs)
{
    _motionHelper.getRobotAttributes(robotAttrs);
}

// Homing commands
void RobotBase::goHome(RobotCommandArgs &args)
{
    _motionHelper.goHome(args);
}

void RobotBase::setHome(RobotCommandArgs &args)
{
}

bool RobotBase::wasActiveInLastNSeconds(unsigned int nSeconds)
{
    time_t timeNow;
    time(&timeNow);
    return (timeNow < _motionHelper.getLastActiveUnixTime() + nSeconds);
}
