
#include "stdafx.h"
#include "Application.h"

RdLogger* RdLogger::_pLogger = 0;

Time_t Time;


void pinMode(int pin, int mode)
{
}

void digitalWrite(int pin, int val)
{
}

bool digitalRead(int pin)
{
	return 0;
}

void delayMicroseconds(unsigned long us)
{
}

unsigned long millis()
{
	return 0;
}

uint32_t curMicros = 0;
uint32_t micros()
{
	return curMicros++;
}
