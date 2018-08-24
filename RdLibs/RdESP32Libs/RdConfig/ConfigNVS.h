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

    // Clear
    void clear()
    {
        // Open preferences writeable
        _preferences.begin(_configNamespace.c_str(), false);

        // Clear namespace
        _preferences.clear();

        // Close prefs
        _preferences.end();

        // Set the config str to empty
        ConfigBase::clear();

        // Clear the config string
        setConfigData("");
    }

    // Initialise
    bool setup()
    {
        // Setup base class
        ConfigBase::setup();

        // Open preferences read-only
        _preferences.begin(_configNamespace.c_str(), true);

        // Get config string
        String configData = _preferences.getString("JSON", "{}");
        setConfigData(configData.c_str());
        Log.trace("ConfigNVS: Config %s read len: %d\n", _configNamespace.c_str(), configData.length());
        Log.trace("ConfigNVS: Config %s read: %s\n", _configNamespace.c_str(), configData.c_str());

        // Close prefs
        _preferences.end();

        // Ok
        return true;
    }

    // Write configuration string
    bool writeConfig()
    {
        const char *pConfigData = getConfigData();
        String truncatedStr = "";

        // Get length of string
        int dataStrLen = 0;
        if (pConfigData != NULL)
        {
            dataStrLen = strlen(pConfigData);
        }
        if (dataStrLen >= _configMaxDataLen)
        {
            dataStrLen = _configMaxDataLen - 1;
            truncatedStr = _dataStrJSON.substring(0, dataStrLen);
            pConfigData = truncatedStr.c_str();
        }
        Log.trace("ConfigNVS: Writing %s config len: %d\n", _configNamespace.c_str(), dataStrLen);

        // Open preferences writeable
        _preferences.begin(_configNamespace.c_str(), false);

        // Set config string
        int numPut = _preferences.putString("JSON", pConfigData);

        if (numPut != dataStrLen)
        {
            Log.trace("ConfigNVS: Failed %s write - written = %d\n", _configNamespace.c_str(), numPut);
        }

        // Close prefs
        _preferences.end();

        // Ok
        return true;
    }
};
