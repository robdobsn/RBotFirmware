// RBotFirmware
// Rob Dobson 2016

#ifndef _ROBOT_BASE_H_
#define _ROBOT_BASE_H_

class RobotBase
{
public:
    RobotBase()
    {
    }

    ~RobotBase()
    {
    }

    // Movement commands
    virtual void actuator(double value)
    {

    }

    virtual void moveTo(double xPos, double yPos, double zPos)
    {

    }
};

#endif // _ROBOT_BASE_H_
