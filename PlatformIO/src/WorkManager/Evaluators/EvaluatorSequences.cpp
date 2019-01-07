// RBotFirmware
// Rob Dobson 2017

#include <Arduino.h>
#include <ArduinoLog.h>
#include "EvaluatorSequences.h"
#include "RdJson.h"
#include "../WorkManager.h"

static const char* MODULE_PREFIX = "EvaluatorSequences: ";

EvaluatorSequences::EvaluatorSequences(FileManager& fileManager, WorkManager& workManager) :
         _fileManager(fileManager), _workManager(workManager)
{
    _inProgress = 0;
    _reqLineIdx = 0;
    _linesDone = 0;
    _defaultShuffleMode = false;
    _defaultRepeatMode = false;
    _shuffleMode = false;
    _repeatMode = false;
    _lineCount = 0;
}

void EvaluatorSequences::setConfig(const char* configStr)
{
    // Store the config string
    _jsonConfigStr = configStr;
    _defaultShuffleMode = RdJson::getLong("seqShuffleMode", 0, configStr) != 0;
    _defaultRepeatMode = RdJson::getLong("seqRepeatMode", 0, configStr) != 0;
    _lineCount = 0;
    Log.trace("%ssetConfig defaultShuffleMode %s defaultRepeatMode %s\n", MODULE_PREFIX, 
                _shuffleMode ? "Y" : "N",  _repeatMode ? "Y" : "N");
}

const char* EvaluatorSequences::getConfig()
{
    return _jsonConfigStr.c_str();
}

// Is Busy
bool EvaluatorSequences::isBusy()
{
    return _inProgress;
}

// Check if valid
bool EvaluatorSequences::isValid(WorkItem& workItem)
{
    // Form the file name
    String fileName = workItem.getString();
    // Check extension valid
    String fileExt = FileManager::getFileExtension(fileName);
    if (!fileExt.equalsIgnoreCase("seq"))
        return false;
    // Check on file system
    int fileLen = 0;
    bool rslt = _fileManager.getFileInfo("", fileName, fileLen);
    if (fileLen == 0)
        return false;
    return rslt;
}

// Count lines
int EvaluatorSequences::countLines(String& lines)
{
    int lineCount = 0;
    bool lineBlank = true;
    const char* pStr = lines.c_str();
    while(true)
    {
        if ((*pStr == '\n') || (*pStr == 0))
        {
            if (!lineBlank)
                lineCount++;
            lineBlank = true;
            if (*pStr == 0)
                break;
        }
        else
        {
            if (*pStr != '\r')
                lineBlank = false;
        }
        pStr++;
    }
    return lineCount;
}

// Process WorkItem
bool EvaluatorSequences::execWorkItem(WorkItem& workItem)
{
    // Find the command info
    String fileName = workItem.getString();
    _commandList = _fileManager.getFileContents("", fileName, MAX_SEQUENCE_FILE_LEN);
    if (_commandList.length() > 0)
    {
        _inProgress = true;
        _shuffleMode = _defaultShuffleMode;
        _repeatMode = _defaultRepeatMode;
        _lineCount = countLines(_commandList);
        if (_commandList.indexOf("ShuffleMode") >= 0)
            _shuffleMode = true;
        if (_commandList.indexOf("NoShuffleMode") >= 0)
            _shuffleMode = false;
        if (_commandList.indexOf("RepeatMode") >= 0)
            _repeatMode = true;
        if (_commandList.indexOf("NoRepeatMode") >= 0)
            _repeatMode = false;
        _linesDone = 0;
        _reqLineIdx = 0;
        if (_shuffleMode)
            _reqLineIdx = rand() % _lineCount;
        Log.trace("%sexecWorkItem len %d lineCount %d reqLineIdx %d shuffleMode %s repeatMode %s\n", MODULE_PREFIX, 
                _commandList.length(), _lineCount, _reqLineIdx, _shuffleMode ? "Y" : "N",  _repeatMode ? "Y" : "N");
        return true;
    }
    Log.trace("%sexecWorkItem Not Found %s\n", MODULE_PREFIX, _commandList.c_str());
    return false;
}

void EvaluatorSequences::service()
{
    // Only add process commands at this level if the workitem queue is completely empty
    if (!_workManager.queueIsEmpty())
        return;

    // Check if operative
    if (!_inProgress)
        return;
        
    // Get required line
    const char* pCommandList = _commandList.c_str();
    const char* pStr = pCommandList;
    int lineStartPos = 0;
    int sepIdx = 0;
    bool lineBlank = true;
    while(true)
    {
        if ((*pStr == '\n') || (*pStr == 0))
        {
            if (!lineBlank)
                sepIdx++;
            if ((sepIdx == _reqLineIdx+1) || (*pStr == 0))
                break;
            lineBlank = true;
            lineStartPos = pStr - pCommandList;
        }
        else
        {
            if (*pStr != '\r')
                lineBlank = false;
        }
        pStr++;
    }
    if (sepIdx == _reqLineIdx+1)
    {
        // Line to process
        String newCmd = _commandList.substring(lineStartPos, pStr-pCommandList);
        newCmd.trim();
        // Separator found so add command
        Log.trace("%sservice reqLineIdx %d cmd %s\n", MODULE_PREFIX, 
                _reqLineIdx, newCmd.c_str());
        if (newCmd.length() > 0)
        {
            String retStr;
            WorkItem workItem(newCmd);
            _workManager.addWorkItem(workItem, retStr, _reqLineIdx);
        }
        // Bump
        _linesDone++;
        if ((_linesDone == _lineCount) && !_repeatMode)
        {
            _inProgress = false;
            Log.trace("%sservice linesDone %d lineCount %d no repeat so stopping\n", MODULE_PREFIX, 
                _linesDone, _lineCount);
        }
        // Next req item
        _reqLineIdx++;
        if (_reqLineIdx >= _lineCount)
            _reqLineIdx = 0;
        if (_shuffleMode)
            _reqLineIdx = rand() % _lineCount;
    }
    else
    {
        // Separator not found so stop
        _inProgress = false;
        Log.trace("%sservice reqLineIdx %d not found so stopping\n", MODULE_PREFIX, 
                _reqLineIdx);
    }

    // if (millis() > _lastMillis + 10000)
    // {
    //     _lastMillis = millis();
    //     Log.trace("%sprocess cmdStr %s cmdIdx %d numToProc %d isEmpty %d\n", MODULE_PREFIX, _commandList.c_str(), _curCmdIdx, _numCmdsToProcess,
    //                     _workManager.queueIsEmpty());
    // }
}

void EvaluatorSequences::stop()
{
    _inProgress = false;
}
