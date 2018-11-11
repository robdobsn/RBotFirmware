// RBotFirmware
// Rob Dobson 2017

#pragma once

class WorkManager;
class WorkItem;

class EvaluatorSequences
{
public:
    EvaluatorSequences()
    {
        _curCmdIdx = 0;
        _numCmdsToProcess = 0;
        // _lastMillis = 0;
    }

    // Config
    void setConfig(const char* configStr);
    const char* getConfig();

    // Process WorkItem
    bool execWorkItem(WorkItem& workItem);

    // Call frequently
    void service(WorkManager* pWorkManager);

    // Control
    void stop();
    
private:
    // Full configuration JSON
    String _jsonConfigStr;

    // List of commands to add to workflow - delimited string
    String _commandList;

    // Number of commands in command list and current position in list
    int _numCmdsToProcess;
    int _curCmdIdx;
};
