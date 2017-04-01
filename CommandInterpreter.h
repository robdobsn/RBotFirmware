// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "ConfigManager.h"
#include "CommandElem.h"
#include "RobotController.h"
#include "PatternGenerator.h"

// Command interpreter
class CommandInterpreter
{
private:
    WorkflowManager* _pWorkflowManager;
    PatternGenerator** _patternGenerators;
    int _numPatternGenerators;

public:
    CommandInterpreter(WorkflowManager* pWorkflowManager, PatternGenerator** patternGenerators,
                    int numPatternGenerators)
    {
        _pWorkflowManager = pWorkflowManager;
        _patternGenerators = patternGenerators;
        _numPatternGenerators = numPatternGenerators;
    }

    static const char *trimWhitespace(const char *pStr)
    {
        while(isspace(*pStr))
            pStr++;
        return pStr;
    }

    bool canAcceptCommand()
    {
        if (_pWorkflowManager)
            return !_pWorkflowManager->isFull();
        return false;
    }

    bool processSingle(const char* pCmdStr)
    {
        // RWAD TODO check if this is an immediate command

        // Check for PatternGenerators commands
        const char* pCmdStart = trimWhitespace(pCmdStr);
        const char* pPatternPos = NULL;
        int patternGenIdx = -1;
        for (int i = 0; i < _numPatternGenerators; i++)
        {
            // See if pattern name is at the start of the string
            pPatternPos = strstr(pCmdStart, _patternGenerators[i]->getPatternName());
            // Serial.printlnf("Pat %s %s %ld %ld", pCmdStart, _patternGenerators[i]->getPatternName(), pPatternPos, pCmdStart);
            if (pPatternPos == pCmdStart)
            {
                patternGenIdx = i;
                break;
            }
        }
        if (patternGenIdx != -1)
        {
            // Start the pattern generator
            _patternGenerators[patternGenIdx]->start();
            return true;
        }

        // Send the line for processing
        bool rslt = false;
        if (_pWorkflowManager)
        {
            if (strlen(pCmdStart) != 0)
                rslt = _pWorkflowManager->add(pCmdStart);
        }
        Log.trace("processSingle rslt %s", rslt ? "OK" : "Fail");

        return rslt;
    }

    bool process(const char* pCmdStr)
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
};
