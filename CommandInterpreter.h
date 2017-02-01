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

    bool process(const char* pCmdStr)
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
            rslt = _pWorkflowManager->add(pCmdStr);
        return rslt;
    }
};
