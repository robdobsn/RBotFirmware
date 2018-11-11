// Serial Console
// Rob Dobson 2018

#include "SerialConsole.h"

void SerialConsole::setup(ConfigBase& hwConfig, RestAPIEndpoints &endpoints)
{
    ConfigBase consoleConfig(hwConfig.getString("serialConsole", "").c_str());
    _serialPortNum = consoleConfig.getLong("portNum", 0);
    _pEndpoints = &endpoints;
}

int SerialConsole::getChar()
{
    if (_serialPortNum == 0)
    {
        // Get char
        return Serial.read();
    }
    return -1;
}

// Get the state of the reception of Commands 
// Maybe:
//   idle = 'i' = no command entry in progress,
//   newChar = XOFF = new command char received since last call - pause transmission
//   waiting = 'w' = command incomplete but no new char since last call
//   complete = XON = command completed - resume transmission
SerialConsole::CommandRxState SerialConsole::getXonXoff()
{
    char curSt = _cmdRxState;
    if (_cmdRxState == CommandRx_complete)
    {
        // Serial.printf("<COMPLETE>");
        _cmdRxState = CommandRx_idle;
    }
    else if (_cmdRxState == CommandRx_newChar)
    {
        // Serial.printf("<NEWCH>");
        _cmdRxState = CommandRx_waiting;
    }
    return curSt;
}

void SerialConsole::service()
{
    // Check for char
    int ch = getChar();
    if (ch == -1)
        return;

    // Check for line end
    if ((ch == '\r') || (ch == '\n'))
    {
        // Check for terminal sending a CRLF sequence
        if (_prevChar == '\r' || _prevChar == '\n')
            return;
        _prevChar = ch;

        // Check if empty line - show menu
        if (_curLine.length() <= 0)
        {
            Serial.printf("Configuration Options ch=%d\n", ch);
            // Show endpoints
            if (_pEndpoints)
            {
                for (int i = 0; i < _pEndpoints->getNumEndpoints(); i++)
                {
                    RestAPIEndpointDef* pEndpoint = _pEndpoints->getNthEndpoint(i);
                    if (!pEndpoint)
                        continue;
                    Serial.println(String(" ") + pEndpoint->_endpointStr + String(": ") +  pEndpoint->_description);
                    Serial.println();
                }
                return;
            }
        }

        Serial.println();
        // Check for immediate instructions
        if (_pEndpoints)
        {
            Log.trace("CommsSerial: ->cmdInterp cmdStr %s\n", _curLine.c_str());
            String retStr;
            _pEndpoints->handleApiRequest(_curLine.c_str(), retStr);
            // Display response
            Serial.println(retStr);
            Serial.println();
        }

        // Reset line
        _curLine = "";
        _cmdRxState = CommandRx_complete;
        return;
    }

    // Store previous char for CRLF checks
    _prevChar = ch;

    // Check line not too long
    if (_curLine.length() >= ABS_MAX_LINE_LEN)
    {
        _curLine = "";
        _cmdRxState = CommandRx_idle;
        return;
    }

    // Check for backspace
    if (ch == 0x08)
    {
        if (_curLine.length() > 0)
        {
            _curLine.remove(_curLine.length() - 1);
            Serial.print((char)ch);
            Serial.print(' ');
            Serial.print((char)ch);
        }
        return;
    }

    // Output for user to see
    if (_curLine.length() == 0)
        Serial.println();
    Serial.print((char)ch);

    // Add char to line
    _curLine.concat((char)ch);

    // Set state to show we're busy getting a command
    _cmdRxState = CommandRx_newChar;

    //Log.trace("Str = %s (%c)\n", _curLine.c_str(), ch);
}
