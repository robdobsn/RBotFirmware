// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "ConfigManager.h"
#include "RobotCommandArgs.h"

// Robot types
#include "RobotMugBot.h"
#include "RobotGeistBot.h"

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
            Log.info("Constructing MugBot");
            _pRobot = new RobotMugBot("MugBot");
            if (!_pRobot)
                return false;
            _pRobot->init(configStr);
        }
        else if (robotType.equalsIgnoreCase("GeistBot"))
        {
            Log.info("Constructing GeistBot");
            _pRobot = new RobotGeistBot("GeistBot");
            if (!_pRobot)
                return false;
            _pRobot->init(configStr);
        }
        else
        {
            Log.info("Cannot determine robotType %s", robotType.c_str());
        }
    }

    // Check busy
    bool isBusy()
    {
        if (!_pRobot)
            return false;
        return _pRobot->isBusy();
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

    void moveTo(RobotCommandArgs& args)
    {
        if (!_pRobot)
            return;
        _pRobot->moveTo(args);
    }

    // Home
    void home(RobotCommandArgs& args)
    {
        if (!_pRobot)
            return;
        _pRobot->home(args);
    }

};
