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
		String& getConfigData()
		{
			// Log.info("ConfigManager: getConfigData length %d [0] %d", _dataStrJSON.length(), _dataStrJSON.charAt(0));
			return _dataStrJSON;
		}

    // Set the configuration data directly
    void setConfigData(const char* configJSONStr)
    {
			_dataStrJSON = configJSONStr;
			// Log.info("ConfigManager: setConfigData length %d [0] %d", _dataStrJSON.length(), _dataStrJSON.charAt(0));
    }
};
