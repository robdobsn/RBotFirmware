// RBotFirmware
// Rob Dobson 2016

#pragma once

class MotionHelper;
class RobotCommandArgs;

class RobotBase
{
  protected:
    String _robotTypeName;
    // MotionHelper for the robot motion
    MotionHelper &_motionHelper;

  public:
    RobotBase(const char *pRobotTypeName, MotionHelper &motionHelper);
    virtual ~RobotBase();

    // Pause (or un-pause) all motion
    virtual void pause(bool pauseIt);
    // Check if paused
    virtual bool isPaused();
    // Stop
    virtual void stop();
    virtual bool init(const char *robotConfigStr);
    virtual bool canAcceptCommand();
    virtual void service();
    // Movement commands
    virtual void actuator(double value);
    virtual void moveTo(RobotCommandArgs &args);
    virtual void setMotionParams(RobotCommandArgs &args);
    virtual void getCurStatus(RobotCommandArgs &args);
    virtual void getRobotAttributes(String& robotAttrs);
    // Homing commands
    virtual void goHome(RobotCommandArgs &args);
    virtual void setHome(RobotCommandArgs &args);
    virtual bool wasActiveInLastNSeconds(unsigned int nSeconds);
};
