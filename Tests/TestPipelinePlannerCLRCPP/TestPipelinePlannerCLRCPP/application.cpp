
#include "stdafx.h"
#include "application.h"

#include <iostream>
#include <fstream>

RdLogger* RdLogger::_pLogger = 0;

Time_t Time;

std::ofstream __outFile;
bool __outOpen = false;
String __outFileBase = "../../TestOutputData/PipelinePlanner/steps_";
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

void openDebugFileIfReqd(int testIdx, const char* testName, bool firstInRun, int& debugFileIdx)
{
	if (!__outOpen)
	{
		int fileCount = debugFileIdx;
		while (true)
		{
			__outFileName = __outFileBase + String::format("%05d", fileCount) + "_" + String::format("%02d", testIdx) + "_" + testName + ".txt";
			if (!firstInRun)
				break;
			if (!fexists(__outFileName))
				break;
			fileCount++;
		}
		debugFileIdx = fileCount;
		__outFile.open(__outFileName);
		__outOpen = true;
	}
}

void digitalWriteFast(int pin, int val)
{
	digitalWrite(pin, val);
}

void digitalWrite(int pin, int val)
{
	// Ignore if it is a lowering of a step pin (to avoid end of test problem)
	if ((val == 0) && (pin == 17 || pin == 15))
		return;
	// Ignore motor enable
	if (pin == 4)
		return;
	char* pinName = "st0";
	if (pin == 16)
		pinName = "dr0";
	else if (pin == 15)
		pinName = "st1";
	else if (pin == 14)
		pinName = "dr1";
	// Dump
	if (__outFile.is_open())
	{
		__outFile << "W\t" << __curMicros << "\t" << pinName << "\t" << val << "\n";
	}
}

void pinSetFast(int pin)
{
	digitalWrite(pin, 1);
}

void pinResetFast(int pin)
{
	digitalWrite(pin, 0);
}

void testStarting(int testIdx, const char* testName, const char* debugParams, bool firstInRun, int& debugFileIdx)
{
	openDebugFileIfReqd(testIdx, testName, firstInRun, debugFileIdx);
	__outFile << debugParams << "\n\n";
}

void testCompleted()
{
	if (__outOpen)
	{
		Log.trace("================== TEST COMPLETE OUTFILE %s", __outFileName.c_str());
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

void __disable_irq()
{
}

void __enable_irq()
{
}

uint32_t SystemTicks()
{
	return 0;
}

uint32_t SystemTicksPerMicrosecond()
{
	return 1;
}