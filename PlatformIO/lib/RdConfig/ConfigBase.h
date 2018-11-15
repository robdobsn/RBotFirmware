// Config
// Rob Dobson 2016-2018

#pragma once

#include "RdJson.h"

class ConfigBase
{
protected:
    // Data is stored in a single string as JSON
    String _dataStrJSON;

    // Max length of config data
    int _configMaxDataLen;

public:
    ConfigBase()
    {
        setConfigData("");
        _configMaxDataLen = 0;
    }

    ConfigBase(const char* configStr)
    {
        setConfigData(configStr);
    }

    ~ConfigBase()
    {
    }

    // Get config data string
    virtual const char *getConfigCStrPtr()
    {
        return _dataStrJSON.c_str();
    }

    // Get reference to config WString
    virtual String& getConfigString()
    {
        return _dataStrJSON;
    }

    // Set the configuration data directly
    virtual void setConfigData(const char *configJSONStr)
    {
        if (strlen(configJSONStr) == 0)
            _dataStrJSON = "{}";
        else
            _dataStrJSON = configJSONStr;
    }

    // Get max length
    int getMaxLen()
    {
        return _configMaxDataLen;
    }

    virtual String getString(const char *dataPath, const char *defaultValue)
    {
        return RdJson::getString(dataPath, defaultValue, _dataStrJSON.c_str());
    }

    virtual long getLong(const char *dataPath, long defaultValue)
    {
        return RdJson::getLong(dataPath, defaultValue, _dataStrJSON.c_str());
    }

    virtual void clear()
    {
    }

    virtual bool setup()
    {
        return false;
    }

    virtual bool writeConfig()
    {
        return false;
    }
};
