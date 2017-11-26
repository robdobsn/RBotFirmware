// RBotFirmware
// Rob Dobson 2016-2017

#pragma once

class ConfigEEPROM
{
private:
    // Base location to store dataStr in EEPROM
    int _eepromBaseLocation;

    // Max length of config data
    int _configMaxDataLen;

    // Write state
    bool _isWriting;
    int _curWriteOffset;
    int _numBytesToWrite;
    String _stringToWrite;

    // Dirty flag - when set indicates a write is needed
    bool _dirtyFlag;

public:
    ConfigEEPROM(int configBase, int configMaxLen)
    {
        _eepromBaseLocation = configBase;
        _configMaxDataLen = configMaxLen;
        _isWriting = false;
        _curWriteOffset = 0;
        _numBytesToWrite = 0;
        _stringToWrite = "";
        _dirtyFlag = false;
    }

    ~ConfigEEPROM()
    {
    }

    bool isDirty()
    {
        return _dirtyFlag;
    }

    void setDirty()
    {
        _dirtyFlag = true;
        Log.info("setDirty");
    }

    // Read a configuration string from EEPROM
    String read()
    {
        // Check EEPROM has been initialised - if not just start with a null string
        if (EEPROM.read(_eepromBaseLocation) == 0xff)
        {
            Log.info("ConfigEEPROM::read uninitialised");
            return "";
        }

        // Find out how long the string is - don't allow > _configMaxDataLen
        int dataStrLen = _configMaxDataLen - 1;
        for (int chIdx = 0; chIdx < _configMaxDataLen; chIdx++)
        {
            int ch = EEPROM.read(_eepromBaseLocation + chIdx);
            if (ch == 0)
            {
                dataStrLen = chIdx;
                break;
            }
        }

        // Get string from EEPROM
        String dataStr;
        dataStr.reserve(dataStrLen);

        // Fill string from EEPROM location
        for (int chIdx = 0; chIdx < dataStrLen; chIdx++)
        {
            char ch = EEPROM.read(_eepromBaseLocation + chIdx);
            dataStr.concat(ch);
        }
        Log.info("ConfigEEPROM::read %d chars", dataStrLen);
        _dirtyFlag = false;
        return dataStr;
    }


    // Write configuration string to EEPROM
    bool write(const char* pDataStr)
    {
        Log.info("ConfigEEPROM:write writing %d bytes", strlen(pDataStr));

        // Copy data to write (in case it changes in the background)
        _stringToWrite = pDataStr;
        _curWriteOffset = 0;
        _dirtyFlag = false;

        // Get length of string
        int dataStrLen = _stringToWrite.length();
        if (dataStrLen >= _configMaxDataLen)
            dataStrLen = _configMaxDataLen - 1;

        // Record details to write
        _numBytesToWrite = dataStrLen;
        _isWriting = true;
        return true;
    }

    // Service the writing process
    void service()
    {
        // Only if writing
        if (!_isWriting)
            return;
        // Write next char
        if (_curWriteOffset < _numBytesToWrite)
        {
            EEPROM.write(_eepromBaseLocation + _curWriteOffset, _stringToWrite.charAt(_curWriteOffset));
            // Log.trace("Writing %c", _stringToWrite.charAt(_curWriteOffset));
        }
        else
        {
            EEPROM.write(_eepromBaseLocation + _curWriteOffset, 0);
            // Log.trace("Writing 0 terminator");
        }
        // Bump
        _curWriteOffset += 1;
        if (_curWriteOffset > _numBytesToWrite)
        {
            // Clean up
            _isWriting = false;
            _stringToWrite = "";
            Log.info("ConfigEEPROM:service Write Complete");
        }
    }
};
