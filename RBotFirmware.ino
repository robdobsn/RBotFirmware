// RBotFirmware
// Rob Dobson 2016

#define RD_DEBUG_LEVEL 4
#include "LogHandlerR.h"
#include "ConfigEEPROM.h"
#include "RobotController.h"
#include "WorkflowManager.h"
#include "GCodeInterpreter.h"
#include "CommsSerial.h"

//define RUN_TESTS_CONFIG
#define RUN_TEST_WORKFLOW
#ifdef RUN_TEST_CONFIG
#include "TestConfigManager.h"
#endif
#ifdef RUN_TEST_WORKFLOW
#include "TestWorkflowGCode.h"
#endif

SerialLogHandlerR logHandler(SerialLogHandlerR::LOG_LEVEL_ALL);

RobotController _robotController;
WorkflowManager _workflowManager;
CommandInterpreter _commandInterpreter(&_workflowManager);
CommsSerial _commsSerial(0);
ConfigEEPROM configEEPROM;

static const char* EEPROM_CONFIG_LOCATION_STR =
    "{\"base\": 0, \"maxLen\": 1000}";

static const char* TEST_ROBOT_CONFIG_STR =
    "{\"robotType\": \"GeistBot\", \"motorEnPin\":\"D4\", \"motorEnOnVal\":1, \"motorDisableSecs\":60.0,"
    " \"mugRotation\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":100.0, \"accel\":100.0},"
    " \"xLinear\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":100.0, \"accel\":100.0}"
    "}";

static const char* TEST_WORKFLOW_CONFIG_STR =
    "{\"CommandQueue\": { \"cmdQueueMaxLen\":50 } }";

void setup()
{
    delay(5000);
    Log.info("RBotFirmware (built %s %s)", __DATE__, __TIME__);
    Log.info("System version: %s", (const char*)System.version());

    // Initialise the config manager
    configEEPROM.setConfigLocation(EEPROM_CONFIG_LOCATION_STR);

    #ifdef RUN_TEST_CONFIG
    TestConfigManager::runTests();
    #endif

    _robotController.init(TEST_ROBOT_CONFIG_STR);
    _workflowManager.init(TEST_WORKFLOW_CONFIG_STR);

}

void loop()
{
    #ifdef RUN_TEST_WORKFLOW
    // TEST add to command queue
    __testWorkflowGCode.testLoop(_workflowManager, _robotController);
    #endif

    // Service CommsSerial
    _commsSerial.service(_commandInterpreter);

    // Service the robot controller
    _robotController.service();
}
