// RBotFirmware
// Rob Dobson 2016

#ifndef _COMMAND_QUEUE_H_
#define _COMMAND_QUEUE_H_

#include "application.h"
#include "CommandElem.h"
#include <queue>

class CommandQueue
{
private:
    std::queue<CommandElem> _cmdElemQueue;
    int _cmdQueueMaxLen;
    static const int _cmdQueueMaxLenDefault = 50;

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
//        Log.info("Configuring CommandQueue from %s", configStr);
        _cmdQueueMaxLen = (int) ConfigManager::getLong("cmdQueueMaxLen",
                                            _cmdQueueMaxLenDefault, configStr);
//        Log.info("CmdQueueMaxLen %d", _cmdQueueMaxLen);
    }

    // Check if queue full
    bool isFull()
    {
        return (_cmdElemQueue.size() >= _cmdQueueMaxLen);
    }

    // Add to queue
    bool add(const char* pCmdStr)
    {
        // Check if queue is full
        if (_cmdElemQueue.size() >= _cmdQueueMaxLen)
        {
            return false;
        }

        // Queue up the item
        _cmdElemQueue.push(CommandElem(pCmdStr));
        return true;
    }

    // Get from queue
    bool get(CommandElem& cmdElem)
    {
        // Check if queue is empty
        if (_cmdElemQueue.empty())
        {
            return false;
        }

        // read the item and remove
        cmdElem = _cmdElemQueue.front();
        _cmdElemQueue.pop();
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
