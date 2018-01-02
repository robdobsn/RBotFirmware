
#include "stdafx.h"
#include "Application.h"

#include <iostream>
#include <fstream>

RdLogger* RdLogger::_pLogger = 0;

Time_t Time;

std::ofstream __outFile;
bool __outOpen = false;
String __outFileBase = "testOut/testOut_";
String __outFileName;

unsigned long millis()
{
	return 0;
}

uint32_t __curMicros = 0;
uint32_t micros()
{
	return __curMicros++;
}

void setTickCount(uint32_t tickCount)
{
	__curMicros = tickCount;
}

void pinMode(int pin, int mode)
{

}

bool fexists(const String filename) {
	std::ifstream ifile(filename.c_str());
	return (bool)ifile;
}

void digitalWrite(int pin, int val)
{
	// Ignore if it is a lowering of a step pin (to avoid end of test problem)
	if ((val == 0) && (pin == 2 || pin == 4))
		return;
	// 
	if (!__outOpen)
	{
		int fileCount = 0;
		while (true)
		{
			__outFileName = __outFileBase + String::format("%05d", fileCount) + ".txt";
			if (!fexists(__outFileName))
				break;
			fileCount++;
		}
		__outFile.open(__outFileName);
		__outOpen = true;
	}
	if (__outFile.is_open())
	{
		__outFile << "W\t" << __curMicros << "\t" << pin << "\t" << val << "\n";
	}
}

void testCompleted()
{
	if (__outOpen)
	{
		printf("================== TEST COMPLETE OUTFILE %s", __outFileName.c_str());
		__outOpen = false;
		__outFile.close();
	}
}

bool digitalRead(int pin)
{
	return 0;
}

void delayMicroseconds(unsigned long us)
{
}

