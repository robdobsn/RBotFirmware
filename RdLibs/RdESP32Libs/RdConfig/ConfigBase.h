// Config
// Rob Dobson 2016-2018

#pragma once

#include <ArduinoJson.h>

class ConfigBase
{
  protected:
    // Data is stored in a single string as JSON
    String _dataStrJSON;

  public:
    ConfigBase()
    {
        setConfigData("");
    }

    ConfigBase(const char* configStr)
    {
        setConfigData(configStr);
    }

    ~ConfigBase()
    {
    }

    // Get config data string
    virtual const char *getConfigData()
    {
        return _dataStrJSON.c_str();
    }

    // Set the configuration data directly
    virtual void setConfigData(const char *configJSONStr)
    {
        if (strlen(configJSONStr) == 0)
            _dataStrJSON = "{}";
        else
            _dataStrJSON = configJSONStr;
    }

    virtual String getString(const char *dataPath, const char *defaultValue)
    {
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject(_dataStrJSON.c_str());
        if (!root.success())
            return defaultValue;
        if (!root.containsKey(dataPath))
            return defaultValue;
        return root[dataPath];
    }

    virtual long getLong(const char *dataPath, long defaultValue)
    {
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject(_dataStrJSON.c_str());
        if (!root.success())
            return defaultValue;
        if (!root.containsKey(dataPath))
            return defaultValue;
        return root[dataPath].as<long>();
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
