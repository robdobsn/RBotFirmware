// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "RdJson.h"
#include "RobotCommandArgs.h"
#include "MotionHelper.h"

// Robot types
#include "RobotMugBot.h"
#include "RobotGeistBot.h"
#include "RobotHockeyBot.h"
#include "RobotSandTableScara.h"
#include "RobotXYBot.h"

class RobotController
{
private:
    RobotBase* _pRobot;
    MotionHelper _motionHelper;

public:
    RobotController()
    {
        // Init
        _pRobot = NULL;
    }

    ~RobotController()
    {
        delete _pRobot;
    }

    bool init(const char* configStr)
    {
        // Init
        delete _pRobot;
        _pRobot = NULL;

        // Get the robot type from the config
        String robotType = RdJson::getString("robotType", "NONE", configStr);
        if (robotType.equalsIgnoreCase("MugBot"))
        {
            Log.notice("Constructing %s", robotType.c_str());
            _pRobot = new RobotMugBot(robotType.c_str(), _motionHelper);
            if (!_pRobot)
                return false;
            _pRobot->init(configStr);
        }
        else if (robotType.equalsIgnoreCase("GeistBot"))
        {
            Log.notice("Constructing %s", robotType.c_str());
            _pRobot = new RobotGeistBot(robotType.c_str(), _motionHelper);
            if (!_pRobot)
                return false;
            _pRobot->init(configStr);
        }
        else if (robotType.equalsIgnoreCase("SandTableScara"))
        {
            Log.notice("Constructing %s", robotType.c_str());
            _pRobot = new RobotSandTableScara(robotType.c_str(), _motionHelper);
            if (!_pRobot)
                return false;
            _pRobot->init(configStr);
        }
        else if (robotType.equalsIgnoreCase("HockeyBot"))
        {
            Log.notice("Constructing %s", robotType.c_str());
            _pRobot = new RobotHockeyBot(robotType.c_str(), _motionHelper);
            if (!_pRobot)
                return false;
            _pRobot->init(configStr);
        }
        else if (robotType.equalsIgnoreCase("XYBot"))
        {
            Log.notice("Constructing %s", robotType.c_str());
            _pRobot = new RobotXYBot(robotType.c_str(), _motionHelper);
            if (!_pRobot)
                return false;
            _pRobot->init(configStr);
        }
        else
        {
            Log.notice("Cannot determine robotType %s", robotType.c_str());
        }

        if (_pRobot)
        {
            _motionHelper.setTestMode(TEST_MOTION_ACTUATOR_CONFIG);
            _pRobot->pause(false);
        }

        return true;
    }

    // Pause (or un-pause) all motion
    void pause(bool pauseIt)
    {
        if (pauseIt)
            Log.notice("RobotController: pausing");
        else
            Log.notice("RobotController: resuming");
        if (!_pRobot)
            return;
        _pRobot->pause(pauseIt);
    }

    // Stop
    void stop()
    {
        Log.notice("RobotController: stop");
        if (!_pRobot)
            return;
        _pRobot->stop();
    }

    // Check if paused
    bool isPaused()
    {
        if (!_pRobot)
            return false;
        return _pRobot->isPaused();
    }

    // Service (called frequently)
    void service()
    {
        if (!_pRobot)
            return;
        _pRobot->service();
    }

    // Movement commands
    void actuator(double value)
    {
        if (!_pRobot)
            return;
        _pRobot->actuator(value);
    }

    // Check if the robot can accept a (motion) command
    bool canAcceptCommand()
    {
        if (!_pRobot)
            return false;
        return _pRobot->canAcceptCommand();
    }

    void moveTo(RobotCommandArgs& args)
    {
        Log.trace("RobotController moveTo");
        if (!_pRobot)
            return;
        _pRobot->moveTo(args);
    }

    // Set motion parameters
    void setMotionParams(RobotCommandArgs& args)
    {
        if (!_pRobot)
            return;
        _pRobot->setMotionParams(args);
    }

    // Get status
    void getCurStatus(RobotCommandArgs& args)
    {
        if (!_pRobot)
            return;
        _pRobot->getCurStatus(args);
    }

    // Go Home
    void goHome(RobotCommandArgs& args)
    {
        if (!_pRobot)
            return;
        _pRobot->goHome(args);
    }

    // Set Home
    void setHome(RobotCommandArgs& args)
    {
        if (!_pRobot)
            return;
        _pRobot->setHome(args);
    }

    bool wasActiveInLastNSeconds(int nSeconds)
    {
        if (!_pRobot)
            return false;
        return _pRobot->wasActiveInLastNSeconds(nSeconds);
    }

    String getDebugStr()
    {
      return _motionHelper.getDebugStr();
    }

};
