// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "ConfigManager.h"
#include "RobotCommandArgs.h"

// Robot types
#include "RobotMugBot.h"

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
            _pRobot = new RobotMugBot();
            if (!_pRobot)
                return false;
            _pRobot->init(configStr);
        }
        else
        {
            Log.info("Cannot determine robotType %s", robotType.c_str());
        }
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
};
