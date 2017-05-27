// RBotFirmware
// Rob Dobson 2016

#include "ConfigEEPROM.h"
#include "RobotController.h"
#include "WorkflowManager.h"
#include "GCodeInterpreter.h"
#include "CommsSerial.h"
#include "PatternGeneratorModSpiral.h"
#include "PatternGeneratorTestPattern.h"

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
PatternGeneratorTestPattern _patternGeneratorTestPattern;
PatternGenerator* _patternGenerators[] = { &_patternGeneratorModSpiral, &_patternGeneratorTestPattern };
constexpr int NUM_PATTERN_GENERATORS = sizeof(_patternGenerators)/sizeof(_patternGenerators[0]);
CommandInterpreter _commandInterpreter(&_workflowManager, _patternGenerators, NUM_PATTERN_GENERATORS);
CommsSerial _commsSerial(0);
ConfigEEPROM configEEPROM;

static const char* EEPROM_CONFIG_LOCATION_STR =
    "{\"base\": 0, \"maxLen\": 1000}";

// Mugbot on PiHat 1.1
// linear axis 1/8 microstepping,
// rotary axis 1/16 microstepping
static const char* ROBOT_CONFIG_STR_MUGBOT_PIHAT_1_1 =
    "{\"robotType\": \"MugBot\", \"xMaxMM\":150, \"yMaxMM\":120, "
    " \"stepEnablePin\":\"D4\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":60.0,"
    " \"axis0\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":5.0, \"acceleration\":2.0,"
    " \"stepsPerRotation\":3200, \"unitsPerRotation\":360, \"minVal\":0, \"maxVal\":240}, "
    " \"axis1\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":75.0, \"acceleration\":5.0,"
    " \"stepsPerRotation\":1600, \"unitsPerRotation\":2.0, \"minVal\":0, \"maxVal\":78, "
    " \"endStop0\": { \"sensePin\": \"D7\", \"activeLevel\":1, \"inputType\":\"INPUT_PULLUP\"}},"
    " \"axis2\": { \"servoPin\": \"D0\", \"isServoAxis\": 1, \"homeOffsetVal\": 120, \"homeOffsetSteps\": 1666,"
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
    " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":75.0, \"acceleration\":5.0,"
    " \"stepsPerRotation\":12000, \"unitsPerRotation\":44.8, \"minVal\":0, \"maxVal\":195, "
    " \"endStop0\": { \"sensePin\": \"A7\", \"activeLevel\":0, \"inputType\":\"INPUT_PULLUP\"}},"
    "}";

static const char* ROBOT_CONFIG_STR_SANDTABLESCARA =
    "{\"robotType\": \"SandTableScara\", \"xMaxMM\":200, \"yMaxMM\":200, "
    " \"stepEnablePin\":\"A2\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
    " \"blockDistanceMM\":400.0, \"homingAxis1OffsetDegs\":20.0,"
    " \"maxHomingSecs\":120, \"cmdsAtStart\":\"G28\","
    " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":75.0, \"acceleration\":5.0,"
    " \"minNsBetweenSteps\":1000000,"
    " \"stepsPerRotation\":9600, \"unitsPerRotation\":628.318,"
    " \"endStop0\": { \"sensePin\": \"A6\", \"activeLevel\":0, \"inputType\":\"INPUT_PULLUP\"}},"
    " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":75.0, \"acceleration\":5.0,"
    " \"minNsBetweenSteps\":1000000,"
    " \"stepsPerRotation\":9600, \"unitsPerRotation\":628.318, \"homeOffsetSteps\": 0,"
    " \"endStop0\": { \"sensePin\": \"A7\", \"activeLevel\":0, \"inputType\":\"INPUT_PULLUP\"}},"
    "}";

static const char* ROBOT_CONFIG_STR = ROBOT_CONFIG_STR_SANDTABLESCARA;

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
unsigned long loopWindowSumMicros = 0;
unsigned long lastLoopStartMicros = 0;
unsigned long lastDebugLoopMillis = 0;
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
                loopWindowSumMicros -= oldVal;
            }
            loopTimeAvgWin[loopTimeAvgWinHead++] = loopTime;
            if (loopTimeAvgWinHead >= loopTimeAvgWinLen)
            loopTimeAvgWinHead = 0;
            if (loopTimeAvgWinCount < loopTimeAvgWinLen)
            loopTimeAvgWinCount++;
            loopWindowSumMicros += loopTime;
        }
    }
    lastLoopStartMicros = micros();
    if (millis() > lastDebugLoopMillis + 10000)
    {
        if (loopTimeAvgWinLen > 0)
        {
            Log.info("Avg loop time %0.3fus (val %lu)",
            1.0 * loopWindowSumMicros / loopTimeAvgWinLen,
            lastLoopStartMicros);
        }
        else
        {
            Log.info("No avg loop time yet");
        }
        lastDebugLoopMillis = millis();
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
            Log.info("----RBotFirmware WorkflowGet Rlst=%d, %s, initMem %d, mem %d, lowMem %d", rslt,
                            cmdElem.getString().c_str(), initialMemory,
                            System.freeMemory(), lowestMemory);
            GCodeInterpreter::interpretGcode(cmdElem, _robotController, true);
        }
    }

}
