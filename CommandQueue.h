// RBotFirmware
// Rob Dobson 2016

#ifndef _COMMAND_QUEUE_H_
#define _COMMAND_QUEUE_H_

#include "application.h"
#include <queue>

class CmdQueueElem
{
private:
    String _cmdStr;

public:
    CmdQueueElem(String& cmdStr)
    {
        _cmdStr = cmdStr;
    }

    String getString()
    {
        return _cmdStr;
    }
};

class CommandQueue
{
private:
    std::queue<CmdQueueElem> _cmdElemQueue;
    int _cmdQueueMaxLen;
    static const int _cmdQueueMaxLenDefault = 100;

public:
    CommandQueue()
    {
        _cmdQueueMaxLen = _cmdQueueMaxLenDefault;
    }

    ~CommandQueue()
    {
    }

    // Set configuration
    void init(const char* configStr)
    {
        Log.info("Configuring CommandQueue from %s", configStr);
        _cmdQueueMaxLen = (int) ConfigManager::getLong("cmdQueueMaxLen",
                                            _cmdQueueMaxLenDefault, configStr);
        Log.info("CmdQueueMaxLen %d", _cmdQueueMaxLen);
    }

    // Add to queue
    bool add(String& cmdStr)
    {
        // Check if queue is full
        if (_cmdElemQueue.size() >= _cmdQueueMaxLen)
        {
            return false;
        }

        // Queue up the item
        _cmdElemQueue.push(CmdQueueElem(cmdStr));
        return true;
    }

    // Get from queue
    bool get(String& cmdStr)
    {
        // Check if queue is empty
        if (_cmdElemQueue.empty())
        {
            return false;
        }

        // read the item and remove
        cmdStr = _cmdElemQueue.front().getString();
        _cmdElemQueue.pop();
        return true;
    }

};

#endif // _COMMAND_QUEUE_H_
