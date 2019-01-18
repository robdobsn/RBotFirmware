// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "WorkItem.h"
#include <queue>
#include "RdJson.h"

class WorkItemQueue
{
private:
    std::queue<WorkItem> _workItemQueue;
    unsigned int _workItemQueueMaxLen;
    static const unsigned int _workItemQueueMaxLenDefault = 50;

public:
    WorkItemQueue()
    {
        _workItemQueueMaxLen = _workItemQueueMaxLenDefault;
    }

    ~WorkItemQueue()
    {
    }

    // Set configuration
    void init(const char* configStr, const char* queueName)
    {
        String queueCfg = RdJson::getString(queueName, "{}", configStr);

//        Log.notice("Configuring WorkItemQueue from %s\n", configStr);
        _workItemQueueMaxLen = (int) RdJson::getLong("maxLen",
                                            _workItemQueueMaxLenDefault, queueCfg.c_str());
        clear();
//        Log.notice("MaxLen %d\n", _workItemQueueMaxLen);
    }

    // Check if queue full
    bool isFull()
    {
        return (_workItemQueue.size() >= _workItemQueueMaxLen);
    }

    // Check if queue empty
    bool isEmpty()
    {
        return (_workItemQueue.size() == 0);
    }

    // Clear the queue
    void clear()
    {
        // Log.notice("Clearing Command Queue size %d max %d\n", _workItemQueue.size(), _workItemQueueMaxLen);
        while(!_workItemQueue.empty()) 
            _workItemQueue.pop();
        // Log.notice("Cleared Command Queue size %d max %d\n", _workItemQueue.size(), _workItemQueueMaxLen);
    }

    // Add to queue
    bool add(const char* pWorkItemStr)
    {
        // Check if queue is full
        if (_workItemQueue.size() >= _workItemQueueMaxLen)
        {
        //    Log.notice("Command Queue FULL size %d max %d\n", _workItemQueue.size(), _workItemQueueMaxLen);
            return false;
        }

        // Queue up the item
        _workItemQueue.push(WorkItem(pWorkItemStr));
        return true;
    }

    // Peek the queue
    bool peek(WorkItem& workItem)
    {
        // Check if queue is empty
        if (_workItemQueue.empty())
        {
            return false;
        }

        // read the item
        workItem = _workItemQueue.front();
        return true;
    }

    // Get from queue
    bool get(WorkItem& workItem)
    {
        // Check if queue is empty
        if (_workItemQueue.empty())
        {
            return false;
        }

        // read the item and remove
        workItem = _workItemQueue.front();
        _workItemQueue.pop();
        return true;
    }

    // Get from queue
    bool get(String& workItemStr)
    {
        // Check if queue is empty
        if (_workItemQueue.empty())
        {
            return false;
        }

        // read the item and remove
        workItemStr = _workItemQueue.front().getString();
        _workItemQueue.pop();
        return true;
    }

    // Get size
    int size()
    {
        return _workItemQueue.size();
    }

};
