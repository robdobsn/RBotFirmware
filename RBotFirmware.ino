// RBotFirmware
// Rob Dobson 2016

#include "ConfigEEPROM.h"
#include "RobotController.h"
#include "WorkflowManager.h"
#include "GCodeInterpreter.h"
#include "CommsSerial.h"
#include "PatternGeneratorModSpiral.h"

//define RUN_TESTS_CONFIG
//#define RUN_TEST_WORKFLOW
#ifdef RUN_TEST_CONFIG
#include "TestConfigManager.h"
#endif
#ifdef RUN_TEST_WORKFLOW
#include "TestWorkflowGCode.h"
#endif

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_TRACE);
RobotController _robotController;
WorkflowManager _workflowManager;
PatternGeneratorModSpiral _patternGeneratorModSpiral;
PatternGenerator* _patternGenerators[] = { &_patternGeneratorModSpiral };
constexpr int NUM_PATTERN_GENERATORS = sizeof(_patternGenerators)/sizeof(_patternGenerators[0]);
CommandInterpreter _commandInterpreter(&_workflowManager, _patternGenerators, NUM_PATTERN_GENERATORS);
CommsSerial _commsSerial(0);
ConfigEEPROM configEEPROM;

static const char* EEPROM_CONFIG_LOCATION_STR =
    "{\"base\": 0, \"maxLen\": 1000}";

static const char* ROBOT_CONFIG_STR_MUGBOT_PIHAT_1_1 =
    "{\"robotType\": \"MugBot\", \"stepEnablePin\":\"D4\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":60.0,"
    " \"axis0\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":75.0, \"acceleration\":5.0,"
    " \"stepsPerRotation\":12000, \"unitsPerRotation\":360, \"minVal\":0, \"maxVal\":240}, "
    " \"axis1\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":75.0, \"acceleration\":5.0, "
    " \"stepsPerRotation\":12000, \"unitsPerRotation\":44.8, \"minVal\":0, \"maxVal\":195, "
    " \"endStop0\": { \"sensePin\": \"D7\", \"activeLevel\":0, \"inputType\":\"INPUT_PULLUP\"}},"
    " \"axis2\": { \"servoPin\": \"D0\", \"isServoAxis\": 1, \"servoHomeVal\": 90, \"servoHomeSteps\": 1500,"
    " \"minVal\":0, \"maxVal\":180, \"stepsPerRotation\":2000, \"unitsPerRotation\":360 },"
    "}";

static const char* ROBOT_CONFIG_STR_GEISTBOT =
    "{\"robotType\": \"GeistBot\", \"xMaxMM\":400, \"yMaxMM\":400, "
    " \"stepEnablePin\":\"A2\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
    " \"maxHomingSecs\":120, \"homingLinOffsetDegs\":70, \"homingCentreOffsetMM\":4,"
    " \"homingRotCentreDegs\":3.7, \"cmdsAtStart\":\"G28;ModSpiral\", "
    " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":75.0, \"acceleration\":5.0,"
    " \"stepsPerRotation\":12000, \"unitsPerRotation\":360, \"isDominantAxis\":1,"
    " \"endStop0\": { \"sensePin\": \"A6\", \"activeLevel\":1, \"inputType\":\"INPUT_PULLUP\"}},"
    " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":75.0, \"acceleration\":5.0, "
    " \"stepsPerRotation\":12000, \"unitsPerRotation\":44.8, \"minVal\":0, \"maxVal\":195, "
    " \"endStop0\": { \"sensePin\": \"A7\", \"activeLevel\":0, \"inputType\":\"INPUT_PULLUP\"}},"
    "}";

static const char* ROBOT_CONFIG_STR = ROBOT_CONFIG_STR_MUGBOT_PIHAT_1_1;

static const char* WORKFLOW_CONFIG_STR =
    "{\"CommandQueue\": { \"cmdQueueMaxLen\":50 } }";

void setup()
{
    Serial.begin(115200);
    delay(5000);
    Log.info("RBotFirmware (built %s %s)", __DATE__, __TIME__);
    Log.info("System version: %s", (const char*)System.version());

    // Initialise the config manager
    configEEPROM.setConfigLocation(EEPROM_CONFIG_LOCATION_STR);

    #ifdef RUN_TEST_CONFIG
    TestConfigManager::runTests();
    #endif

    _robotController.init(ROBOT_CONFIG_STR);
    _workflowManager.init(WORKFLOW_CONFIG_STR);

    // Check for cmdsAtStart
    String cmdsAtStart = ConfigManager::getString("cmdsAtStart", "", ROBOT_CONFIG_STR);
    _commandInterpreter.process(cmdsAtStart);

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
            Log.info("Avg loop time %0.3fus (val %lu)",
            1.0 * loopTimeAvgWinSum / loopTimeAvgWinLen,
            lastLoopStartMicros);
        }
        else
        {
            Log.info("No avg loop time yet");
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

    // Service the pattern generators
    for (int i = 0; i < NUM_PATTERN_GENERATORS; i++)
    {
        _patternGenerators[i]->service(_commandInterpreter);
    }

    // Service the robot controller
    _robotController.service();

    // Pump the workflow here
    // Check if the RobotController can accept more
    if (_robotController.canAcceptCommand())
    {
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

}
