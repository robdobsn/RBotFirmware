// RBotFirmware
// Rob Dobson 2016

#include "ConfigManager.h"

static const char* CONFIG_LOCATION_STR =
    "{\"source\": \"EEPROM\", \"base\": 0, \"maxLen\": 1000}";

ConfigManager configManager;

void setup()
{
    // Initialise the config manager
    configManager.SetConfigLocation(CONFIG_LOCATION_STR);
    /*bool isValid = false;
    String cStr = configManager.getConfigString("source", "", isValid);
    Serial.printlnf("CStr %s, vlid %d", cStr, isValid);*/
}

void loop()
{

}
