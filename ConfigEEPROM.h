// RBotFirmware
// Rob Dobson 2016

#ifndef _CONFIG_EEPROM_H_
#define _CONFIG_EEPROM_H_

#include "ConfigManager.h"

#pragma push_macro("RD_DEBUG_FNAME")
#define RD_DEBUG_FNAME "ConfigEEPROM.h"
#include "RdDebugLevel.h"

class ConfigEEPROM
{
private:
    // Base location to store dataStr in EEPROM
    int _eepromBaseLocation;

    // Max length of config data
    int _configMaxDataLen;

public:
    ConfigEEPROM()
    {
    }

    ~ConfigEEPROM()
    {
    }

    // Initialise
    bool setConfigLocation(const char* configStr)
    {
        // configStr defines the location of config
        bool isValid = false;
        jsmntype_t objType = JSMN_UNDEFINED;
        int objSize = 0;
        long configPos = getLong("base", 0, isValid, configStr);
        if (!isValid)
        {
            RD_ERR("configLocation base not found");
    		return false;
        }

        long configMaxLen = getLong("maxLen", 500, isValid, configStr);
        if (!isValid)
        {
            RD_ERR("configLocation maxLen not found");
    		return false;
        }

        // Config base in EEPROM
        _eepromBaseLocation = configPos;

        // Config max len
        _configMaxDataLen = configMaxLen;

        // Debug
        RD_INFO("configLocation source %s, base %ld, maxLen %ld",
                configSource.c_str(), _eepromBaseLocation, _configMaxDataLen);

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
            delete[] _pDataStrJSON;
            _pDataStrJSON = NULL;
            RD_INFO("EEPROM uninitialised, _pDataStrJSON empty");
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

        // Set initial size of string to avoid unnecessary resizing as we read it
        delete[] _pDataStrJSON;
        _pDataStrJSON = new char[dataStrLen + 1];

        // Fill string from EEPROM location
        for (int chIdx = 0; chIdx < dataStrLen; chIdx++)
        {
            char ch = EEPROM.read(_eepromBaseLocation + chIdx);
            _pDataStrJSON[chIdx] = ch;
        }
        _pDataStrJSON[dataStrLen + 1] = 0;

        RD_INFO("Read config str: %s", _pDataStrJSON);
    }


    // Write configuration string to EEPROM
    bool writeToEEPROM()
    {
        RD_DBG("Writing config str: %s", _pDataStrJSON);

        // Get length of string
        int dataStrLen = 0;
        if (_pDataStrJSON != NULL)
        {
            dataStrLen = strlen(_pDataStrJSON);
        }
        if (dataStrLen >= _configMaxDataLen)
        {
            dataStrLen = _configMaxDataLen - 1;
        }

        // Write the current value of the string to EEPROM
        for (int chIdx = 0; chIdx < dataStrLen; chIdx++)
        {
            EEPROM.write(_eepromBaseLocation + chIdx, _pDataStrJSON[chIdx]);
        }

        // Terminate string
        EEPROM.write(_eepromBaseLocation + dataStrLen, 0);

        return true;
    }
};

#pragma pop_macro("RD_DEBUG_FNAME")

#endif _CONFIG_EEPROM_H_
