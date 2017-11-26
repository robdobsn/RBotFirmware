// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "RdJson.h"

class ConfigManager
{
private:
	// Data is stored in a single string as JSON
	String _dataStrJSON;

public:
    ConfigManager()
    {
    }

    ~ConfigManager()
    {
    }

	// Get config data string
	const char* getConfigData()
	{
		return _dataStrJSON.c_str();
	}

    // Set the configuration data directly
    void setConfigData(const char* configJSONStr)
    {
		_dataStrJSON = configJSONStr;
    }
};
