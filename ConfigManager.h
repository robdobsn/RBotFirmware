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

    ~ConfigManager()
    {
        delete[] _pDataStrJSON;
    }

    // Initialise
    bool SetConfigLocation(const char* configStr, bool configStrDefinesLocation = true)
    {
        // Check if the configStr is the actual config or if it defines the
        // location of config
        if (configStrDefinesLocation)
        {
            bool isValid = false;
            jsmntype_t objType = JSMN_UNDEFINED;
            int objSize = 0;
            String configSource = getString("source", "", isValid, objType,
                            objSize, configStr);
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

            // Set config data
            setConfigData(configStr);

            // Debug
            RD_INFO("configLocation Source Passed-In-Str");
        }

        return true;
    }

    // Set the configuration data directly
    void setConfigData(const char* configJSONStr)
    {
        // Simply make a copy of the config string
        delete[] _pDataStrJSON;
        _pDataStrJSON = new char[safeStringLen(configJSONStr)+1];
        safeStringCopy(_pDataStrJSON, configJSONStr,
                strlen(configJSONStr));
    }

    // Get a string from the JSON
    String getString (const char* dataPath,
                        const char* defaultValue, bool& isValid,
                        jsmntype_t& objType, int& objSize,
                        const char* pSourceStr = NULL)
    {
        // Get source string
        if (pSourceStr == NULL)
            pSourceStr = _pDataStrJSON;

        // Parse json into tokens
        int numTokens = 0;
        jsmntok_t* pTokens = parseJson(pSourceStr, numTokens);
        if (pTokens == NULL)
            return defaultValue;

        // Find token
        int startTokenIdx, endTokenIdx;
        isValid = getTokenByDataPath(pSourceStr, dataPath,
                        pTokens, numTokens, startTokenIdx, endTokenIdx);
        if (!isValid)
        {
            delete[] pTokens;
            return defaultValue;
        }

        // Extract as a string
        objType = pTokens[startTokenIdx].type;
        objSize = pTokens[startTokenIdx].size;
        String outStr;
        if (objType == JSMN_STRING || objType == JSMN_PRIMITIVE)
        {
            char* pStr = safeStringDup(pSourceStr + pTokens[startTokenIdx].start,
                        pTokens[startTokenIdx].end - pTokens[startTokenIdx].start);
            outStr = pStr;
            delete[] pStr;
            objSize = outStr.length();
        }
        else
        {
//            recreateJson(pSourceStr, pTokens + startTokenIdx, 1, 0, outStr);
            char* pStr = safeStringDup(pSourceStr + pTokens[startTokenIdx].start,
                        pTokens[startTokenIdx].end - pTokens[startTokenIdx].start);
            outStr = pStr;
            delete[] pStr;
        }

        // Tidy up
        delete[] pTokens;
        return outStr;
    }

    double getDouble (const char* dataPath,
                    double defaultValue, bool& isValid,
                    const char* pSourceStr = NULL)
    {
        // Get source string
        if (pSourceStr == NULL)
            pSourceStr = _pDataStrJSON;

        // Parse json into tokens
        int numTokens = 0;
        jsmntok_t* pTokens = parseJson(pSourceStr, numTokens);
        if (pTokens == NULL)
            return defaultValue;

        // Find token
        int startTokenIdx, endTokenIdx;
        isValid = getTokenByDataPath(pSourceStr, dataPath,
                        pTokens, numTokens, startTokenIdx, endTokenIdx);
        if (!isValid)
        {
            delete[] pTokens;
            return defaultValue;
        }

        // Tidy up
        delete[] pTokens;
        return strtod(pSourceStr + pTokens[startTokenIdx].start, NULL);
    }

    long getLong (const char* dataPath,
                    long defaultValue, bool& isValid,
                    const char* pSourceStr = NULL)
    {
        // Get source string
        if (pSourceStr == NULL)
            pSourceStr = _pDataStrJSON;

        // Parse json into tokens
        int numTokens = 0;
        jsmntok_t* pTokens = parseJson(pSourceStr, numTokens);
        if (pTokens == NULL)
            return defaultValue;

        // Find token
        int startTokenIdx, endTokenIdx;
        isValid = getTokenByDataPath(pSourceStr, dataPath,
                        pTokens, numTokens, startTokenIdx, endTokenIdx);
        if (!isValid)
        {
            delete[] pTokens;
            return defaultValue;
        }

        // Tidy up
        delete[] pTokens;
        return strtol(pSourceStr + pTokens[startTokenIdx].start, NULL, 10);
    }

    static int recreateJson(const char *js, jsmntok_t *t, size_t count, int indent, String& outStr)
    {
        int i, j, k;

        if (count == 0)
        {
            return 0;
        }
        if (t->type == JSMN_PRIMITIVE)
        {
            char *pStr = safeStringDup(js + t->start,
                                       t->end - t->start);
            outStr.concat(pStr);
            delete[] pStr;
            return 1;
        }
        else if (t->type == JSMN_STRING)
        {
            char *pStr = safeStringDup(js + t->start,
                                       t->end - t->start);
            outStr.concat("\"");
            outStr.concat(pStr);
            outStr.concat("\"");
            delete[] pStr;
            return 1;
        }
        else if (t->type == JSMN_OBJECT)
        {
            // Serial.printf("\n\r#Found object size %d, start %d, end %d, str %.*s\n\r",
            //               t->size, t->start, t->end, t->end - t->start, js + t->start);
            j = 0;
            outStr.concat("{");
            for (i = 0; i < t->size; i++)
            {
                j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
                outStr.concat(":");
                j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
                if (i != t->size - 1)
                {
                    outStr.concat(",");
                }
            }
            outStr.concat("}");
            return j + 1;
        }
        else if (t->type == JSMN_ARRAY)
        {
            j = 0;
            outStr.concat("[");
            for (i = 0; i < t->size; i++)
            {
                j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
                if (i != t->size - 1)
                {
                    outStr.concat(",");
                }
            }
            outStr.concat("]");
            return j + 1;
        }
        return 0;
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

    static jsmntok_t* parseJson(const char* jsonStr, int& numTokens,
                int maxTokens = 10000)
    {
        // Check for null source string
        if (jsonStr == NULL)
        {
            RD_ERR("Source JSON is NULL");
            return NULL;
        }

        // Find how many tokens in the string
        jsmn_parser parser;
        jsmn_init(&parser);
        int tokenCountRslt = jsmn_parse(&parser, jsonStr, strlen(jsonStr),
                    NULL, maxTokens);
        if (tokenCountRslt < 0)
        {
            RD_ERR("Failed to parse JSON: %d", tokenCountRslt);
            return NULL;
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
            delete[] pTokens;
            return NULL;
        }
        numTokens = tokenCountRslt;
        return pTokens;
    }

    static bool getTokenByDataPath(const char* jsonStr, const char* dataPath,
                jsmntok_t* pTokens, int numTokens,
                int& startTokenIdx, int& endTokenIdx)
    {
        // // Top level item must be an object
        // if (tokenCountRslt < 1 || pTokens[0].type != JSMN_OBJECT)
        // {
        //     RD_ERR("JSON must have top level object");
        //     delete[] pTokens;
        //     return false;
        // }
        // Get required token
        int keyIdx = findKeyInJson(jsonStr, pTokens, numTokens,
                            dataPath, endTokenIdx);
        if (keyIdx < 0)
        {
            RD_INFO("JSON cannot find key");
            return false;
        }

        // Copy out token value
        startTokenIdx = keyIdx+1;
        return true;
    }

    static int findObjectEnd(const char *jsonOriginal, jsmntok_t tokens[],
                unsigned int numTokens, int curTokenIdx,
                int count, bool atObjectKey = true)
    {
        // RD_DBG("findObjectEnd idx %d, count %d, start %s", curTokenIdx, count,
        //                 jsonOriginal + tokens[curTokenIdx].start);
        int tokIdx = curTokenIdx;
        for (int objIdx = 0; objIdx < count; objIdx++)
        {
            jsmntok_t* pTok = tokens + tokIdx;
            if (pTok->type == JSMN_PRIMITIVE)
            {
                // RD_DBG("findObjectEnd PRIMITIVE");
                tokIdx += 1;
            }
            else if (pTok->type == JSMN_STRING)
            {
                // RD_DBG("findObjectEnd STRING");
				if (atObjectKey)
				{
					tokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx + 1, pTok->size, false);
                    tokIdx += 1;
				}
				else
				{
					tokIdx += 1;
				}
            }
            else if (pTok->type == JSMN_OBJECT)
            {
                // RD_DBG("findObjectEnd OBJECT");
                tokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx+1, pTok->size, true);
                tokIdx += 1;
            }
            else if (pTok->type == JSMN_ARRAY)
            {
                // RD_DBG("findObjectEnd ARRAY");
                tokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx+1, pTok->size, false);
                tokIdx += 1;
			}
			else
			{
				RD_DBG("findObjectEnd UNKNOWN!!!!!!! %d", pTok->type);
				tokIdx += 1;
			}
            if (tokIdx >= numTokens)
            {
                break;
            }
        }
        tokIdx--;
        // RD_DBG("findObjectEnd returning %d, start %s, end %s", tokIdx,
        //                 jsonOriginal + tokens[tokIdx].start,
        //             jsonOriginal + tokens[tokIdx].end);
        return tokIdx;
    }

    static int findKeyInJson(const char *jsonOriginal, jsmntok_t tokens[],
                             unsigned int numTokens, const char *dataPath,
                             int& endTokenIdx,
                             jsmntype_t keyType = JSMN_UNDEFINED)
    {
        const int  MAX_SRCH_KEY_LEN = 100;
        char       srchKey[MAX_SRCH_KEY_LEN + 1];
        const char *pDataPathPos = dataPath;
        // Note that the root object has index 0
        int  curTokenIdx = 1;
        int  maxTokenIdx = numTokens - 1;
        bool atNodeLevel = false;

        // Go through the parts of the path to find the whole
        while (pDataPathPos < dataPath + strlen(dataPath))
        {
            // Get the next part of the path
            const char *slashPos = strstr(pDataPathPos, "/");
            // RD_DBG("SlashPos %d, %d", slashPos, slashPos-pDataPathPos);
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
            // RD_DBG("findKeyInJson srchKey %s", srchKey);


            // Iterate over tokens to find key of the right type
            // If we are already looking at the node level then search for requested type
            // Otherwise search for and object that will contain the next level key
            jsmntype_t keyTypeToFind = atNodeLevel ? keyType : JSMN_STRING;
            for (int tokIdx = curTokenIdx; tokIdx <= maxTokenIdx; )
            {
                jsmntok_t *pTok = tokens + tokIdx;
                if ((pTok->type == JSMN_STRING) &&
                    ((int)strlen(srchKey) == pTok->end - pTok->start) &&
                    (strncmp(jsonOriginal + pTok->start, srchKey, pTok->end - pTok->start) == 0))
                {
                    if (atNodeLevel)
                    {
                        // RD_DBG("findObjectEnd we have got it %d", tokIdx);
                        if ((keyTypeToFind == JSMN_UNDEFINED) || (tokens[tokIdx + 1].type == keyTypeToFind))
                        {
                            endTokenIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx, 1);
                            return tokIdx;
                        }
                        return -1;
                    }
                    else
                    {
                        // Check for an object
                        // RD_DBG("findObjectEnd inside");
                        if (tokens[tokIdx + 1].type == JSMN_OBJECT)
                        {
                            // Continue next level of search in this object
                            maxTokenIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx, 1);
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
                    // We're at a key string but it isn't the one we want so skip its contents
                    tokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx, 1) + 1;
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

    static size_t safeStringLen(const char* pSrc, bool skipWhitespace = false)
    {
        const char* pS = pSrc;
		size_t toCount = strlen(pS);
        int stringLen = 0;
		for (size_t i = 0; i < toCount + 1; i++)
		{
            char ch = *pS++;
            if (skipWhitespace && isspace(ch))
                continue;
			stringLen++;
		}
        return stringLen;
    }

	static void safeStringCopy(char* pDest, const char* pSrc,
                size_t maxx, bool removeWhitespace = false)
	{
		char* pD = pDest;
		const char* pS = pSrc;
		size_t maxToCopy = strlen(pS);
        int stringLen = 0;
		for (size_t i = 0; i < maxToCopy + 1; i++)
		{
            char ch = *pS++;
            if (removeWhitespace && isspace(ch))
                continue;
			*pD++ = ch;
            stringLen++;
			if (stringLen >= maxx)
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
