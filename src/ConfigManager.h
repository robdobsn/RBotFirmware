// RBotFirmware
// Rob Dobson 2016

// Many of the methods here support a dataPath parameter. This uses a syntax like a much simplified XPath:
// [0] returns the 0th element of an array
// / is a separator of nodes

#pragma once

#include "jsmnParticleR.h"
#include "Utils.h"

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
                        jsmnrtype_t& objType, int& objSize,
                        const char* pSourceStr)
    {
		// Check for null
		if (!pSourceStr)
			return defaultValue;

        // Parse json into tokens
        int numTokens = 0;
        jsmnrtok_t* pTokens = parseJson(pSourceStr, numTokens);
        if (pTokens == NULL)
        {
            // Parse failed
            Utils::logLongStr("Failed to parse JSON", pSourceStr);
            return defaultValue;
        }

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
        if (objType == JSMNR_STRING || objType == JSMNR_PRIMITIVE)
        {
            char* pStr = safeStringDup(pSourceStr + pTokens[startTokenIdx].start,
                        pTokens[startTokenIdx].end - pTokens[startTokenIdx].start,
                        false);
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
				const char* pSourceStr, bool& isValid)
	{
		jsmnrtype_t objType = JSMNR_UNDEFINED;
		int objSize = 0;
		return getString(dataPath, defaultValue, isValid, objType, objSize,
					pSourceStr);
	}

	static String getString (const char* dataPath, const char* defaultValue,
				const char* pSourceStr)
	{
		bool isValid = false;
		jsmnrtype_t objType = JSMNR_UNDEFINED;
		int objSize = 0;
		return getString(dataPath, defaultValue, isValid, objType, objSize,
					pSourceStr);
	}

	// Get a string from the JSON
    String getString (const char* dataPath,
                        const char* defaultValue, bool& isValid,
                        jsmnrtype_t& objType, int& objSize)
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
        jsmnrtok_t* pTokens = parseJson(pSourceStr, numTokens);
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
        jsmnrtok_t* pTokens = parseJson(pSourceStr, numTokens);
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

	static const char* getObjTypeStr(jsmnrtype_t type)
	{
		switch (type)
		{
		case JSMNR_PRIMITIVE: return "PRIMITIVE";
		case JSMNR_STRING: return "STRING";
		case JSMNR_OBJECT: return "OBJECT";
		case JSMNR_ARRAY: return "ARRAY";
		case JSMNR_UNDEFINED: return "UNDEFINED";
		}
		return "UNKNOWN";
	}

    static const jsmnrtype_t getType(int& arrayLen, const char* pSourceStr)
    {
        // Check for null
		if (!pSourceStr)
			return JSMNR_UNDEFINED;

        // Parse json into tokens
        int numTokens = 0;
        jsmnrtok_t* pTokens = parseJson(pSourceStr, numTokens);
        if (pTokens == NULL)
            return JSMNR_UNDEFINED;

        // Get the type of the first token
        arrayLen = pTokens->size;
        return pTokens->type;
    }

public:
    static jsmnrtok_t* parseJson(const char* jsonStr, int& numTokens,
                int maxTokens = 10000)
    {
        // Check for null source string
        if (jsonStr == NULL)
        {
            Log.error("ConfigMgr: Source JSON is NULL");
            return NULL;
        }

        // Find how many tokens in the string
        JSMNR_parser parser;
        JSMNR_init(&parser);
        int tokenCountRslt = JSMNR_parse(&parser, jsonStr, strlen(jsonStr),
                    NULL, maxTokens);
        if (tokenCountRslt < 0)
        {
            Log.trace("ConfigMgr: parseJson result %d maxTokens %d jsonLen %d", tokenCountRslt, maxTokens, strlen(jsonStr));
            Utils::logLongStr("ConfigMgr: jsonStr", jsonStr);
            return NULL;
        }

        // Allocate space for tokens
        if (tokenCountRslt > maxTokens)
            tokenCountRslt = maxTokens;
        jsmnrtok_t* pTokens = new jsmnrtok_t[tokenCountRslt];

        // Parse again
        JSMNR_init(&parser);
        tokenCountRslt = JSMNR_parse(&parser, jsonStr, strlen(jsonStr),
                    pTokens, tokenCountRslt);
        if (tokenCountRslt < 0)
        {
            Log.info("parseJson result: %d", tokenCountRslt);
            Log.trace("jsonStr %s numTok %d maxTok %d", jsonStr, numTokens, maxTokens);
            delete[] pTokens;
            return NULL;
        }
        numTokens = tokenCountRslt;
        return pTokens;
    }

    static void escapeString(String& strToEsc)
    {
        // Replace characters which are invalid in JSON
        strToEsc.replace("\\", "\\\\");
        strToEsc.replace("\"", "\\\"");
        strToEsc.replace("\n", "\\n");
    }

    static void unescapeString(String& strToUnEsc)
    {
        // Replace characters which are invalid in JSON
        strToUnEsc.replace("\\\"", "\"");
        strToUnEsc.replace("\\\\", "\\");
        strToUnEsc.replace("\\n", "\n");
    }

private:
    static bool getTokenByDataPath(const char* jsonStr, const char* dataPath,
                jsmnrtok_t* pTokens, int numTokens,
                int& startTokenIdx, int& endTokenIdx)
    {
        // Get required token
        int keyIdx = findKeyInJson(jsonStr, pTokens, numTokens,
                            dataPath, endTokenIdx);
        if (keyIdx < 0)
        {
            //Log.info("getTokenByDataPath not found %s", dataPath);
            return false;
        }

        // Copy out token value
        startTokenIdx = keyIdx;
        return true;
    }

    static int findObjectEnd(const char *jsonOriginal, jsmnrtok_t tokens[],
                unsigned int numTokens, int curTokenIdx,
                int count, bool atObjectKey = true)
    {
        // Log.trace("findObjectEnd idx %d, count %d, start %s", curTokenIdx, count,
        //                 jsonOriginal + tokens[curTokenIdx].start);
		// Primitives have a size of 0 but we still need to skip over them ...
        unsigned int tokIdx = curTokenIdx;
		if (count == 0)
		{
            jsmnrtok_t* pTok = tokens + tokIdx;
            if (pTok->type == JSMNR_ARRAY)
                return tokIdx+1;
        }
        for (int objIdx = 0; objIdx < count; objIdx++)
        {
            jsmnrtok_t* pTok = tokens + tokIdx;
            if (pTok->type == JSMNR_PRIMITIVE)
            {
                // Log.trace("findObjectEnd PRIMITIVE");
                tokIdx += 1;
            }
            else if (pTok->type == JSMNR_STRING)
            {
                // Log.trace("findObjectEnd STRING");
				if (atObjectKey)
				{
					tokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx + 1, 1, false);
				}
				else
				{
					tokIdx += 1;
				}
            }
            else if (pTok->type == JSMNR_OBJECT)
            {
                // Log.trace("findObjectEnd OBJECT");
                tokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx+1, pTok->size, true);
            }
            else if (pTok->type == JSMNR_ARRAY)
            {
                // Log.trace("findObjectEnd ARRAY");
                tokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx+1, pTok->size, false);
			}
			else
			{
				Log.trace("findObjectEnd UNKNOWN!!!!!!! %d", pTok->type);
				tokIdx += 1;
			}
            if (tokIdx >= numTokens)
            {
                break;
            }
        }
        // Log.trace("findObjectEnd returning %d, start %s, end %s", tokIdx,
        //                 jsonOriginal + tokens[tokIdx].start,
        //             jsonOriginal + tokens[tokIdx].end);
        return tokIdx;
    }

    static int findKeyInJson(const char *jsonOriginal, jsmnrtok_t tokens[],
                             unsigned int numTokens, const char *dataPath,
                             int& endTokenIdx,
                             jsmnrtype_t keyType = JSMNR_UNDEFINED)
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
            // Log.trace("SlashPos %d, %d", slashPos, slashPos-pDataPathPos);
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

            // Log.trace("findKeyInJson srchKey %s arrayIdx %ld", srchKey, reqdArrayIdx);

            // Iterate over tokens to find key of the right type
            // If we are already looking at the node level then search for requested type
            // Otherwise search for and object that will contain the next level key
            jsmnrtype_t keyTypeToFind = atNodeLevel ? keyType : JSMNR_STRING;
            for (int tokIdx = curTokenIdx; tokIdx <= maxTokenIdx; )
            {
				// See if the key matches - this can either be a string match on an object key or
				// just an array element match (with an empty key)
                jsmnrtok_t *pTok = tokens + tokIdx;
				bool keyMatchFound = false;
				if ( (pTok->type == JSMNR_STRING) &&
					((int)strlen(srchKey) == pTok->end - pTok->start) &&
					(strncmp(jsonOriginal + pTok->start, srchKey, pTok->end - pTok->start) == 0) )
				{
					keyMatchFound = true;
					tokIdx += 1;
				}
				else if ((pTok->type == JSMNR_ARRAY) &&
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
                        if (tokens[tokIdx].type == JSMNR_ARRAY)
                        {
                            int newTokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx+1, reqdArrayIdx, false);
                            Log.trace("TokIdxArray inIdx %d, reqdArrayIdx %d, outTokIdx %d",
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
                        // Log.trace("findObjectEnd we have got it %d", tokIdx);
                        if ((keyTypeToFind == JSMNR_UNDEFINED) || (tokens[tokIdx].type == keyTypeToFind))
                        {
                            endTokenIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx, 1, false);
                            //int testTokenIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx+1, 1);
                            //Log.trace("TokIdxDiff max %d, test %d, diff %d", endTokenIdx, testTokenIdx, testTokenIdx-endTokenIdx);
                            return tokIdx;
                        }
                        return -1;
                    }
                    else
                    {
                        // Check for an object
                        // Log.trace("findObjectEnd inside");
                        if (tokens[tokIdx].type == JSMNR_OBJECT)
                        {
                            // Continue next level of search in this object
                            maxTokenIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx, 1);
                            //int testTokenIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx+1, 1);
                            //Log.trace("TokIdxDiff2 max %d, test %d, diff %d", maxTokenIdx, testTokenIdx, testTokenIdx- maxTokenIdx);
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
                else if (pTok->type == JSMNR_STRING)
                {
                    // We're at a key string but it isn't the one we want so skip its contents
                    tokIdx = findObjectEnd(jsonOriginal, tokens, numTokens, tokIdx, 1);
                }
				else if (pTok->type == JSMNR_OBJECT)
				{
					// Move to the first key of the object
					tokIdx++;
				}
				else if (pTok->type == JSMNR_ARRAY)
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
        if (maxx == 0)
            return 0;
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
				skipJSONWhitespace && (isspace(ch) || (ch > 0 && ch < 32) || (ch >= 127)))
            {
				continue;
            }
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
        pDest[toAlloc] = 0;
        if (toAlloc == 0)
            return pDest;
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
//        Log.trace("safeStringDup <%s> %d %d %d %d %d %d %d <%s>", pSrc, maxx, toAlloc, srcStrlen, stringLen, insideDoubleQuotes, insideSingleQuotes, skipJSONWhitespace, pDest);
		return pDest;
	}

