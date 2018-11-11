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

    const String getString()
    {
        return _str;
    }
};
