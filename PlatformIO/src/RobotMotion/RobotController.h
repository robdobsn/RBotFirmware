// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "MotionControl/MotionHelper.h"

class RobotBase;
class RobotCommandArgs;

class RobotController
{
private:
    RobotBase* _pRobot;
    MotionHelper _motionHelper;

public:
    RobotController();
    ~RobotController();
    bool init(const char* configStr);

    // Pause (or un-pause) all motion
    void pause(bool pauseIt);

    // Stop
    void stop();

    // Check if paused
    bool isPaused();

    // Service (called frequently)
    void service();

    // Movement commands
    void actuator(double value);

    // Check if the robot can accept a (motion) command
    bool canAcceptCommand();

    void moveTo(RobotCommandArgs& args);

    // Set motion parameters
    void setMotionParams(RobotCommandArgs& args);

    // Get status
    void getCurStatus(RobotCommandArgs& args);

    // Get robot attributes
    void getRobotAttributes(String& robotAttrs);

    // Go Home
    void goHome(RobotCommandArgs& args);

    // Set Home
    void setHome(RobotCommandArgs& args);

    bool wasActiveInLastNSeconds(int nSeconds);

    String getDebugStr();
};
