// RBotFirmware
// Rob Dobson 2017

#include "PatternEvaluator.h"
#include "PatternGeneratorModSpiral.h"
#include "PatternGeneratorTestPattern.h"

class CommandInterpreter;

class CommandExtender
{
public:
    CommandExtender(CommandInterpreter* pCommandInterpreter)
    {
        _patternGenerators[0] = &_patternGeneratorModSpiral;
        _patternGenerators[1] = &_patternGeneratorTestPattern;
        _numPatternGenerators = 2;
        _pCommandInterpreter = pCommandInterpreter;
    }

    void service()
    {
        // Service pattern generators
        for (int i = 0; i < _numPatternGenerators; i++)
        {
            _patternGenerators[i]->service(_pCommandInterpreter);
        }
        // Service pattern evaluator
        _patternEvaluator.service(_pCommandInterpreter);
    }

    bool procCommand(const char* pCmdStr)
    {
        // See if the command is a pattern generator
        const char* pCmdStart = trimWhitespace(pCmdStr);
        const char* pPatternPos = NULL;
        int patternGenIdx = -1;
        for (int i = 0; i < _numPatternGenerators; i++)
        {
            // See if pattern name is at the start of the string
            pPatternPos = strstr(pCmdStart, _patternGenerators[i]->getPatternName());
            // Log.trace("Pat %s %s %ld %ld", pCmdStart, _patternGenerators[i]->getPatternName(), pPatternPos, pCmdStart);
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

        // See if it is a pattern evaluator
        if (strstr(pCmdStart, "pattern"))
        {
            if (pPatternPos == pCmdStart)
            {
                _patternEvaluator.start();
            }
        }
        return false;
    }

private:
    static const char *trimWhitespace(const char *pStr)
    {
        while(isspace(*pStr))
            pStr++;
        return pStr;
    }

private:
    PatternGeneratorModSpiral _patternGeneratorModSpiral;
    PatternGeneratorTestPattern _patternGeneratorTestPattern;
    PatternGenerator* _patternGenerators[10];
    int _numPatternGenerators;
    CommandInterpreter* _pCommandInterpreter;
    PatternEvaluator _patternEvaluator;
};
