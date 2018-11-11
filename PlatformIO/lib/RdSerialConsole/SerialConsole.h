// Serial Console
// Rob Dobson 2018

#pragma once

#include <Arduino.h>
#include "RestAPIEndpoints.h"
#include "ConfigBase.h"

typedef void (*SerialConsoleCallbackType)(const char *cmdStr, String &retStr);

class SerialConsole
{
public:
    static constexpr char ASCII_XOFF = 0x13;
    static constexpr char ASCII_XON = 0x11;
    typedef char CommandRxState;
    static constexpr CommandRxState CommandRx_idle = 'i';
    static constexpr CommandRxState CommandRx_newChar = ASCII_XOFF;
    static constexpr CommandRxState CommandRx_waiting = 'w';
    static constexpr CommandRxState CommandRx_complete = ASCII_XON;

private:
    int _serialPortNum;
    String _curLine;
    static const int MAX_REGULAR_LINE_LEN = 100;
    static const int ABS_MAX_LINE_LEN = 1000;
    RestAPIEndpoints* _pEndpoints;
    int _prevChar;
    CommandRxState _cmdRxState;

public:
    SerialConsole()
    {
        _pEndpoints = NULL;
        _serialPortNum = 0;
        _curLine.reserve(MAX_REGULAR_LINE_LEN);
        _prevChar = -1;
        _cmdRxState = CommandRx_idle;
    }

    // Init
    void setup(ConfigBase& hwConfig, RestAPIEndpoints &endpoints);
    int getChar();

    // Get the state of the reception of Commands 
    // Maybe:
    //   idle = 'i' = no command entry in progress,
    //   newChar = XOFF = new command char received since last call - pause transmission
    //   waiting = 'w' = command incomplete but no new char since last call
    //   complete = XON = command completed - resume transmission
    CommandRxState getXonXoff();

    // Call frequently
    void service();
};
