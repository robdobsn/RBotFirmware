// RBotFirmware
// Rob Dobson 2016

#include "WorkflowManager.h"
#include "CommandInterpreter.h"
#include "CommandExtender.h"

CommandInterpreter::CommandInterpreter(WorkflowManager* pWorkflowManager)
{
    _pWorkflowManager = pWorkflowManager;
    _pCommandExtender = new CommandExtender(this);
}

bool CommandInterpreter::canAcceptCommand()
{
    if (_pWorkflowManager)
        return !_pWorkflowManager->isFull();
    return false;
}

bool CommandInterpreter::processSingle(const char* pCmdStr)
{
    // RWAD TODO check if this is an immediate command

    // Check for extended commands
    bool rslt = false;
    if (_pCommandExtender)
        rslt = _pCommandExtender->procCommand(pCmdStr);

    // Send the line for processing
    if (!rslt && (_pWorkflowManager != NULL))
    {
        if (strlen(pCmdStr) != 0)
            rslt = _pWorkflowManager->add(pCmdStr);
    }
    Log.trace("processSingle rslt %s", rslt ? "OK" : "Fail");

    return rslt;
}

bool CommandInterpreter::process(const char* pCmdStr)
{
    // Handle the case of a single string
    if (strstr(pCmdStr, ";") == NULL)
    {
        Log.trace("cmdProc oneline %s", pCmdStr);
        return processSingle(pCmdStr);
    }

    // Handle multiple commands (tab delimited)
    Log.trace("cmdProc multiline %s", pCmdStr);
    const int MAX_TEMP_CMD_STR_LEN = 1000;
    const char* pCurStr = pCmdStr;
    const char* pCurStrEnd = pCmdStr;
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
            Log.trace("cmdProc single %d %s", stLen, pCurCmd);
            processSingle(pCurCmd);
            delete [] pCurCmd;

            // Move on
            if (*pCurStrEnd == '\0')
                break;
            pCurStr = pCurStrEnd + 1;
        }
        pCurStrEnd++;
    }
}

void CommandInterpreter::service()
{
    _pCommandExtender->service();
}
