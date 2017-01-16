// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "ConfigManager.h"
#include "CommandElem.h"
#include "RobotController.h"

// Command interpreter
class CommandInterpreter
{
private:
    WorkflowManager* _pWorkflowManager;

public:
    CommandInterpreter(WorkflowManager* pWorkflowManager)
    {
        _pWorkflowManager = pWorkflowManager;
    }

    bool process(const char* pCmdStr)
    {
        // RWAD TODO check if this is an immediate command

        // Send the line for processing
        bool rslt = false;
        if (_pWorkflowManager)
            rslt = _pWorkflowManager->add(pCmdStr);
        return rslt;
    }
};
