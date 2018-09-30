// RdJson
// Rob Dobson 2017

#include "RdJson.h"

// Note that the code in this file is not the main body of code (which is
// in the header)

#ifdef RDJSON_RECREATE_JSON
static int RdJson::recreateJson(const char* js, jsmnrtok_t* t,
                                size_t count, int indent, String& outStr)
{
    int i, j, k;

    if (count == 0) {
        return 0;
    }
    if (t->type == JSMNR_PRIMITIVE) {
        Log.trace(F("#Found primitive size %dCR), start %d, end %d",
                  t->size, t->start, t->end);
        Log.trace(F("%.*s"CR), t->end - t->start, js + t->start);
        char* pStr = safeStringDup(js + t->start,
                                   t->end - t->start);
        outStr.concat(pStr);
        delete[] pStr;
        return 1;
    }
    else if (t->type == JSMNR_STRING) {
        Log.trace(F("#Found string size %d, start %d, end %d"CR),
                  t->size, t->start, t->end);
        Log.trace(F("'%.*s'"CR), t->end - t->start, js + t->start);
        char* pStr = safeStringDup(js + t->start,
                                   t->end - t->start);
        outStr.concat("\"");
        outStr.concat(pStr);
        outStr.concat("\"");
        delete[] pStr;
        return 1;
    }
    else if (t->type == JSMNR_OBJECT) {
        Log.trace(F("#Found object size %d, start %d, end %d"CR),
                  t->size, t->start, t->end);
        j = 0;
        outStr.concat("{");
        for (i = 0; i < t->size; i++) {
            for (k = 0; k < indent; k++) {
                Log.trace(F("  "));
            }
            j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
            outStr.concat(":");
            Log.trace(F(": "));
            j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
            Log.trace(F(""));
            if (i != t->size - 1) {
                outStr.concat(",");
            }
        }
        outStr.concat("}");
        return j + 1;
    }
    else if (t->type == JSMNR_ARRAY) {
        Log.trace(F("#Found array size %d, start %d, end %d"CR),
                  t->size, t->start, t->end);
        j = 0;
        outStr.concat("[");
        Log.trace(F(""));
        for (i = 0; i < t->size; i++) {
            for (k = 0; k < indent - 1; k++) {
                Log.trace(F("  "));
            }
            Log.trace(F("   - "));
            j += recreateJson(js, t + 1 + j, count - j, indent + 1, outStr);
            if (i != t->size - 1) {
                outStr.concat(",");
            }
            Log.trace(F(""));
        }
        outStr.concat("]");
        return j + 1;
    }
    return 0;
}

static bool RdJson::doPrint(const char* jsonStr)
{
    JSMNR_parser parser;
    JSMNR_init(&parser);
    int tokenCountRslt = JSMNR_parse(&parser, jsonStr, strlen(jsonStr),
                                     NULL, 1000);
    if (tokenCountRslt < 0) {
        Log.info(F("JSON parse result: %d"CR), tokenCountRslt);
        return false;
    }
    jsmnrtok_t* pTokens = new jsmnrtok_t[tokenCountRslt];
    JSMNR_init(&parser);
    tokenCountRslt = JSMNR_parse(&parser, jsonStr, strlen(jsonStr),
                                 pTokens, tokenCountRslt);
    if (tokenCountRslt < 0) {
        Log.info(F("JSON parse result: %d"CR), tokenCountRslt);
        delete pTokens;
        return false;
    }
    // Top level item must be an object
    if (tokenCountRslt < 1 || pTokens[0].type != JSMNR_OBJECT) {
        Log.error(F("JSON must have top level object"CR);
        delete pTokens;
        return false;
    }
    Log.trace(F("Dumping"CR));
    recreateJson(jsonStr, pTokens, parser.toknext, 0);
    delete pTokens;
    return true;
}

#endif // CONFIG_MANAGER_RECREATE_JSON
