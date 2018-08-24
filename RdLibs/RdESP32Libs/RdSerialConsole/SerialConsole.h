// RinggitTheSalesBell
// Rob Dobson 2018

#pragma once

typedef void (*SerialConsoleCallbackType)(const char *cmdStr, String &retStr);

class SerialConsole
{
  private:
    int _serialPortNum;
    String _curLine;
    static const int MAX_REGULAR_LINE_LEN = 100;
    static const int ABS_MAX_LINE_LEN = 1000;
    RestAPIEndpoints* _pEndpoints;
    int _prevChar;

  public:
    SerialConsole()
    {
        _pEndpoints = NULL;
        _serialPortNum = 0;
        _curLine.reserve(MAX_REGULAR_LINE_LEN);
        _prevChar = -1;
    }

    void setup(int serialPortNum, RestAPIEndpoints &endpoints)
    {
        _pEndpoints = &endpoints;
        _serialPortNum = serialPortNum;
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

    void service()
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
                        const char* endpointStr = _pEndpoints->getNthEndpointStr(i);
                        const char* descr = _pEndpoints->getNthEndpointDescr(i);
                        Serial.println(String(" ") + endpointStr + String(": ") +  descr);
                    }
                    return;
                }
            }

            Serial.println();
            // Check for immediate instructions
            if (_pEndpoints)
            {
                Log.trace("CommsSerial ->cmdInterp cmdStr %s\n", _curLine.c_str());
                String retStr;
                _pEndpoints->handleApiRequest(_curLine.c_str(), retStr);
                // Display response
                Serial.println(retStr);
            }

            // Reset line
            _curLine = "";
            return;
        }

        // Store previous char for CRLF checks
        _prevChar = ch;

        // Check line not too long
        if (_curLine.length() >= ABS_MAX_LINE_LEN)
        {
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

        // Add char to line
        _curLine.concat((char)ch);
        Serial.print((char)ch);

        //Log.trace("Str = %s (%c)\n", _curLine.c_str(), ch);
    }
};
