// RBotFirmware
// Rob Dobson 2016

#include "WorkflowManager.h"
#include "CommandInterpreter.h"
#include "CommandExtender.h"
#include "RobotController.h"
#include "GCodeInterpreter.h"

CommandInterpreter::CommandInterpreter(WorkflowManager* pWorkflowManager, RobotController* pRobotController)
{
    _pWorkflowManager = pWorkflowManager;
    _pRobotController = pRobotController;
    _pCommandExtender = new CommandExtender(this);
}

void CommandInterpreter::setSequences(const char* configStr)
{
    Log.trace("CmdInterp setSequences %s", configStr);

    // Simply pass the whole config to the extender at present
    _pCommandExtender->setSequences(configStr);
}

const char* CommandInterpreter::getSequences()
{
    return _pCommandExtender->getSequences();
}

void CommandInterpreter::setPatterns(const char* configStr)
{
    Log.trace("CmdInterp setPatterns %s", configStr);

    // Simply pass the whole config to the extender at present
    _pCommandExtender->setPatterns(configStr);
}

const char* CommandInterpreter::getPatterns()
{
    return _pCommandExtender->getPatterns();
}

bool CommandInterpreter::canAcceptCommand()
{
    if (_pWorkflowManager)
        return !_pWorkflowManager->isFull();
    return false;
}

bool CommandInterpreter::queueIsEmpty()
{
    if (_pWorkflowManager)
        return _pWorkflowManager->isEmpty();
    return false;
}

bool CommandInterpreter::processSingle(const char* pCmdStr)
{
    // Check if this is an immediate command
    if (stricmp(pCmdStr, "pause") == 0)
    {
        if (_pRobotController)
            _pRobotController->pause(true);
    }
    else if (stricmp(pCmdStr, "resume") == 0)
    {
        if (_pRobotController)
            _pRobotController->pause(false);
    }

    // Send the line to the workflow manager
    bool rslt = false;
    if (_pWorkflowManager)
    {
        if (strlen(pCmdStr) != 0)
            rslt = _pWorkflowManager->add(pCmdStr);
    }
    Log.trace("CmdInterp procSingle rslt %s", rslt ? "OK" : "Fail");

    return rslt;
}

bool CommandInterpreter::process(const char* pCmdStr, int cmdIdx)
{
    // Handle the case of a single string
    if (strstr(pCmdStr, ";") == NULL)
    {
        return processSingle(pCmdStr);
    }

    // Handle multiple commands (semicolon delimited)
    /*Log.trace("CmdInterp process %s", pCmdStr);*/
    const int MAX_TEMP_CMD_STR_LEN = 1000;
    const char* pCurStr = pCmdStr;
    const char* pCurStrEnd = pCmdStr;
    int curCmdIdx = 0;
    while (true)
    {
        // Find line end
        if ((*pCurStrEnd == ';') || (*pCurStrEnd == '\0'))
        {
            // Extract the line
            int stLen = pCurStrEnd-pCurStr;
            if ((stLen == 0) || (stLen > MAX_TEMP_CMD_STR_LEN))
                break;

            // Alloc
            char* pCurCmd = new char [stLen+1];
            if (!pCurCmd)
                break;
            for (int i = 0; i < stLen; i++)
                pCurCmd[i] = *pCurStr++;
            pCurCmd[stLen] = 0;

            // process
            if (cmdIdx == -1 || cmdIdx == curCmdIdx)
            {
                /*Log.trace("cmdProc single %d %s", stLen, pCurCmd);*/
                processSingle(pCurCmd);
            }
            delete [] pCurCmd;

            // Move on
            curCmdIdx++;
            if (*pCurStrEnd == '\0')
                break;
            pCurStr = pCurStrEnd + 1;
        }
        pCurStrEnd++;
    }
}

void CommandInterpreter::service()
{
    // Pump the workflow here
    // Check if the RobotController can accept more
    if (_pRobotController->canAcceptCommand())
    {
        CommandElem cmdElem;
        bool rslt = _pWorkflowManager->get(cmdElem);
        if (rslt)
        {
            Log.info("CmdInterp getWorkflow rlst=%d (waiting %d), %s", rslt,
                            _pWorkflowManager->numWaiting(),
                            cmdElem.getString().c_str());

            // Check for extended commands
            if (_pCommandExtender)
                rslt = _pCommandExtender->procCommand(cmdElem.getString().c_str());

            // Check for GCode
            if (!rslt)
                GCodeInterpreter::interpretGcode(cmdElem, _pRobotController, true);
        }
    }

    // Service command extender (which pumps the state machines associated with extended commands)
    _pCommandExtender->service();
}
