
#pragma once

#include <stdio.h>  
#include <stdarg.h> 
#include <algorithm>
#include "String.h"

#define TEST_IN_GCPP

#define STEP_TICKER_FREQUENCY 1000000.0f
#define STEP_TICKER_FREQUENCY_2 (STEP_TICKER_FREQUENCY*STEP_TICKER_FREQUENCY)
#define STEPTICKER_FPSCALE (1<<30)
#define STEPTICKER_TOFP(x) ((int32_t)roundf((float)(x)*STEPTICKER_FPSCALE))
#define STEPTICKER_FROMFP(x) ((float)(x)/STEPTICKER_FPSCALE)

#define __attribute__(A) /* do nothing */

class RdLogger {
public:
#pragma warning(push)
#pragma warning(disable: 4793)
#pragma warning(disable: 4996)
	void info(const char * format, ...)
	{
		char buffer[1000];
		va_list args;
		va_start(args, format);
		vsprintf(buffer, format, args);
		printf(buffer);
		printf("\n");
		//System::Console::WriteLine(buffer);
		va_end(args);
	}
	void trace(const char * format, ...)
	{
		char buffer[1000];
		va_list args;
		va_start(args, format);
		vsprintf(buffer, format, args);
		printf(buffer);
		printf("\n");
		//System::Console::WriteLine(buffer);
		va_end(args);
	}
#pragma warning(pop)

	static RdLogger* getLogger() {
		if (_pLogger == NULL)
		{
			_pLogger = new RdLogger();
		}
		return _pLogger;
	}

	static RdLogger* _pLogger;
};

static RdLogger Log;

#define String std::string

