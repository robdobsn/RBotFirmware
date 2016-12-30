// RBotFirmware
// Rob Dobson 2016

#ifndef _CONFIG_MANAGER_H_
#define _CONFIG_MANAGER_H_

#include "jsmnParticleR.h"

#define RD_DEBUG_LEVEL 4
#define RD_DEBUG_FNAME "ConfigManager.h"
#include "RdDebugLevel.h"

class ConfigManager
{
public:
    ConfigManager()
    {
        _pDataStrJSON = NULL;
    }

    // Initialise
    bool SetConfigLocation(const char* configStr, bool configStrDefinesLocation = true)
    {
        // Check if the configStr is the actual config or if it defines the
        // location of config
        if (configStrDefinesLocation)
        {
            bool isValid = false;
            String configSource = getString("source", "", isValid, configStr);
            if (!isValid)
            {
                RD_ERR("configLocation source not found");
        		return false;
            }
            if (configSource.equalsIgnoreCase("EEPROM"))
            {
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

                // Record source of config
                _configSource = CONFIG_SOURCE_EEPROM;

                // Config base in EEPROM
                _eepromBaseLocation = configPos;

                // Config max len
                _configMaxDataLen = configMaxLen;

                // Debug
                RD_INFO("configLocation source %s, base %ld, maxLen %ld",
                        configSource.c_str(), _eepromBaseLocation, _configMaxDataLen);

                // Read the config JSON str from EEPROM
                readFromEEPROM();
            }
            else
            {
                // Can't make sense of config source
                RD_INFO("configLocation source %s unknown", configSource.c_str());
                return false;
            }

        }
        else
        {
            // Record source of config
            _configSource = CONFIG_SOURCE_STR;

            // Simply make a copy of the config string
            delete _pDataStrJSON;
            _pDataStrJSON = new char[strlen(configStr)+1];
            strcpy(_pDataStrJSON, configStr);

            // Debug
            RD_INFO("configLocation Source Passed-In-Str");

        }

        return true;
    }

    // Get a string from the JSON
    String getString (const char* dataPath,
                        const char* defaultValue, bool& isValid,
                        const char* pSourceStr = NULL)
    {
        if (pSourceStr == NULL)
            pSourceStr = _pDataStrJSON;
        jsmntok_t outToken;
        isValid = parseAndGetToken(pSourceStr, dataPath, outToken);
        if (!isValid)
            return defaultValue;
        char* pStr = strndup(pSourceStr + outToken.start,
                    outToken.end - outToken.start);
        String outStr(pStr);
        free(pStr);
        return outStr;
    }

    double getDouble (const char* dataPath,
                    double defaultValue, bool& isValid,
                    const char* pSourceStr = NULL)
    {
        if (pSourceStr == NULL)
            pSourceStr = _pDataStrJSON;
        jsmntok_t outToken;
        isValid = parseAndGetToken(pSourceStr, dataPath, outToken);
        if (!isValid)
            return defaultValue;
        return strtod(pSourceStr + outToken.start, NULL);
    }

    long getLong (const char* dataPath,
                    long defaultValue, bool& isValid,
                    const char* pSourceStr = NULL)
    {
        if (pSourceStr == NULL)
            pSourceStr = _pDataStrJSON;
        jsmntok_t outToken;
        isValid = parseAndGetToken(pSourceStr, dataPath, outToken);
        if (!isValid)
            return defaultValue;
        return strtol(pSourceStr + outToken.start, NULL, 10);
    }

private:
    // Data is stored in a single string as JSON
    char* _pDataStrJSON;

    // Source of config
    typedef enum CONFIG_SOURCE
    {
        CONFIG_SOURCE_STR,
        CONFIG_SOURCE_EEPROM
    };
    CONFIG_SOURCE _configSource = CONFIG_SOURCE_STR;

    // Base location to store dataStr in EEPROM
    int _eepromBaseLocation;

    // Max length of config data
    int _configMaxDataLen;

private:

