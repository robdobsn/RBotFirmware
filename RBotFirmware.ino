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

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

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
    "{\"robotType\": \"GeistBot\", \"xMaxMM\":400, \"yMaxMM\":400, "
    " \"stepEnablePin\":\"A2\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
    " \"maxHomingSecs\":120, \"homingLinOffsetDegs\":70, \"homingCentreOffsetMM\":0,"
    " \"homingRotCentreDegs\":3.3,"
    " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":100.0, \"acceleration\":100.0,"
    " \"stepsPerRotation\":12000, \"unitsPerRotation\":360, "
    " \"endStop0\": { \"sensePin\": \"A6\", \"activeLevel\":1, \"inputType\":\"INPUT_PULLUP\"}},"
    " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":100.0, \"acceleration\":100.0, "
    "\"stepsPerRotation\":12000, \"unitsPerRotation\":44.8, \"minVal\":0, \"maxVal\":195, "
    " \"endStop0\": { \"sensePin\": \"A7\", \"activeLevel\":0, \"inputType\":\"INPUT_PULLUP\"}},"
    "}";

static const char* TEST_WORKFLOW_CONFIG_STR =
    "{\"CommandQueue\": { \"cmdQueueMaxLen\":50 } }";

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

    // Initialise the config manager
    configEEPROM.setConfigLocation(EEPROM_CONFIG_LOCATION_STR);

    #ifdef RUN_TEST_CONFIG
    TestConfigManager::runTests();
    #endif

    _robotController.init(TEST_ROBOT_CONFIG_STR);
    _workflowManager.init(TEST_WORKFLOW_CONFIG_STR);

}

long initialMemory = System.freeMemory();
long lowestMemory = System.freeMemory();


// Timing of the loop - used to determine if blocking/slow processes are delaying the loop iteration
const int loopTimeAvgWinLen = 50;
int loopTimeAvgWin[loopTimeAvgWinLen];
int loopTimeAvgWinHead = 0;
int loopTimeAvgWinCount = 0;
unsigned long loopTimeAvgWinSum = 0;
unsigned long lastLoopStartMicros = 0;
unsigned long lastDebugLoopTime = 0;
void debugLoopTimer()
{
    // Monitor how long it takes to go around loop
    if (lastLoopStartMicros != 0)
    {
        unsigned long loopTime = micros() - lastLoopStartMicros;
        if (loopTime > 0)
        {
            if (loopTimeAvgWinCount == loopTimeAvgWinLen)
            {
                int oldVal = loopTimeAvgWin[loopTimeAvgWinHead];
                loopTimeAvgWinSum -= oldVal;
            }
            loopTimeAvgWin[loopTimeAvgWinHead++] = loopTime;
            if (loopTimeAvgWinHead >= loopTimeAvgWinLen)
            loopTimeAvgWinHead = 0;
            if (loopTimeAvgWinCount < loopTimeAvgWinLen)
            loopTimeAvgWinCount++;
            loopTimeAvgWinSum += loopTime;
        }
    }
    lastLoopStartMicros = micros();
    if (millis() > lastDebugLoopTime + 10000)
    {
        if (loopTimeAvgWinLen > 0)
        {
            Serial.printlnf("Avg loop time %0.3fus (val %lu)",
            1.0 * loopTimeAvgWinSum / loopTimeAvgWinLen,
            lastLoopStartMicros);
        }
        else
        {
            Serial.println("No avg loop time yet");
        }
        lastDebugLoopTime = millis();
    }
}

void loop()
{
    debugLoopTimer();

    #ifdef RUN_TEST_WORKFLOW
    // TEST add to command queue
    __testWorkflowGCode.testLoop(_workflowManager, _robotController);
    #endif

    // Service CommsSerial
    _commsSerial.service(_commandInterpreter);

    // Service the robot controller
    _robotController.service();

    // Pump the workflow here
    CommandElem cmdElem;
    bool rslt = _workflowManager.get(cmdElem);
    if (rslt)
    {
        if (lowestMemory > System.freeMemory())
            lowestMemory = System.freeMemory();
        Log.info("");
        Log.info("Get %d = %s, initMem %d, mem %d, lowMem %d", rslt,
                        cmdElem.getString().c_str(), initialMemory,
                        System.freeMemory(), lowestMemory);
        GCodeInterpreter::interpretGcode(cmdElem, _robotController, true);
    }

}
