// Utils
// Rob Dobson 2012-2017

#pragma once

#include <Arduino.h>

class Utils
{
public:

    // Test for a timeout handling wrap around
    // Usage: isTimeout(millis(), myLastTime, 1000)
    // This will return true if myLastTime was set to millis() more than 1000 milliseconds ago
    static bool isTimeout(unsigned long curTime, unsigned long lastTime, unsigned long maxDuration);

    // Get time until a timeout handling wrap around
    // Usage: timeToTimeout(millis(), myLastTime, 1000)
    // This will return true if myLastTime was set to millis() more than 1000 milliseconds ago
    static int timeToTimeout(unsigned long curTime, unsigned long lastTime, unsigned long maxDuration);

    // Setup a result string
    static void setJsonBoolResult(String& resp, bool rslt, const char* otherJson = NULL);

    // Following code from Unix sources
    static const unsigned long INADDR_NONE = ((unsigned long)0xffffffff);
    static unsigned long convIPStrToAddr(String &inStr);

    // Escape a string in JSON
    static String escapeJSON(const String& inStr);

    // Convert HTTP query format to JSON
    // JSON only contains name/value pairs and not {}
    static String getJSONFromHTTPQueryStr(const char* inStr, bool mustStartWithQuestionMark = true);

    // Get Nth field from string
    static String getNthField(const char* inStr, int N, char separator);
};
