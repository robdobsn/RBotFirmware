// RBotFirmware
// Rob Dobson 2016

#define RD_DEBUG_LEVEL 4
#include "LogHandlerR.h"
#include "RobotController.h"
#include "WorkflowManager.h"
#include "GCodeInterpreter.h"

//define RUN_TESTS
#ifdef RUN_TESTS
#include "TestConfigManager.h"
#include "TestWorkflowGCode.h"
#endif

SerialLogHandlerR logHandler(SerialLogHandlerR::LOG_LEVEL_ALL);

RobotController robotController;
WorkflowManager workflowManager;

static const char* EEPROM_CONFIG_LOCATION_STR =
    "{\"base\": 0, \"maxLen\": 1000}";

static const char* TEST_ROBOT_CONFIG_STR =
    "{\"robotType\": \"MugBot\", \"mugRotatePin\": \"D0\"}";
static const char* TEST_WORKFLOW_CONFIG_STR =
    "{\"CommandQueue\": { \"cmdQueueMaxLen\":50 } }";


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

void loop()
{
    #ifdef RUN_TESTS
    // TEST add to command queue
    __testWorkflowGCode.testLoop(workflowManager, robotController);
    #endif
}
