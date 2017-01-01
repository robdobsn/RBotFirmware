
#include "LogHandlerR.h"

bool LoggerWiring::_logToSerial = false;
SerialLogHandlerR::LOG_LEVEL_TYPE LoggerWiring::_logLevel =
                    SerialLogHandlerR::LOG_LEVEL_NONE;
char LoggerWiring::buf[LOG_MAX_STRING_LENGTH_R+1];

LoggerWiring Log;

SerialLogHandlerR::SerialLogHandlerR(SerialLogHandlerR::LOG_LEVEL_TYPE logLevel)
{
    Log.setLogLevel(logLevel);
}
