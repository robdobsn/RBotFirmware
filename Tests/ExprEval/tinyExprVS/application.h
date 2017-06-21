#pragma once

#include <stdio.h>
#include <stdarg.h>
#include "spark_wiring_string.h"

class Serial_
{
public:
	static void printf(const char * format, ...)
	{
		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}
	static void printlnf(const char * format, ...)
	{
		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		printf("\n");
	}
};

Serial_ Serial;

