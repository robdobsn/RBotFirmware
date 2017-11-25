// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "ConfigManager.h"
#include "RobotCommandArgs.h"

// Robot types
#include "RobotMugBot.h"
#include "RobotGeistBot.h"
#include "RobotSandTableScara.h"

class RobotController
{
private:
    RobotBase* _pRobot;

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
        String robotType = ConfigManager::getString("robotType", "NONE", configStr);
        if (robotType.equalsIgnoreCase("MugBot"))
        {
            Log.info("Constructing %s", robotType.c_str());
            _pRobot = new RobotMugBot(robotType.c_str());
            if (!_pRobot)
                return false;
            _pRobot->init(configStr);
        }
        else if (robotType.equalsIgnoreCase("GeistBot"))
        {
            Log.info("Constructing %s", robotType.c_str());
            _pRobot = new RobotGeistBot(robotType.c_str());
            if (!_pRobot)
                return false;
            _pRobot->init(configStr);
        }
        else if (robotType.equalsIgnoreCase("SandTableScara"))
        {
            Log.info("Constructing %s", robotType.c_str());
            _pRobot = new RobotSandTableScara(robotType.c_str());
            if (!_pRobot)
                return false;
            _pRobot->init(configStr);
        }
        else
        {
            Log.info("Cannot determine robotType %s", robotType.c_str());
        }
        return true;
    }

    // Pause (or un-pause) all motion
    void pause(bool pauseIt)
    {
        if (pauseIt)
            Log.info("RobotController: pausing");
        else
            Log.info("RobotController: resuming");
        if (!_pRobot)
            return;
        _pRobot->pause(pauseIt);
    }

    // Stop
    void stop()
    {
        Log.info("RobotController: stop");
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
//        Log.trace("RobotController moveTo");
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

    // Home
    void home(RobotCommandArgs& args)
    {
        if (!_pRobot)
            return;
        _pRobot->home(args);
    }

    bool wasActiveInLastNSeconds(int nSeconds)
    {
        if (!_pRobot)
            return false;
        return _pRobot->wasActiveInLastNSeconds(nSeconds);
    }

};
