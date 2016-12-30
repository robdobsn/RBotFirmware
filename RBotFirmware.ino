// RBotFirmware
// Rob Dobson 2016

#include "ConfigManager.h"

static const char* CONFIG_LOCATION_STR =
    "{\"source\": \"EEPROM\", \"base\": 0, \"maxLen\": 1000}";

ConfigManager configManager;

void setup()
{
    // Initialise the config manager
    configManager.SetConfigLocation(CONFIG_LOCATION_STR, false);

    for (int i = 0; i < 1000000; i++)
    {
        bool isValid = false;
        String cStr = configManager.getString("source", "", isValid);
        if (isValid == false || i % 100000 == 0)
        {
            Serial.printlnf("CStr %s, valid %d", cStr.c_str(), isValid);
            uint32_t freemem = System.freeMemory();
            Serial.print("free memory: ");
            Serial.println(freemem);
        }
    }
}

void loop()
{

}
