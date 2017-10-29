// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "ConfigManager.h"

class ConfigEEPROM : public ConfigManager
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

public:
    ConfigEEPROM()
    {
        _eepromBaseLocation = 0;
        _configMaxDataLen = 1000;
        _isWriting = false;
        _curWriteOffset = 0;
        _numBytesToWrite = 0;
        _stringToWrite = "";
    }

    ~ConfigEEPROM()
    {
    }

    // Initialise
    bool setConfigLocation(const char* configStr)
    {
        // configStr defines the location of config
        bool isValid = false;
        long configPos = ConfigManager::getLong("base", 0, isValid, configStr);
        if (!isValid)
        {
            Log.error("configLocation base not found");
    		return false;
        }

        long configMaxLen = ConfigManager::getLong("maxLen", 500, isValid, configStr);
        if (!isValid)
        {
            Log.error("configLocation maxLen not found");
    		return false;
        }

        // Config base in EEPROM
        _eepromBaseLocation = configPos;

        // Config max len
        _configMaxDataLen = configMaxLen;

        // Debug
        Log.info("configEEPROM base %d, maxLen %d", _eepromBaseLocation, _configMaxDataLen);

        // Read the config JSON str from EEPROM
        readFromEEPROM();

        return true;
    }

    // Read a configuration string from EEPROM
    void readFromEEPROM()
    {
        // Check EEPROM has been initialised - if not just start with a null string
        if (EEPROM.read(_eepromBaseLocation) == 0xff)
        {
            setConfigData("");
            Log.info("EEPROM uninitialised, _pDataStrJSON empty");
            return;
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
        char* pData = new char[dataStrLen + 1];

        // Fill string from EEPROM location
        for (int chIdx = 0; chIdx < dataStrLen; chIdx++)
        {
            char ch = EEPROM.read(_eepromBaseLocation + chIdx);
            pData[chIdx] = ch;
        }
        pData[dataStrLen + 1] = 0;
        Log.info("Read config string from EEPROM length: %d", strlen(pData));

        // Store in config string
        setConfigData(pData);

        // Tidy up
        delete [] pData;
    }


    // Write configuration string to EEPROM
    bool writeToEEPROM()
    {
        const char* pConfigData = getConfigData();

        Log.info("Start writing config str to EEPROM len %d ...", strlen(pConfigData));

        // Remember data to write (in case it changes in the background)
        _stringToWrite = pConfigData;
        _curWriteOffset = 0;

        // Get length of string
        int dataStrLen = 0;
        if (pConfigData != NULL)
        {
            dataStrLen = strlen(pConfigData);
        }
        if (dataStrLen >= _configMaxDataLen)
        {
            dataStrLen = _configMaxDataLen - 1;
        }

        // Length of string
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
            Log.info("Writing config to EEPROM Complete");
        }
    }
};
