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
            safeStringCopy(_pDataStrJSON, configStr, strlen(configStr));

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
        char* pStr = safeStringDup(pSourceStr + outToken.start,
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
                jsmntok_t& outToken, int maxTokens = 1000)
    {
        // Check for null source string
        if (jsonStr == NULL)
        {
            RD_ERR("Source JSON is NULL");
            return false;
        }

        // Find how many tokens in the string
        jsmn_parser parser;
        jsmn_init(&parser);
        int tokenCountRslt = jsmn_parse(&parser, jsonStr, strlen(jsonStr),
                    NULL, maxTokens);
        if (tokenCountRslt < 0)
        {
            RD_ERR("Failed to parse JSON: %d", tokenCountRslt);
            return false;
        }

        // Allocate space for tokens
        if (tokenCountRslt > maxTokens)
            tokenCountRslt = maxTokens;
        jsmntok_t* pTokens = new jsmntok_t[tokenCountRslt];

        // Parse again
        jsmn_init(&parser);
        tokenCountRslt = jsmn_parse(&parser, jsonStr, strlen(jsonStr),
                    pTokens, tokenCountRslt);
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

    static int findObjectEnd(jsmntok_t tokens[],
                unsigned int numTokens, int curTokenIdx,
                int count, bool atObjectKey = true)
    {
        int tokIdx = curTokenIdx;
        for (int objIdx = 0; objIdx < count; objIdx++)
        {
            jsmntok_t* pTok = tokens + tokIdx;
            if (pTok->type == JSMN_PRIMITIVE)
            {
                RD_DBG("findObjectEnd PRIMITIVE");
                tokIdx += 1;
            }
            else if (pTok->type == JSMN_STRING)
            {
                RD_DBG("findObjectEnd STRING");
				if (atObjectKey)
				{
					tokIdx = findObjectEnd(tokens, numTokens, tokIdx + 1, pTok->size, false);
				}
				else
				{
					tokIdx += 1;
				}
            }
            else if (pTok->type == JSMN_OBJECT)
            {
                RD_DBG("findObjectEnd OBJECT");
                tokIdx = findObjectEnd(tokens, numTokens, tokIdx+1, pTok->size, true);
            }
            else if (pTok->type == JSMN_ARRAY)
            {
                RD_DBG("findObjectEnd ARRAY");
                tokIdx = findObjectEnd(tokens, numTokens, tokIdx+1, pTok->size, false);
			}
			else
			{
				RD_DBG("findObjectEnd UNKNOWN!!!!!!!");
				tokIdx += 1;
			}
            if (tokIdx >= numTokens)
            {
                break;
            }
        }
        RD_DBG("findObjectEnd returning %d", tokIdx);
        return tokIdx;
    }

    static int findKeyInJson(const char *jsonOriginal, jsmntok_t tokens[],
                             unsigned int numTokens, const char *dataPath,
                             jsmntype_t keyType = JSMN_UNDEFINED)
    {
        const int  MAX_SRCH_KEY_LEN = 100;
        char       srchKey[MAX_SRCH_KEY_LEN + 1];
        const char *pDataPathPos = dataPath;
        // Note that the root object has index 0
        int  curTokenIdx = 1;
        int  maxTokenIdx = numTokens;
        bool atNodeLevel = false;

        // Go through the parts of the path to find the whole
        while (pDataPathPos < dataPath + strlen(dataPath))
        {
            // Get the next part of the path
            const char *slashPos = strstr(pDataPathPos, "/");
            if (slashPos == NULL)
            {
                safeStringCopy(srchKey, pDataPathPos, MAX_SRCH_KEY_LEN);
                pDataPathPos += strlen(pDataPathPos);
                atNodeLevel   = true;
            }
            else if (slashPos - pDataPathPos >= MAX_SRCH_KEY_LEN)
            {
                safeStringCopy(srchKey, pDataPathPos, MAX_SRCH_KEY_LEN);
                pDataPathPos = slashPos + 1;
            }
            else
            {
                safeStringCopy(srchKey, pDataPathPos, slashPos - pDataPathPos);
                pDataPathPos = slashPos + 1;
            }
            RD_DBG("SlashPos %d, %d", slashPos-pDataPathPos, atNodeLevel);
            RD_DBG("findKeyInJson srchKey %s", srchKey);


            // Iterate over tokens to find key of the right type
            // If we are already looking at the node level then search for requested type
            // Otherwise search for and object that will contain the next level key
            jsmntype_t keyTypeToFind = atNodeLevel ? keyType : JSMN_STRING;
            for (int tokIdx = curTokenIdx; tokIdx < maxTokenIdx; )
            {
                jsmntok_t *pTok = tokens + tokIdx;
                if ((pTok->type == JSMN_STRING) &&
                    ((int)strlen(srchKey) == pTok->end - pTok->start) &&
                    (strncmp(jsonOriginal + pTok->start, srchKey, pTok->end - pTok->start) == 0))
                {
                    if (atNodeLevel)
                    {
                        if ((keyTypeToFind == JSMN_UNDEFINED) || (tokens[tokIdx + 1].type == keyTypeToFind))
                        {
                            return tokIdx;
                        }
                        return -1;
                    }
                    else
                    {
                        // Check for an object
                        RD_DBG("findObjectEnd inside");
                        if (tokens[tokIdx + 1].type == JSMN_OBJECT)
                        {
                            // Continue next level of search in this object
                            maxTokenIdx = findObjectEnd(tokens, numTokens, tokIdx, 1);
                            curTokenIdx = tokIdx + 2;
                            break;
                        }
                        else
                        {
                            // Found a key in the path but it didn't point to an object so we can't continue
                            return -1;
                        }
                    }
                }
                else if (pTok->type == JSMN_STRING)
                {
                    // We're at a key string but it isn't the one we want to skip its contents
                    tokIdx = findObjectEnd(tokens, numTokens, tokIdx, 1);
                }
                else
                {
                    // Shouldn't really get here as all keys are strings
                    tokIdx++;
                }
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

	static void safeStringCopy(char* pDest, const char* pSrc, size_t maxx)
	{
		char* pD = pDest;
		const char* pS = pSrc;
		size_t toCopy = strlen(pS);
		for (size_t i = 0; i < toCopy + 1; i++)
		{
			*pD++ = *pS++;
			if (i >= maxx - 1)
			{
				*pD = 0;
				break;
			}
		}
	}

	static char* safeStringDup(const char* pSrc, size_t maxx)
	{
		size_t toCopy = strlen(pSrc);
		char* pDest = new char[toCopy+1];
		char* pD = pDest;
		const char* pS = pSrc;
		for (size_t i = 0; i < toCopy + 1; i++)
		{
			*pD++ = *pS++;
			if (i >= maxx - 1)
			{
				*pD = 0;
				break;
			}
		}
		return pDest;
	}
};

#define RD_DEBUG_FNAME ""

#endif _CONFIG_MANAGER_H_
