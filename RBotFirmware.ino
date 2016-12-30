// RBotFirmware
// Rob Dobson 2016

#include "ConfigManager.h"

static const char* CONFIG_LOCATION_STR =
    "{\"source\": \"EEPROM\", \"base\": 0, \"maxLen\": 1000}";

    static const char *JSON_STRING =
    	"{\"user\": \"johndoe\", \"admin\": false, \"uid\": 1000,\n  "
    	"\"things\": [\"users\", \"wheel\", \"audio\", \"video\"], "
        "\"groups\": {\"my-array\" : [1,2,3,4], \"my-str\":\"TESTSTR\"}}";

ConfigManager configManager;

void setup()
{
    // Initialise the config manager
    configManager.SetConfigLocation(CONFIG_LOCATION_STR);

    /*for (int i = 0; i < 1000000; i++)
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
    }*/

    configManager.doPrint(JSON_STRING);

    bool isValid = false;
    String myStr = configManager.getString("groups/my-str", "NOTFOUND", isValid, JSON_STRING);
    Serial.printlnf("Result str %d = %s", isValid, myStr.c_str());

}

void loop()
{

}
