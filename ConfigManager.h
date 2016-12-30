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
            // Extract json params
            jsmn_parser parser;
            // Only expect a few tokens
            jsmntok_t tokens[10];
        	jsmn_init(&parser);
        	int tokenCountRslt = jsmn_parse(&parser, configStr, strlen(configStr),
                        tokens, sizeof(tokens)/sizeof(tokens[0]));
            if (tokenCountRslt < 0)
            {
                RD_ERR("Failed to parse configLocation JSON: %d", tokenCountRslt);
        		return false;
        	}
            // Top level item must be an object
        	if (tokenCountRslt < 1 || tokens[0].type != JSMN_OBJECT)
            {
                RD_ERR("configLocation JSON must have top level object");
        		return false;
        	}
            // Get config settings
            bool isValid = false;
            String configSource = getStringFromTokens(configStr, tokens,
                                tokenCountRslt, "source", "", isValid);
            if (!isValid)
            {
                RD_ERR("configLocation source not found");
        		return false;
            }
            if (configSource.equalsIgnoreCase("EEPROM"))
            {
                long configPos = getLongFromTokens(configStr, tokens,
                                    tokenCountRslt, "base", 0, isValid);
                if (!isValid)
                {
                    RD_ERR("configLocation base not found");
            		return false;
                }

                long configMaxLen = getLongFromTokens(configStr, tokens,
                                    tokenCountRslt, "maxLen", 500, isValid);
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
    String getConfigString (const char* dataPath,
                            const char* defaultValue, bool& isValid)
    {
        jsmntok_t outToken;
        isValid = parseAndGetToken(_pDataStrJSON, dataPath, outToken);
        if (!isValid)
            return defaultValue;
        char* pStr = strndup(_pDataStrJSON + outToken.start,
                    outToken.end - outToken.start);
        String outStr(pStr);
        free(pStr);
        return outStr;
        // isValid = false;
        // return defaultValue;
    }

    double getConfigDouble (const char* dataPath, const char* nodeName,
                    double defaultValue, bool& isValid)
    {
        isValid = false;
        return defaultValue;
    }

    long getConfigLong (const char* dataPath, const char* nodeName,
                    long defaultValue, bool& isValid)
    {
        isValid = false;
        return defaultValue;
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

    // Get a string value
    static String getStringFromTokens(const char* jsonOriginal,
                jsmntok_t tokens[], unsigned int numTokens, const char* reqdKey,
    			const char* defaultValue, bool& isValid)
    {
        isValid = false;
        int keyIdx = findKeyInJson(jsonOriginal, tokens, numTokens,
                            reqdKey, JSMN_STRING);
        if (keyIdx >= 0)
        {
            isValid = true;
            char* pStr = strndup(jsonOriginal + tokens[keyIdx+1].start,
                        tokens[keyIdx+1].end - tokens[keyIdx+1].start);
            String outStr(pStr);
            free(pStr);
            return outStr;
        }
        return defaultValue;
    }

    // Get a long value
    static long getLongFromTokens(const char* jsonOriginal,
                jsmntok_t tokens[], unsigned int numTokens, const char* reqdKey,
    			long defaultValue, bool& isValid)
    {
        isValid = false;
        int keyIdx = findKeyInJson(jsonOriginal, tokens, numTokens,
                            reqdKey, JSMN_STRING);
        if (keyIdx >= 0)
        {
            isValid = true;
            long outVal = strtol(jsonOriginal + tokens[keyIdx+1].start,
                        NULL, 10);
            return outVal;
        }
        return defaultValue;
    }

    // Get a double value
    static double getDoubleFromTokens(const char* jsonOriginal,
                jsmntok_t tokens[], unsigned int numTokens, const char* reqdKey,
    			double defaultValue, bool& isValid)
    {
        isValid = false;
        int keyIdx = findKeyInJson(jsonOriginal, tokens, numTokens,
                            reqdKey, JSMN_STRING);
        if (keyIdx >= 0)
        {
            isValid = true;
            double outVal = strtod(jsonOriginal + tokens[keyIdx+1].start,
                        NULL);
            return outVal;
        }
        return defaultValue;
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
