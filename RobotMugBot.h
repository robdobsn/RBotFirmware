// RBotFirmware
// Rob Dobson 2016

#ifndef _ROBOT_MUGBOT_H_
#define _ROBOT_MUGBOT_H_

#include "application.h"
#include "RobotBase.h"

class RobotMugBot : public RobotBase
{
public:
    RobotMugBot(const char* robotConfigStr)
    {
        Log.info("Constructing MugBot from %s", robotConfigStr);
    }

    ~RobotMugBot()
    {
    }

    // Movement commands
    void actuator(double value)
    {
        Log.info("MugBot actuator %f", value);
    }

    void moveTo(double xPos, double yPos, double zPos)
    {
        Log.info("MugBot moveTo %f,%f,%f", xPos, yPos, zPos);
    }
};

#endif // _ROBOT_MUGBOT_H_
