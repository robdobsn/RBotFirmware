// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "ConfigManager.h"
#include "CommandElem.h"
#include "RobotController.h"

class CommandElem
{
private:
    String _cmdStr;

public:
    CommandElem()
    {
        _cmdStr = "";
    }
    
    CommandElem(String& cmdStr)
    {
        _cmdStr = cmdStr;
    }

    String getString()
    {
        return _cmdStr;
    }
};