#ifdef CONFIG_MANAGER_RECREATE_JSON
	static int recreateJson(const char *js, jsmnrtok_t *t,
		size_t count, int indent, String& outStr)
	{
		int i, j, k;

		if (count == 0)
		{
			return 0;
		}
		if (t->type == JSMNR_PRIMITIVE)
		{
			Log.trace("#Found primitive size %d, start %d, end %d",
				t->size, t->start, t->end);
			Log.trace("%.*s", t->end - t->start, js + t->start);
			char *pStr = safeStringDup(js + t->start,
				t->end - t->start);
			outStr.concat(pStr);
			delete[] pStr;
			return 1;
		}
		else if (t->type == JSMNR_STRING)
		{
			Log.trace("#Found string size %d, start %d, end %d",
				t->size, t->start, t->end);
			Log.trace("'%.*s'", t->end - t->start, js + t->start);
			char *pStr = safeStringDup(js + t->start,
				t->end - t->start);
			outStr.concat("\"");
			outStr.concat(pStr);
			outStr.concat("\"");
			delete[] pStr;
			return 1;
		}
		else if (t->type == JSMNR_OBJECT)
		{
			Log.trace("#Found object size %d, start %d, end %d",
				t->size, t->start, t->end);
			j = 0;
			outStr.concat("{");
			for (i = 0; i < t->size; i++)
			{
				for (k = 0; k < indent; k++)
				{
					Log.trace("  ");
				}
				j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
				outStr.concat(":");
				Log.trace(": ");
				j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
				Log.trace("");
				if (i != t->size - 1)
				{
					outStr.concat(",");
				}
			}
			outStr.concat("}");
			return j + 1;
		}
		else if (t->type == JSMNR_ARRAY)
		{
			Log.trace("#Found array size %d, start %d, end %d",
				t->size, t->start, t->end);
			j = 0;
			outStr.concat("[");
			Log.trace("");
			for (i = 0; i < t->size; i++)
			{
				for (k = 0; k < indent - 1; k++)
				{
					Log.trace("  ");
				}
				Log.trace("   - ");
				j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
				if (i != t->size - 1)
				{
					outStr.concat(",");
				}
				Log.trace("");
			}
			outStr.concat("]");
			return j + 1;
		}
		return 0;
	}

	static bool doPrint(const char* jsonStr)
	{
		JSMNR_parser parser;
		JSMNR_init(&parser);
		int tokenCountRslt = JSMNR_parse(&parser, jsonStr, strlen(jsonStr),
			NULL, 1000);
		if (tokenCountRslt < 0)
		{
			Log.info("JSON parse result: %d", tokenCountRslt);
			return false;
		}
		jsmnrtok_t* pTokens = new jsmnrtok_t[tokenCountRslt];
		JSMNR_init(&parser);
		tokenCountRslt = JSMNR_parse(&parser, jsonStr, strlen(jsonStr),
			pTokens, tokenCountRslt);
		if (tokenCountRslt < 0)
		{
			Log.info("JSON parse result: %d", tokenCountRslt);
			delete pTokens;
			return false;
		}
		// Top level item must be an object
		if (tokenCountRslt < 1 || pTokens[0].type != JSMNR_OBJECT)
		{
			Log.error("JSON must have top level object");
			delete pTokens;
			return false;
		}
		Log.trace("Dumping");
		recreateJson(jsonStr, pTokens, parser.toknext, 0);
		delete pTokens;
		return true;
	}

#endif // CONFIG_MANAGER_RECREATE_JSON

};