    static bool parseAndGetToken(const char* jsonStr, const char* dataPath,
                jsmntok_t& outToken, int maxTokens = 200)
    {
        // Check for null source string
        if (jsonStr == NULL)
        {
            RD_ERR("Source JSON is NULL");
            return false;
        }

        // Extract json params
        jsmn_parser parser;
        // Max tokens can be overridden
        jsmntok_t* pTokens = new jsmntok_t[maxTokens];
        jsmn_init(&parser);
        int tokenCountRslt = jsmn_parse(&parser, jsonStr, strlen(jsonStr),
                    pTokens, maxTokens);
        if (tokenCountRslt < 0)
        {
            RD_ERR("Failed to parse JSON: %d", tokenCountRslt);
            delete pTokens;
            return false;
        }
        // Top level item must be an object
        if (tokenCountRslt < 1 || pTokens[0].type != JSMN_OBJECT)
        {
            RD_ERR("JSON must have top level object");
            delete pTokens;
            return false;
        }
        // Get required token
        int keyIdx = findKeyInJson(jsonStr, pTokens, tokenCountRslt,
                            dataPath, JSMN_STRING);
        if (keyIdx < 0)
        {
            RD_INFO("JSON cannot find key");
            delete pTokens;
            return false;
        }

        // Copy out token value
        outToken = pTokens[keyIdx+1];
        delete pTokens;
        return true;
    }

    static int findKeyInJson(const char* jsonOriginal, jsmntok_t tokens[],
                unsigned int numTokens, const char* reqdKey,
                jsmntype_t keyType = JSMN_UNDEFINED)
    {
        // Iterate over tokens to find key
        // Note that the root object has index 0
        for (int tokIdx = 1; tokIdx < numTokens; tokIdx++)
        {
            jsmntok_t* pTok = &tokens[tokIdx];
            if (((keyType == JSMN_UNDEFINED) || (pTok->type == keyType)) &&
                ((int) strlen(reqdKey) == pTok->end - pTok->start) &&
                (strncmp(jsonOriginal + pTok->start, reqdKey, pTok->end - pTok->start) == 0))
            {
                return tokIdx;
            }
        }
        return -1;
    }

    // Read a configuration string from EEPROM
    void readFromEEPROM()
    {
        // Check EEPROM has been initialised - if not just start with a null string
        if (EEPROM.read(_eepromBaseLocation) == 0xff)
        {
            delete _pDataStrJSON;
            _pDataStrJSON = NULL;
            RD_INFO("EEPROM uninitialised, _pDataStrJSON empty");
            return;
        }

        // Find out how long the string is - don't allow > _configMaxDataLen
        int dataStrLen = _configMaxDataLen-1;
        for (int chIdx = 0; chIdx < _configMaxDataLen; chIdx++)
        {
            if (EEPROM.read(_eepromBaseLocation+chIdx) == 0)
            {
                dataStrLen = chIdx;
                break;
            }
        }

        // Set initial size of string to avoid unnecessary resizing as we read it
        delete _pDataStrJSON;
        _pDataStrJSON = new char[dataStrLen+1];

        // Fill string from EEPROM location
        for (int chIdx = 0; chIdx < dataStrLen; chIdx++)
        {
            char ch = EEPROM.read(_eepromBaseLocation+chIdx);
            _pDataStrJSON[chIdx] = ch;
        }
        _pDataStrJSON[dataStrLen+1] = 0;

        RD_INFO("Read config str: %s", _pDataStrJSON);
    }

    // Write configuration string to EEPROM
    bool writeToEEPROM()
    {
        RD_DBG("Writing config str: %s", _pDataStrJSON);

        // Get length of string
        int dataStrLen = 0;
        if (_pDataStrJSON != NULL)
            dataStrLen = strlen(_pDataStrJSON);
        if (dataStrLen >= _configMaxDataLen)
            dataStrLen = _configMaxDataLen-1;

        // Write the current value of the string to EEPROM
        for (int chIdx = 0; chIdx < dataStrLen; chIdx++)
        {
            EEPROM.write(_eepromBaseLocation+chIdx, _pDataStrJSON[chIdx]);
        }

        // Terminate string
        EEPROM.write(_eepromBaseLocation+dataStrLen, 0);

        return true;
    }
};

#define RD_DEBUG_FNAME ""

#endif _CONFIG_MANAGER_H_
