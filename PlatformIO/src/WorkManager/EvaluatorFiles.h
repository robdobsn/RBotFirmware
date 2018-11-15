// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "FileManager.h"

class WorkManager;
class WorkItem;

class EvaluatorFiles
{
public:
    EvaluatorFiles(FileManager& fileManager) : _fileManager(fileManager)
    {
        _inProgress = false;
    }

    // Config
    void setConfig(const char* configStr);
    const char* getConfig();

    // Check valid
    bool isValid(WorkItem& workItem);

    // Process WorkItem
    bool execWorkItem(WorkItem& workItem);

    // Call frequently
    void service(WorkManager* pWorkManager);

    // Control
    void stop();
    
private:
    // Filename in progress
    bool _inProgress;

    // File manager
    FileManager& _fileManager;

};
