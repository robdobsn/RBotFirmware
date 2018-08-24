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

bool CommandInterpreter::setWifi(const char* pCmdStr)
{
    // TODO ...

    // // Get args
    // const char* pArgsPos = strstr(pCmdStr, " ");
    // if (pArgsPos == 0)
    //     return false;
    // // SSID
    // const char* pSSIDPos = pArgsPos + 1;
    // pArgsPos = strstr(pSSIDPos, " ");
    // if (pArgsPos == 0)
    //     return false;
    // int stLen = pArgsPos - pSSIDPos;
    // char* pSsidStr = new char[stLen + 1];
    // for (int i = 0; i < stLen; i++)
    //     pSsidStr[i] = pSSIDPos[i];
    // pSsidStr[stLen] = 0;
    // // password
    // const char* pPword = pArgsPos + 1;
    // // Set WiFi info
    // WiFi.setCredentials(pSsidStr, pPword);
    // Log.notice("CmdInterp: WiFi SSID %s pwLen %d", pSsidStr, strlen(pPword));
    // delete [] pSsidStr;
    return true;
}

void CommandInterpreter::processSingle(const char* pCmdStr, String& retStr)
{
    const char* okRslt = "{\"rslt\":\"ok\"}";
    const char* failRslt = "{\"rslt\":\"fail\"}";
    retStr = "{\"rslt\":\"none\"}";

    // Check if this is an immediate command
    if (strcasecmp(pCmdStr, "pause") == 0)
    {
        if (_pRobotController)
        {
            _pRobotController->pause(true);
            retStr = okRslt;
        }
    }
    else if (strcasecmp(pCmdStr, "resume") == 0)
    {
        if (_pRobotController)
        {
            _pRobotController->pause(false);
            retStr = okRslt;
        }
    }
    else if (strcasecmp(pCmdStr, "stop") == 0)
    {
        if (_pRobotController)
            _pRobotController->stop();
        if (_pWorkflowManager)
            _pWorkflowManager->clear();
        if (_pCommandExtender)
            _pCommandExtender->stop();
        retStr = okRslt;
    }
    else if (strstr(pCmdStr, "setwifi") == pCmdStr)
    {
        if (setWifi(pCmdStr))
            retStr = okRslt;
        else
            retStr = failRslt;
    }
    else if (strstr(pCmdStr, "clearwifi") == pCmdStr)
    {
    // TODO ...
        // WiFi.clearCredentials();
        Log.notice("CmdInterp: WiFi Credentials Cleared");
        retStr = okRslt;
    }
    // else if (strstr(pCmdStr, "setconfig") == pCmdStr)
    // {
    //     Log.notice("CmdInterp: set config");
    //
    //     retStr = okRslt;
    // }
    else if (_pWorkflowManager)
    {
        // Send the line to the workflow manager
        if (strlen(pCmdStr) != 0)
        {
            bool rslt = _pWorkflowManager->add(pCmdStr);
            if (!rslt)
                retStr = "{\"rslt\":\"busy\"}";
            else
                retStr = okRslt;
        }
    }
    Log.trace("CmdInterp procSingle rslt %s", retStr.c_str());
}

void CommandInterpreter::process(const char* pCmdStr, String& retStr, int cmdIdx)
{
    // Handle the case of a single string
    if (strstr(pCmdStr, ";") == NULL)
    {
        return processSingle(pCmdStr, retStr);
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
                processSingle(pCurCmd, retStr);
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
            Log.trace("CmdInterp getWorkflow rlst=%d (waiting %d), %s", rslt,
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
