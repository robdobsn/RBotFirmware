// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "jsmnParticleR.h"

#pragma push_macro("RD_DEBUG_FNAME")
#define RD_DEBUG_FNAME "ConfigManager.h"
#include "RdDebugLevel.h"

class ConfigManager
{
private:
	// Data is stored in a single string as JSON
	char* _pDataStrJSON;

public:
    ConfigManager()
    {
        _pDataStrJSON = NULL;
    }

    ~ConfigManager()
    {
        delete[] _pDataStrJSON;
    }

	// Get config data string
	const char* getConfigData()
	{
		if (!_pDataStrJSON)
			return "";
		return _pDataStrJSON;
	}

    // Set the configuration data directly
    void setConfigData(const char* configJSONStr)
    {
        // Simply make a copy of the config string
        delete[] _pDataStrJSON;
        int stringLength = safeStringLen(configJSONStr, true);
        _pDataStrJSON = new char[stringLength+1];
        safeStringCopy(_pDataStrJSON, configJSONStr,
                stringLength, true);
    }

    // Get a string from the JSON
    static String getString (const char* dataPath,
                        const char* defaultValue, bool& isValid,
                        jsmntype_t& objType, int& objSize,
                        const char* pSourceStr)
    {
		// Check for null
		if (!pSourceStr)
			return defaultValue;

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
                        pTokens[startTokenIdx].end - pTokens[startTokenIdx].start,
                        true);
            outStr = pStr;
            delete[] pStr;
            objSize = outStr.length();
        }
        else
        {
            //recreateJson(pSourceStr, pTokens + startTokenIdx, objSize, 0, outStr);
            char* pStr = safeStringDup(pSourceStr + pTokens[startTokenIdx].start,
                        pTokens[startTokenIdx].end - pTokens[startTokenIdx].start,
                        true);
            outStr = pStr;
            delete[] pStr;
        }

        // Tidy up
        delete[] pTokens;
        return outStr;
    }

	static String getString (const char* dataPath, const char* defaultValue,
				const char* pSourceStr)
	{
		bool isValid = false;
		jsmntype_t objType = JSMN_UNDEFINED;
		int objSize = 0;
		return getString(dataPath, defaultValue, isValid, objType, objSize,
					pSourceStr);
	}

	// Get a string from the JSON
    String getString (const char* dataPath,
                        const char* defaultValue, bool& isValid,
                        jsmntype_t& objType, int& objSize)
    {
		return getString(dataPath, defaultValue, isValid, objType, objSize,
					_pDataStrJSON);
	}

    static double getDouble (const char* dataPath,
                    double defaultValue, bool& isValid,
                    const char* pSourceStr)
    {
		// Check for null
		if (!pSourceStr)
			return defaultValue;

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

	static double getDouble (const char* dataPath, double defaultValue,
				const char* pSourceStr)
	{
		bool isValid = false;
		return getDouble(dataPath, defaultValue, isValid, pSourceStr);
	}

	double getDouble (const char* dataPath, double defaultValue)
	{
		bool isValid = false;
		return getDouble(dataPath, defaultValue, isValid, _pDataStrJSON);
	}

    static long getLong (const char* dataPath,
                    long defaultValue, bool& isValid,
                    const char* pSourceStr)
    {
		// Check for null
		if (!pSourceStr)
			return defaultValue;

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

	static long getLong (const char* dataPath, long defaultValue, const char* pSourceStr)
	{
		bool isValid = false;
		return getLong(dataPath, defaultValue, isValid, pSourceStr);
	}

	long getLong (const char* dataPath,	long defaultValue)
	{
		bool isValid = false;
		return getLong(dataPath, defaultValue, isValid, _pDataStrJSON);
	}

	static const char* getObjType(jsmntype_t type)
	{
		switch (type)
		{
		case JSMN_PRIMITIVE: return "PRIMITIVE";
		case JSMN_STRING: return "STRING";
		case JSMN_OBJECT: return "OBJECT";
		case JSMN_ARRAY: return "ARRAY";
		case JSMN_UNDEFINED: return "UNDEFINED";
		}
		return "UNKNOWN";
	}

public:
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

private:
    static bool getTokenByDataPath(const char* jsonStr, const char* dataPath,
                jsmntok_t* pTokens, int numTokens,
                int& startTokenIdx, int& endTokenIdx)
    {
        // Get required token
        int keyIdx = findKeyInJson(jsonStr, pTokens, numTokens,
                            dataPath, endTokenIdx);
        if (keyIdx < 0)
        {
            RD_INFO("JSON cannot find key");
            return false;
        }

        // Copy out token value
        startTokenIdx = keyIdx;
        return true;
    }

    static int findObjectEnd(const char *jsonOriginal, jsmntok_t tokens[],
                unsigned int numTokens, int curTokenIdx,
                int count, bool atObjectKey = true)
    {
        // RD_DBG("findObjectEnd idx %d, count %d, start %s", curTokenIdx, count,
        //                 jsonOriginal + tokens[curTokenIdx].start);
		// Primitives have a size of 0 but we still need to skip over them ...
		if (count == 0)
			count = 1;
        unsigned int tokIdx = curTokenIdx;
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
					tokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx + 1, 1, false);
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
            }
            else if (pTok->type == JSMN_ARRAY)
            {
                // RD_DBG("findObjectEnd ARRAY");
                tokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx+1, pTok->size, false);
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
        int  curTokenIdx = 0;
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

            // See if search key contains an array reference
            bool arrayElementReqd = false;
            int reqdArrayIdx = 0;
            char *sqBracketPos = strstr(srchKey, "[");
            if (sqBracketPos != NULL)
            {
                // Extract array index
                long arrayIdx = strtol(sqBracketPos+1, NULL, 10);
                if (arrayIdx >= 0)
                {
                    arrayElementReqd = true;
                    reqdArrayIdx = (int)arrayIdx;
                }
                // Truncate the string at the square bracket
                *sqBracketPos = 0;
            }

            // RD_DBG("findKeyInJson srchKey %s", srchKey);

            // Iterate over tokens to find key of the right type
            // If we are already looking at the node level then search for requested type
            // Otherwise search for and object that will contain the next level key
            jsmntype_t keyTypeToFind = atNodeLevel ? keyType : JSMN_STRING;
            for (int tokIdx = curTokenIdx; tokIdx <= maxTokenIdx; )
            {
				// See if the key matches - this can either be a string match on an object key or
				// just an array element match (with an empty key)
                jsmntok_t *pTok = tokens + tokIdx;
				bool keyMatchFound = false;
				if ( (pTok->type == JSMN_STRING) &&
					((int)strlen(srchKey) == pTok->end - pTok->start) &&
					(strncmp(jsonOriginal + pTok->start, srchKey, pTok->end - pTok->start) == 0) )
				{
					keyMatchFound = true;
					tokIdx += 1;
				}
				else if ((pTok->type == JSMN_ARRAY) &&
					((int)strlen(srchKey) == 0) &&
					(arrayElementReqd))
				{
					keyMatchFound = true;
				}

				if (keyMatchFound)
				{
                    // We have found the matching key so now for the contents ...

                    // Check if we were looking for an array element
                    if (arrayElementReqd)
                    {
                        if (tokens[tokIdx].type == JSMN_ARRAY)
                        {
                            int newTokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx+1, reqdArrayIdx, false);
                            RD_DBG("TokIdxArray inIdx %d, reqdArrayIdx %d, outTokIdx %d",
                                            tokIdx, reqdArrayIdx, newTokIdx);
                            tokIdx = newTokIdx;
                        }
                        else
                        {
                            // This isn't an array element
                            return -1;
                        }
                    }

					// atNodeLevel indicates that we are now at the level of the JSON tree that the user requested
					// - so we should be extracting the value referenced now
                    if (atNodeLevel)
                    {
                        // RD_DBG("findObjectEnd we have got it %d", tokIdx);
                        if ((keyTypeToFind == JSMN_UNDEFINED) || (tokens[tokIdx].type == keyTypeToFind))
                        {
                            endTokenIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx, 1, false);
                            //int testTokenIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx+1, 1);
                            //RD_DBG("TokIdxDiff max %d, test %d, diff %d", endTokenIdx, testTokenIdx, testTokenIdx-endTokenIdx);
                            return tokIdx;
                        }
                        return -1;
                    }
                    else
                    {
                        // Check for an object
                        // RD_DBG("findObjectEnd inside");
                        if (tokens[tokIdx].type == JSMN_OBJECT)
                        {
                            // Continue next level of search in this object
                            maxTokenIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx, 1);
                            //int testTokenIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx+1, 1);
                            //RD_DBG("TokIdxDiff2 max %d, test %d, diff %d", maxTokenIdx, testTokenIdx, testTokenIdx- maxTokenIdx);
                            curTokenIdx = tokIdx + 1;
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
                    tokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx, 1);
                }
				else if (pTok->type == JSMN_OBJECT)
				{
					// Move to the first key of the object
					tokIdx++;
				}
				else if (pTok->type == JSMN_ARRAY)
				{
					// Root level array which doesn't match the dataPath
					return -1;
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

public:
	static size_t safeStringLen(const char* pSrc,
		bool skipJSONWhitespace = false, size_t maxx = LONG_MAX)
	{
		const char* pS = pSrc;
		int stringLen = 0;
		bool insideDoubleQuotes = false;
		bool insideSingleQuotes = false;
		size_t charsTakenFromSrc = 0;
		while (*pS)
		{
			char ch = *pS++;
			charsTakenFromSrc++;
			if ((ch == '\'') && !insideDoubleQuotes)
				insideSingleQuotes = !insideSingleQuotes;
			else if ((ch == '\"') && !insideSingleQuotes)
				insideDoubleQuotes = !insideDoubleQuotes;
			else if (!insideDoubleQuotes && !insideSingleQuotes &&
				skipJSONWhitespace && isspace(ch))
				continue;
			stringLen++;
			if (maxx != LONG_MAX && charsTakenFromSrc >= maxx)
				return stringLen;
		}
		return stringLen;
	}

	static void safeStringCopy(char* pDest, const char* pSrc,
		size_t maxx, bool skipJSONWhitespace = false)
	{
		char* pD = pDest;
		const char* pS = pSrc;
		size_t srcStrlen = strlen(pS);
		size_t stringLen = 0;
		bool insideDoubleQuotes = false;
		bool insideSingleQuotes = false;
		for (size_t i = 0; i < srcStrlen + 1; i++)
		{
			char ch = *pS++;
			if ((ch == '\'') && !insideDoubleQuotes)
				insideSingleQuotes = !insideSingleQuotes;
			else if ((ch == '\"') && !insideSingleQuotes)
				insideDoubleQuotes = !insideDoubleQuotes;
			else if (!insideDoubleQuotes && !insideSingleQuotes &&
				skipJSONWhitespace && isspace(ch))
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

	static char* safeStringDup(const char* pSrc, size_t maxx,
		bool skipJSONWhitespace = false)
	{
		size_t toAlloc = safeStringLen(pSrc, skipJSONWhitespace, maxx);
		char* pDest = new char[toAlloc + 1];
		char* pD = pDest;
		const char* pS = pSrc;
		size_t srcStrlen = strlen(pS);
		size_t stringLen = 0;
		bool insideDoubleQuotes = false;
		bool insideSingleQuotes = false;
		for (size_t i = 0; i < srcStrlen + 1; i++)
		{
			char ch = *pS++;
			if ((ch == '\'') && !insideDoubleQuotes)
				insideSingleQuotes = !insideSingleQuotes;
			else if ((ch == '\"') && !insideSingleQuotes)
				insideDoubleQuotes = !insideDoubleQuotes;
			else if (!insideDoubleQuotes && !insideSingleQuotes &&
				skipJSONWhitespace && isspace(ch))
				continue;
			*pD++ = ch;
			stringLen++;
			if (stringLen >= maxx || stringLen >= toAlloc)
			{
				*pD = 0;
				break;
			}
		}
		return pDest;
	}

#ifdef CONFIG_MANAGER_RECREATE_JSON
	static int recreateJson(const char *js, jsmntok_t *t,
		size_t count, int indent, String& outStr)
	{
		int i, j, k;

		if (count == 0)
		{
			return 0;
		}
		if (t->type == JSMN_PRIMITIVE)
		{
			RD_DBG("\n\r#Found primitive size %d, start %d, end %d\n\r",
				t->size, t->start, t->end);
			RD_DBG("%.*s", t->end - t->start, js + t->start);
			char *pStr = safeStringDup(js + t->start,
				t->end - t->start);
			outStr.concat(pStr);
			delete[] pStr;
			return 1;
		}
		else if (t->type == JSMN_STRING)
		{
			RD_DBG("\n\r#Found string size %d, start %d, end %d\n\r",
				t->size, t->start, t->end);
			RD_DBG("'%.*s'", t->end - t->start, js + t->start);
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
			RD_DBG("\n\r#Found object size %d, start %d, end %d\n\r",
				t->size, t->start, t->end);
			j = 0;
			outStr.concat("{");
			for (i = 0; i < t->size; i++)
			{
				for (k = 0; k < indent; k++)
				{
					RD_DBG("  ");
				}
				j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
				outStr.concat(":");
				RD_DBG(": ");
				j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
				RD_DBG("\n\r");
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
			RD_DBG("\n\r#Found array size %d, start %d, end %d\n\r",
				t->size, t->start, t->end);
			j = 0;
			outStr.concat("[");
			RD_DBG("\n\r");
			for (i = 0; i < t->size; i++)
			{
				for (k = 0; k < indent - 1; k++)
				{
					RD_DBG("  ");
				}
				RD_DBG("   - ");
				j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
				if (i != t->size - 1)
				{
					outStr.concat(",");
				}
				RD_DBG("\n\r");
			}
			outStr.concat("]");
			return j + 1;
		}
		return 0;
	}

	static bool doPrint(const char* jsonStr)
	{
		jsmn_parser parser;
		jsmn_init(&parser);
		int tokenCountRslt = jsmn_parse(&parser, jsonStr, strlen(jsonStr),
			NULL, 1000);
		if (tokenCountRslt < 0)
		{
			RD_ERR("Failed to parse JSON: %d", tokenCountRslt);
			return false;
		}
		jsmntok_t* pTokens = new jsmntok_t[tokenCountRslt];
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
		RD_DBG("Dumping");
		recreateJson(jsonStr, pTokens, parser.toknext, 0);
		delete pTokens;
		return true;
	}

#endif // CONFIG_MANAGER_RECREATE_JSON

};

#pragma pop_macro("RD_DEBUG_FNAME")
