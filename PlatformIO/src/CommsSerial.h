// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "ConfigManager.h"
#include "CommandInterpreter.h"

// Serial communication protocol
class CommsSerial
{
private:
    int _serialPortNum;
    String _curLine;
    static const int MAX_REGULAR_LINE_LEN = 100;
    static const int ABS_MAX_LINE_LEN = 1000;

public:
    CommsSerial(int serialPortNum)
    {
        _serialPortNum = serialPortNum;
        _curLine.reserve(MAX_REGULAR_LINE_LEN);
    }

    int getChar()
    {
        if (_serialPortNum == 0)
        {
            // Get char
            return Serial.read();
        }
        return -1;
    }

    void service(CommandInterpreter& commandInterpreter)
    {
        // Check for char
        int ch = getChar();
        if (ch == -1)
            return;

        // Check for line end
        if ((ch == '\r') || (ch == '\n'))
        {
            // Check if empty line - ignore
            if (_curLine.length() <= 0)
                return;

            // Check for immediate instructions
            Log.trace("CommsSerial ->cmdInterp cmdStr %s", _curLine.c_str());
            String retStr;
            commandInterpreter.process(_curLine, retStr);

            // Reset line
            _curLine = "";
            return;
        }

        // Check line not too long
        if (_curLine.length() >= ABS_MAX_LINE_LEN)
        {
            return;
        }

        // Add char to line
        _curLine.concat((char)ch);

        //Log.trace("Str = %s (%c)", _curLine.c_str(), ch);
    }
};
