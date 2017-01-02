// RBotFirmware
// Rob Dobson 2016

#ifndef _WORKFLOW_MANAGER_H_
#define _WORKFLOW_MANAGER_H_

#include "application.h"
#include "CommandQueue.h"

class WorkflowManager
{
private:
    CommandQueue _cmdQueue;

public:
    WorkflowManager()
    {
    }

    ~WorkflowManager()
    {
    }

    // Init
    void init(const char* configStr)
    {
        Log.info("Constructing WorkflowManager from %s", configStr);
        String cmdQueueCfg = ConfigManager::getString("CommandQueue", "NONE", configStr);

        _cmdQueue.init(cmdQueueCfg);
    }

    // Add to workflow
    bool add(String& cmdStr)
    {
        return _cmdQueue.add(cmdStr);
    }

    // Get from workflow
    bool get(String& cmdStr)
    {
        return _cmdQueue.get(cmdStr);
    }

};

#endif // _WORKFLOW_MANAGER_H_
