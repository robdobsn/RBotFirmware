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

/*static const char* TEST_ROBOT_CONFIG_STR =
    "{\"robotType\": \"MugBot\", \"motorEnPin\":\"A2\", \"motorEnOnVal\":1, \"motorDisableSecs\":60.0,"
    " \"mugRotation\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":100.0, \"accel\":100.0},"
    " \"xLinear\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":100.0, \"accel\":100.0}"
    "}";*/
static const char* TEST_ROBOT_CONFIG_STR =
    "{\"robotType\": \"GeistBot\", \"motorEnPin\":\"A2\", \"motorEnOnVal\":1, \"motorDisableSecs\":60.0,"
    " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":100.0, \"acceleration\":100.0, \"stepsPerUnit\":33.3333,"
    " \"endStop0\": { \"sensePin\": \"A6\", \"activeLevel\":1, \"inputType\":\"INPUT_PULLUP\"}},"
    " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":100.0, \"acceleration\":100.0, \"stepsPerUnit\":33.3333,"
    " \"endStop0\": { \"sensePin\": \"A7\", \"activeLevel\":1, \"inputType\":\"INPUT_PULLUP\"}},"
    "}";

static const char* TEST_WORKFLOW_CONFIG_STR =
    "{\"CommandQueue\": { \"cmdQueueMaxLen\":50 } }";

void step1()
{
    digitalWrite(D2, 1);
    delayMicroseconds(1);
    digitalWrite(D2, 0);
    delayMicroseconds(500);
}

void setup()
{
    delay(5000);
    Log.info("RBotFirmware (built %s %s)", __DATE__, __TIME__);
    Log.info("System version: %s", (const char*)System.version());

/*
    pinMode(A6, INPUT_PULLUP);
    pinMode(A7, INPUT_PULLUP);
    for (int i = 0; i < 40; i++)
    {
        Serial.printlnf("A6=%d A7=%d", digitalRead(A6), digitalRead(A7));
        delay(500);
    }*/
    /*pinMode(A6, INPUT_PULLUP);
    pinMode(D2, OUTPUT);
    pinMode(D3, OUTPUT);
    pinMode(A2, OUTPUT);
    digitalWrite(A2, 1);
    digitalWrite(D3, 0);
    int turnCount = 0;
    for (int i = 0; i < 30000; i++)
    {
        if (!digitalRead(A6))
            break;
        step1();
    }
    for (int i = 0; i < 30000; i++)
    {
        if (digitalRead(A6))
            break;
        step1();
    }
    for (int i = 0; i < 30000; i++)
    {
        if (!digitalRead(A6))
            break;
        step1();
        turnCount++;
    }
    for (int i = 0; i < 30000; i++)
    {
        if (digitalRead(A6))
            break;
        step1();
        turnCount++;
    }
    Log.info("TURN COUNT %d", turnCount);*/

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
