// RBotFirmware
// Rob Dobson 2016

#define RD_DEBUG_LEVEL 4
#include "LogHandlerR.h"
#include "RobotController.h"
#include "WorkflowManager.h"

/*#define RUN_TESTS*/
#ifdef RUN_TESTS
#include "TestConfigManager.h"
#endif

SerialLogHandlerR logHandler(SerialLogHandlerR::LOG_LEVEL_ALL);

RobotController robotController;
WorkflowManager workflowManager;

static const char* EEPROM_CONFIG_LOCATION_STR =
    "{\"base\": 0, \"maxLen\": 1000}";

static const char* TEST_ROBOT_CONFIG_STR =
    "{\"robotType\": \"MugBot\", \"mugRotatePin\": \"D0\"}";
static const char* TEST_WORKFLOW_CONFIG_STR =
    "{\"CommandQueue\": { \"cmdQueueMaxLen\":100 } }";


void setup()
{
    // Initialise the config manager
    /*configManager.SetConfigLocation(CONFIG_LOCATION_STR);*/

    delay(5000);
    Log.info("RBotFirmware (built %s %s)", __DATE__, __TIME__);
    Log.info("System version: %s", (const char*)System.version());

    #ifdef RUN_TESTS
    TestConfigManager::runTests();
    #endif

    robotController.init(TEST_ROBOT_CONFIG_STR);
    workflowManager.init(TEST_WORKFLOW_CONFIG_STR);

}

long millisRateIn = 200;
long millisRateOut = 400;
long lastMillisIn = 0;
long lastMillisOut = 0;
long lastMillisFlip = 0;

void loop()
{
    // TEST add to command queue

    if (millis() > lastMillisIn + millisRateIn)
    {
        String cmdStr = "G01 X0 Y1 Z2";
        bool rslt = workflowManager.add(cmdStr);
        Log.info("Add %d", rslt);
        lastMillisIn = millis();
    }

    if (millis() > lastMillisOut + millisRateOut)
    {
        String cmdStr;
        bool rslt = workflowManager.get(cmdStr);
        Log.info("Get %d = %s, mem %d", rslt,
                            cmdStr.c_str(), System.freeMemory());
        lastMillisOut = millis();
    }

    if (millis() > lastMillisFlip + 30000)
    {
        if (millisRateOut > 300)
            millisRateOut = 100;
        else
            millisRateOut = 400;
        lastMillisFlip = millis();
    }

}
