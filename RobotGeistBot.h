// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "Utils.h"
#include "RobotPolar.h"

class RobotGeistBot : public RobotPolar
{
public:
    RobotGeistBot(const char* pRobotTypeName) :
        RobotPolar(pRobotTypeName)
    {

    }
};
