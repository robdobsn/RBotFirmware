// RBotFirmware
// Rob Dobson 2017

#include <Arduino.h>
#include <ArduinoLog.h>
#include "EvaluatorSequences.h"
#include "RdJson.h"
#include "WorkManager.h"

static const char* MODULE_PREFIX = "EvaluatorSequences: ";

void EvaluatorSequences::setConfig(const char* configStr)
{
    // Store the config string
    _jsonConfigStr = configStr;
}

const char* EvaluatorSequences::getConfig()
{
    return _jsonConfigStr.c_str();
}

// Process WorkItem
bool EvaluatorSequences::execWorkItem(WorkItem& workItem)
{
    // Find the command info
    bool isValid = false;
    String sequenceName = workItem.getString();
    String seqStr = RdJson::getString(sequenceName.c_str(), "{}", _jsonConfigStr.c_str(), isValid);
    // Log.verbose("%scmdStr %s seqStr %s\n", MODULE_PREFIX, cmdStr, seqStr.c_str());
    if (isValid)
    {
        _numCmdsToProcess = 0;
        String cmdList = RdJson::getString("commands", "", seqStr.c_str(), isValid);
        Log.trace("%scmdStr %s isValid %d seqStr %s cmdList %s\n", MODULE_PREFIX, sequenceName.c_str(), isValid, seqStr.c_str(), cmdList.c_str());
        if (isValid)
        {
            _commandList = cmdList;
            _curCmdIdx = 0;
            // Count the separators
            if (cmdList.length() > 0)
            {
                int numSeps = 0;
                const char* pStr = cmdList.c_str();
                while(*pStr)
                {
                    if (*pStr == ';')
                        numSeps++;
                    pStr++;
                }
                _numCmdsToProcess = numSeps + 1;
                Log.trace("%scmdStr %s seqStr %s cmdList %s numCmds %d\n", MODULE_PREFIX, sequenceName.c_str(), seqStr.c_str(), cmdList.c_str(), _numCmdsToProcess);
            }
        }
    }
    return isValid;
}

void EvaluatorSequences::service(WorkManager* pWorkManager)
{
    // TODO check this is valid ...

    // if (millis() > _lastMillis + 10000)
    // {
    //     _lastMillis = millis();
    //     Log.trace("%sprocess cmdStr %s cmdIdx %d numToProc %d isEmpty %d\n", MODULE_PREFIX, _commandList.c_str(), _curCmdIdx, _numCmdsToProcess,
    //                     pWorkManager->queueIsEmpty());
    // }
    // Check there is something left to do
    if (_numCmdsToProcess <= _curCmdIdx)
        return;

    // Only add process commands at this level if the queue is completely empty
    if (!pWorkManager->queueIsEmpty())
        return;

    // Process the next command
    Log.verbose("%scmdInterp cmdStr %s cmdIdx %d numToProc %d\n", MODULE_PREFIX, _commandList.c_str(), _curCmdIdx, _numCmdsToProcess);
    String retStr;
    WorkItem workItem(_commandList);
    pWorkManager->addWorkItem(workItem, retStr, _curCmdIdx++);
}

void EvaluatorSequences::stop()
{
    _commandList = "";
    _numCmdsToProcess = 0;
    _curCmdIdx = 0;
}