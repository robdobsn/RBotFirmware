// Config Non-Volatile Storage
// Rob Dobson 2016-2017

#pragma once

#include "ConfigBase.h"
#include <Preferences.h>

class ConfigNVS : public ConfigBase
{
private:
    // Max length of config data
    int _configMaxDataLen;

    // Namespace used for Arduino Preferences lib
    String _configNamespace;

    // Arduino preferences instance
    Preferences _preferences;

public:
    ConfigNVS(const char *configNamespace, int configMaxlen)
    {
        _configNamespace = configNamespace;
        _configMaxDataLen = configMaxlen;
    }

    ~ConfigNVS()
    {
    }

    // Get max length
    int getMaxLen();

    // Clear
    void clear();

    // Initialise
    bool setup();

    // Write configuration string
    bool writeConfig();
};
