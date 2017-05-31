// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "application.h"

class CommandInterpreter;

class PatternGenerator
{
protected:
    String _patternName;

public:
    PatternGenerator()
    {
    }
    const char* getPatternName()
    {
        return _patternName.c_str();
    }
    virtual void start()
    {
    }
    virtual void setParameters(const char* drawParams)
    {
    }
    virtual void service(CommandInterpreter* pCommandInterpreter)
    {
    }
};
