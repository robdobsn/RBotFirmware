// RBotFirmware
// Rob Dobson 2017

#include "PatternEvaluator.h"
#include "CommandSequencer.h"
#include "FileSerializer.h"

class CommandInterface;

class CommandExtender
{
public:
    CommandExtender(FileManager& fileManager) : _fileSerializer(fileManager)
    {
        _pCommandInterface = NULL;
    }

    void setup(CommandInterface* pCommandInterface)
    {
        _pCommandInterface = pCommandInterface;
    }

    void setSequences(const char* configStr)
    {
        _commandSequencer.setConfig(configStr);
    }

    const char* getSequences()
    {
        return _commandSequencer.getConfig();
    }

    void setPatterns(const char* configStr)
    {
        _patternEvaluator.setConfig(configStr);
    }

    const char* getPatterns()
    {
        return _patternEvaluator.getConfig();
    }

    void stop()
    {
        _patternEvaluator.stop();
        _commandSequencer.stop();
    }

    void service()
    {
        if (!_pCommandInterface)
            return;
        // Service pattern evaluator
        _patternEvaluator.service(_pCommandInterface);
        // Service command sequencer
        _commandSequencer.service(_pCommandInterface);
        // Service file serializer
        _fileSerializer.service(_pCommandInterface);
    }

    bool procCommand(const char* pCmdStr)
    {
        // See if the command is a pattern generator
        bool handledOk = false;
        // See if it is a pattern evaluator
        handledOk = _patternEvaluator.procCommand(pCmdStr);
        if (handledOk)
            return handledOk;
        // See if it is a command sequencer
        handledOk = _commandSequencer.procCommand(pCmdStr);
        if (handledOk)
            return handledOk;
        // See if it is a file
        handledOk = _fileSerializer.procFile(pCmdStr);
        return handledOk;
    }

private:
    static const char *trimWhitespace(const char *pStr)
    {
        while(isspace(*pStr))
            pStr++;
        return pStr;
    }

private:
    CommandInterface* _pCommandInterface;
    PatternEvaluator _patternEvaluator;
    CommandSequencer _commandSequencer;
    FileSerializer _fileSerializer;
};
