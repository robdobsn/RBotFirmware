// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "RobotCommandArgs.h"
#include "AxisParams.h"

class RobotBase
{
protected:
    String _robotTypeName;
    // MotionHelper for the robot motion
    MotionHelper& _motionHelper;

public:
    RobotBase(const char* pRobotTypeName, MotionHelper& motionHelper) :
        _robotTypeName(pRobotTypeName),
        _motionHelper(motionHelper)
    {
    }

    virtual ~RobotBase()
    {
    }

    // Pause (or un-pause) all motion
    virtual void pause(bool pauseIt)
    {
      _motionHelper.pause(pauseIt);
    }

    // Check if paused
    virtual bool isPaused()
    {
      return _motionHelper.isPaused();
    }

    // Stop
    virtual void stop()
    {
      _motionHelper.stop();
    }

    virtual bool init(const char* robotConfigStr)
    {
      // Init motion controller from config
      _motionHelper.configure(robotConfigStr);
      return true;
    }

    virtual bool canAcceptCommand()
    {
      // Check if motionHelper is can accept a command
      return _motionHelper.canAccept();
    }

    virtual void service()
    {
      // Service homing activity
      bool homingActive = homingService();

      // Service the motion controller
      _motionHelper.service(!homingActive);
    }

    // Movement commands
    virtual void actuator(double value)
    {
    }

    virtual void moveTo(RobotCommandArgs& args)
    {
        _motionHelper.moveTo(args);
    }

    virtual void setMotionParams(RobotCommandArgs& args)
    {
        _motionHelper.setMotionParams(args);
    }

    virtual void getCurStatus(RobotCommandArgs& args)
    {
        _motionHelper.getCurStatus(args);
    }

    // Homing commands
    virtual void goHome(RobotCommandArgs& args)
    {
      _motionHelper.goHome(args);
    }

    virtual void setHome(RobotCommandArgs& args)
    {
    }

    virtual bool wasActiveInLastNSeconds(unsigned int nSeconds)
    {
      if (_homingState != HOMING_STATE_IDLE)
          return true;
      return ((unsigned long)Time.now() < _motionHelper.getLastActiveUnixTime() + nSeconds);
    }

};
