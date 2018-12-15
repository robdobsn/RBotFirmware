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

    // List of callbacks on change of config
    std::vector<ConfigChangeCallbackType> _configChangeCallbacks;

    // Register change callback
    void registerChangeCallback(ConfigChangeCallbackType configChangeCallback);

public:
    ConfigNVS(const char *configNamespace, int configMaxlen);
    ~ConfigNVS();

    // Clear
    void clear();

    // Initialise
    bool setup();

    // Write configuration string
    bool writeConfig();
};
