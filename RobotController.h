// RBotFirmware
// Rob Dobson 2016

#ifndef _ROBOT_CONTROLLER_H_
#define _ROBOT_CONTROLLER_H_

#include "ConfigManager.h"

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
            _pRobot = new RobotMugBot(configStr);
        }
        else
        {
            Log.info("Cannot determine robotType %s", robotType.c_str());
        }
    }

    ~RobotController()
    {
        delete _pRobot;
    }

    // Movement commands
    void actuator(double value)
    {
        if (!_pRobot)
            return;
        _pRobot->actuator(value);
    }

    void moveTo(double xPos, double yPos, double zPos)
    {
        if (!_pRobot)
            return;
        _pRobot->moveTo(xPos, yPos, zPos);
    }
};

#endif // _ROBOT_CONTROLLER_H_
