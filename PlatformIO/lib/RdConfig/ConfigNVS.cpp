// Config Non-Volatile Storage
// Rob Dobson 2016-2017

#include "ConfigNVS.h"

static const char* MODULE_PREFIX = "ConfigNVS: ";

// Clear
void ConfigNVS::clear()
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
bool ConfigNVS::setup()
{
    // Setup base class
    ConfigBase::setup();

    // Debug
    Log.trace("%sConfig %s ...\n", MODULE_PREFIX, _configNamespace.c_str());

    // Open preferences read-only
    _preferences.begin(_configNamespace.c_str(), true);

    // Get config string
    String configData = _preferences.getString("JSON", "{}");
    setConfigData(configData.c_str());
    Log.trace("%sConfig %s read: len(%d) %s\n", MODULE_PREFIX, _configNamespace.c_str(), configData.length(), configData.c_str());

    // Close prefs
    _preferences.end();

    // Ok
    return true;
}

// Write configuration string
bool ConfigNVS::writeConfig()
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
    Log.trace("%sWriting %s config len: %d\n", MODULE_PREFIX, _configNamespace.c_str(), dataStrLen);

    // Open preferences writeable
    _preferences.begin(_configNamespace.c_str(), false);

    // Set config string
    int numPut = _preferences.putString("JSON", pConfigData);

    if (numPut != dataStrLen)
    {
        Log.trace("%sFailed %s write - written = %d\n", MODULE_PREFIX, _configNamespace.c_str(), numPut);
    }
    else
    {
        Log.trace("%sWrite ok written = %d\n", MODULE_PREFIX, numPut);
    }

    // Close prefs
    _preferences.end();

    // Ok
    return true;
}
