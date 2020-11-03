// Config Non-Volatile Storage
// Rob Dobson 2016-2017

#include "ConfigNVS.h"

static const char* MODULE_PREFIX = "ConfigNVS: ";

ConfigNVS::ConfigNVS(const char *configNamespace, int configMaxlen) :
    ConfigBase(configMaxlen)
{
    _configNamespace = configNamespace;
}

ConfigNVS::~ConfigNVS()
{
}

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
    Log.trace("%sConfig %s read: len(%d) %s maxlen %d\n", MODULE_PREFIX, 
                _configNamespace.c_str(), configData.length(), 
                configData.c_str(), ConfigBase::getMaxLen());

    // Close prefs
    _preferences.end();

    // Ok
    return true;
}

// Write configuration string
bool ConfigNVS::writeConfig()
{
    // Get length of string
    if (_dataStrJSON.length() >= _configMaxDataLen)
        _dataStrJSON = _dataStrJSON.substring(0, _configMaxDataLen-1);
    Log.trace("%sWriting %s config len: %d\n", MODULE_PREFIX, 
                _configNamespace.c_str(), _dataStrJSON.length());

    // Open preferences writeable
    _preferences.begin(_configNamespace.c_str(), false);

    // Set config string
    int numPut = _preferences.putString("JSON", _dataStrJSON.c_str());
    if (numPut != _dataStrJSON.length())
    {
        Log.trace("%sFailed %s write - written = %d\n", MODULE_PREFIX, _configNamespace.c_str(), numPut);
    }
    else
    {
        Log.trace("%sWrite ok written = %d\n", MODULE_PREFIX, numPut);
    }

    // Close prefs
    _preferences.end();

    // Call config change callbacks
    for (int i = 0; i < _configChangeCallbacks.size(); i++)
    {
        if (_configChangeCallbacks[i])
            (_configChangeCallbacks[i])();
    }
    // Ok
    return true;
}

void ConfigNVS::registerChangeCallback(ConfigChangeCallbackType configChangeCallback)
{
    // Save callback if required
    if (configChangeCallback)
    {
        _configChangeCallbacks.push_back(configChangeCallback);
    }
}