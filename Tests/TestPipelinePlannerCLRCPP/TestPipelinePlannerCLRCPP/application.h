
#pragma once

#include <stdio.h>  
#include <stdarg.h> 
#include <algorithm>
#include "spark_wiring_string_mod.h"
#include <ctype.h>

#define ROB_IMPLEMENT_TESTS 1

#define SHOW_TRACE 1
#define SHOW_INFO 1

#define TEST_IN_GCPP

#define STEP_TICKER_FREQUENCY 1000000.0f
#define STEP_TICKER_FREQUENCY_2 (STEP_TICKER_FREQUENCY*STEP_TICKER_FREQUENCY)
#define STEPTICKER_FPSCALE (1<<30)
#define STEPTICKER_TOFP(x) ((int32_t)roundf((float)(x)*STEPTICKER_FPSCALE))
#define STEPTICKER_FROMFP(x) ((float)(x)/STEPTICKER_FPSCALE)

#define __attribute__(A) /* do nothing */

#define strcasecmp(a,b) _stricmp(a,b)

#define INPUT_PULLUP 1
#define INPUT_PULLDOWN 2
#define INPUT 0
#define OUTPUT 4

#define String WiringString

class RdLogger {
public:
#pragma warning(push)
#pragma warning(disable: 4793)
#pragma warning(disable: 4996)
	void info(const char * format, ...)
	{
#ifdef SHOW_INFO
		char buffer[1000];
		va_list args;
		va_start(args, format);
		vsprintf(buffer, format, args);
		printf(buffer);
		printf("\n");
		//System::Console::WriteLine(buffer);
		va_end(args);
#endif
	}
	void trace(const char * format, ...)
	{
#ifdef SHOW_TRACE
		char buffer[1000];
		va_list args;
		va_start(args, format);
		vsprintf(buffer, format, args);
		printf(buffer);
		printf("\n");
		//System::Console::WriteLine(buffer);
		va_end(args);
#endif
	}
	void error(const char * format, ...)
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

//bool isspace(int c)
//{
//	return (c == ' ' || c == 9 || c == 0x0a || c == 0x0b || c == 0x0c || c == 0x0d);
//}

extern void pinMode(int pin, int mode);
extern void digitalWrite(int pin, int val);
extern void digitalWriteFast(int pin, int val);
extern bool digitalRead(int pin);
extern void pinSetFast(int pin);
extern void pinResetFast(int pin);
typedef int PinMode;
extern void testStarting(int testIdx, const char* testName, const char* debugParams, bool firstInRun, int& debugFileIdx);
extern void delayMicroseconds(unsigned long us);

class Servo
{
public:
	void attach(int pin)
	{}
	void detach()
	{}
	void writeMicroseconds(int us)
	{}
};

extern void setTickCount(uint32_t tickCount);
extern unsigned long millis();
extern uint32_t micros();

class Time_t
{
public:
	static unsigned long now()
	{
		return 0;
	}
};

extern Time_t Time;

extern void testCompleted();

extern void __disable_irq();

extern void __enable_irq();

uint32_t SystemTicks();
uint32_t SystemTicksPerMicrosecond();
