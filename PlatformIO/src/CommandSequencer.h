// RBotFirmware
// Rob Dobson 2017

#include "CommandInterface.h"

class CommandSequencer
{
public:
    CommandSequencer()
    {
        _curCmdIdx = 0;
        _numCmdsToProcess = 0;
        // _lastMillis = 0;
    }

    void setConfig(const char* configStr)
    {
        // Store the config string
        _jsonConfigStr = configStr;
    }

    const char* getConfig()
    {
        return _jsonConfigStr.c_str();
    }

    // Process a command sequence
    bool procCommand(const char* cmdStr)
    {
        // Find the command info
        bool isValid = false;
        String seqStr = RdJson::getString(cmdStr, "{}", _jsonConfigStr.c_str(), isValid);
        // Log.trace("CommandSequencer cmdStr %s seqStr %s\n", cmdStr, seqStr.c_str());
        if (isValid)
        {
            _numCmdsToProcess = 0;
            String cmdList = RdJson::getString("commands", "", seqStr.c_str(), isValid);
            Log.trace("CommandSequencer cmdStr %s isValid %d seqStr %s cmdList %s\n", cmdStr, isValid, seqStr.c_str(), cmdList.c_str());
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
                    Log.trace("CommandSequencer cmdStr %s seqStr %s cmdList %s numCmds %d\n", cmdStr, seqStr.c_str(), cmdList.c_str(), _numCmdsToProcess);
                }
            }
        }
        return isValid;
    }

    void service(CommandInterface* pCommandInterface)
    {
        // if (millis() > _lastMillis + 10000)
        // {
        //     _lastMillis = millis();
        //     Log.trace("CommandSequencer process cmdStr %s cmdIdx %d numToProc %d isEmpty %d\n", _commandList.c_str(), _curCmdIdx, _numCmdsToProcess,
        //                     pCommandInterpreter->queueIsEmpty());
        // }
        // Check there is something left to do
        if (_numCmdsToProcess <= _curCmdIdx)
            return;

        // Only add process commands at this level if the queue is completely empty
        if (!pCommandInterface->queueIsEmpty())
            return;

        // Process the next command
        Log.trace("CommandSequencer ->cmdInterp cmdStr %s cmdIdx %d numToProc %d\n", _commandList.c_str(), _curCmdIdx, _numCmdsToProcess);
        String retStr;
        pCommandInterface->process(_commandList.c_str(), retStr, _curCmdIdx++);
    }

    void stop()
    {
        _commandList = "";
        _numCmdsToProcess = 0;
        _curCmdIdx = 0;
    }
private:
    // Full configuration JSON
    String _jsonConfigStr;

    // List of commands to add to workflow - delimited string
    String _commandList;

    // Number of commands in command list and current position in list
    int _numCmdsToProcess;
    int _curCmdIdx;

    // long _lastMillis;
};
