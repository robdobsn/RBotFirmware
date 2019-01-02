// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

class WorkItem
{
private:
    String _str;

public:
    WorkItem()
    {
        _str = "";
    }

    WorkItem(const char* pCmdStr)
    {
        _str = pCmdStr;
    }

    WorkItem(const String& cmdStr)
    {
        _str = cmdStr;
    }

    const char* getCString()
    {
        return _str.c_str();
    }

    const String& getString()
    {
        return _str;
    }
};
