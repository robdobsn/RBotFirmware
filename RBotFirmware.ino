// RBotFirmware
// Rob Dobson 2016

#include "ConfigManager.h"

#define RUN_TESTS
#ifdef RUN_TESTS
#include "TestConfigManager.h"
#endif

static const char* CONFIG_LOCATION_STR =
    "{\"source\": \"EEPROM\", \"base\": 0, \"maxLen\": 1000}";

ConfigManager configManager;

void setup()
{
    // Initialise the config manager
    configManager.SetConfigLocation(CONFIG_LOCATION_STR);

    #ifdef RUN_TESTS
    TestConfigManager::runTests(configManager);
    #endif


}

void loop()
{

}
