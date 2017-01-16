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

    CommandElem(const char* pCmdStr)
    {
        _cmdStr = pCmdStr;
    }

    String getString()
    {
        return _cmdStr;
    }
};
