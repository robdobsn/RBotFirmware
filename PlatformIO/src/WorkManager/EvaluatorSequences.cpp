// RBotFirmware
// Rob Dobson 2017

#include <Arduino.h>
#include <ArduinoLog.h>
#include "EvaluatorSequences.h"
#include "RdJson.h"
#include "WorkManager.h"

static const char* MODULE_PREFIX = "EvaluatorSequences: ";

EvaluatorSequences::EvaluatorSequences(FileManager& fileManager) :
         _fileManager(fileManager)
{
    _inProgress = 0;
    _curLineIdx = 0;
}

void EvaluatorSequences::setConfig(const char* configStr)
{
    // Store the config string
    _jsonConfigStr = configStr;
}

const char* EvaluatorSequences::getConfig()
{
    return _jsonConfigStr.c_str();
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

// Process WorkItem
bool EvaluatorSequences::execWorkItem(WorkItem& workItem)
{
    // Find the command info
    String fileName = workItem.getString();
    _commandList = _fileManager.getFileContents("", fileName, MAX_SEQUENCE_FILE_LEN);
    Log.trace("%sseqStr %s\n", MODULE_PREFIX, _commandList.c_str());
    if (_commandList.length() > 0)
    {
        _inProgress = true;
        _curLineIdx = 0;
        return true;
    }
    return false;
}

void EvaluatorSequences::service(WorkManager* pWorkManager)
{
    // Only add process commands at this level if the workitem queue is completely empty
    if (!pWorkManager->queueIsEmpty())
        return;

    // Check if operative
    if (!_inProgress)
        return;
        
    // Get next line
    const char* pCommandList = _commandList.c_str();
    const char* pStr = pCommandList;
    int lineStartPos = 0;
    int sepIdx = 0;
    while(true)
    {
        if ((*pStr == '\n') || (*pStr == 0))
        {
            sepIdx++;
            if ((sepIdx == _curLineIdx+1) || (*pStr == 0))
                break;
            lineStartPos = pStr - pCommandList;
        }
        pStr++;
    }
    if (sepIdx == _curLineIdx+1)
    {
        // Line to process
        String newCmd = _commandList.substring(lineStartPos, pStr-pCommandList);
        newCmd.trim();
        // Separator found so add command
        Log.trace("%sservice curLineIdx %d cmd %s\n", MODULE_PREFIX, 
                _curLineIdx, newCmd.c_str());
        if (newCmd.length() > 0)
        {
            String retStr;
            WorkItem workItem(newCmd);
            pWorkManager->addWorkItem(workItem, retStr, _curLineIdx);
        }
        // Bump
        _curLineIdx++;
    }
    else
    {
        // Separator not found so we're done
        _inProgress = false;
        Log.trace("%sservice curLineIdx %d done\n", MODULE_PREFIX, 
                _curLineIdx);
    }

    // if (millis() > _lastMillis + 10000)
    // {
    //     _lastMillis = millis();
    //     Log.trace("%sprocess cmdStr %s cmdIdx %d numToProc %d isEmpty %d\n", MODULE_PREFIX, _commandList.c_str(), _curCmdIdx, _numCmdsToProcess,
    //                     pWorkManager->queueIsEmpty());
    // }
}

void EvaluatorSequences::stop()
{
    _inProgress = false;
}
