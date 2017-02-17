// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"

#define LOG_MAX_STRING_LENGTH_R 160

#if PLATFORM_ID == 88 // RedBear Duo

class SerialLogHandlerR
{
public:
    typedef enum LOG_LEVEL_TYPE
    {
        LOG_LEVEL_ALL = 10,
        LOG_LEVEL_TRACE = 5,
        LOG_LEVEL_INFO = 4,
        LOG_LEVEL_WARN = 3,
        LOG_LEVEL_ERROR = 2,
        LOG_LEVEL_NONE = 0
    } ;

public:
    SerialLogHandlerR(LOG_LEVEL_TYPE logLevel);
};

class LoggerWiring
{
private:
    static char buf[LOG_MAX_STRING_LENGTH_R+1];
    static bool _logToSerial;
    static SerialLogHandlerR::LOG_LEVEL_TYPE _logLevel;

public:
    static void setLogLevel(SerialLogHandlerR::LOG_LEVEL_TYPE logLevel)
    {
		_logLevel = logLevel;
		_logToSerial = true;
	}
    static void vprintln(const char* fmt, va_list args)
    {
        vsnprintf(buf, sizeof(buf)-1, fmt, args);
        Serial.println(buf);
    }

    static void trace(const char* fmt, ...)
    {
        if (!_logToSerial)
            return;
        if (_logLevel < SerialLogHandlerR::LOG_LEVEL_TRACE)
            return;
        va_list args;
        va_start(args, fmt);
        vprintln(fmt, args);
        va_end(args);
    }
    static void info(const char* fmt, ...)
    {
        if (!_logToSerial)
            return;
        if (_logLevel < SerialLogHandlerR::LOG_LEVEL_INFO)
            return;

        va_list args;
        va_start(args, fmt);
        vprintln(fmt, args);
        va_end(args);
    }
    static void warn(const char* fmt, ...)
    {
        if (!_logToSerial)
            return;
        if (_logLevel < SerialLogHandlerR::LOG_LEVEL_WARN)
            return;
        va_list args;
        va_start(args, fmt);
        vprintln(fmt, args);
        va_end(args);
    }
    static void error(const char* fmt, ...)
    {
        if (!_logToSerial)
            return;
        if (_logLevel < SerialLogHandlerR::LOG_LEVEL_ERROR)
            return;
        va_list args;
        va_start(args, fmt);
        vprintln(fmt, args);
        va_end(args);
    }
};

extern LoggerWiring Log;

#endif
