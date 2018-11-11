// RdJson
// Rob Dobson 2017-2018

// Many of the methods here support a dataPath parameter. This uses a syntax like a much simplified XPath:
// [0] returns the 0th element of an array
// / is a separator of nodes

#pragma once
#include <stdlib.h>
#include <limits.h>
#include <WString.h>
#include <ArduinoLog.h>
#include "jsmnParticleR.h"

// Define this to enable reformatting of JSON
//#define RDJSON_RECREATE_JSON 1

class RdJson {
public:
    // Get location of element in JSON string
    static bool getElement(const char* dataPath,
                           int& startPos, int& strLen,
                           jsmnrtype_t& objType, int& objSize,
                           const char* pSourceStr);
    // Get a string from the JSON
    static String getString(const char* dataPath,
                            const char* defaultValue, bool& isValid,
                            jsmnrtype_t& objType, int& objSize,
                            const char* pSourceStr);

    // Alternate form of getString with fewer parameters
    static String getString(const char* dataPath, const char* defaultValue,
                            const char* pSourceStr, bool& isValid);

    // Alternate form of getString with fewer parameters
    static String getString(const char* dataPath, const char* defaultValue,
                            const char* pSourceStr);

    static double getDouble(const char* dataPath,
                            double defaultValue, bool& isValid,
                            const char* pSourceStr);

    static double getDouble(const char* dataPath, double defaultValue,
                            const char* pSourceStr);

    static long getLong(const char* dataPath,
                        long defaultValue, bool& isValid,
                        const char* pSourceStr);

    static long getLong(const char* dataPath, long defaultValue, const char* pSourceStr);

    static const char* getObjTypeStr(jsmnrtype_t type);

    static const jsmnrtype_t getType(int& arrayLen, const char* pSourceStr);

    static jsmnrtok_t* parseJson(const char* jsonStr, int& numTokens,
                                 int maxTokens = 10000);

    static void escapeString(String& strToEsc);

    static void unescapeString(String& strToUnEsc);

    static bool getTokenByDataPath(const char* jsonStr, const char* dataPath,
                                   jsmnrtok_t* pTokens, int numTokens,
                                   int& startTokenIdx, int& endTokenIdx);

    static int findObjectEnd(const char* jsonOriginal, jsmnrtok_t tokens[],
                             unsigned int numTokens, int curTokenIdx,
                             int count, bool atObjectKey = true);

    static int findKeyInJson(const char* jsonOriginal, jsmnrtok_t tokens[],
                             unsigned int numTokens, const char* dataPath,
                             int& endTokenIdx,
                             jsmnrtype_t keyType = JSMNR_UNDEFINED);

    static size_t safeStringLen(const char* pSrc,
                                bool skipJSONWhitespace = false, size_t maxx = LONG_MAX);

    static void safeStringCopy(char* pDest, const char* pSrc,
                               size_t maxx, bool skipJSONWhitespace = false);

    static char* safeStringDup(const char* pSrc, size_t maxx,
                               bool skipJSONWhitespace = false);

#ifdef RDJSON_RECREATE_JSON
    static int recreateJson(const char* js, jsmnrtok_t* t,
                            size_t count, int indent, String& outStr);
    static bool doPrint(const char* jsonStr);
#endif // RDJSON_RECREATE_JSON
};
