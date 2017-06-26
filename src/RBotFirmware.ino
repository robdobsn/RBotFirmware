// RBotFirmware
// Rob Dobson 2016

#include "ConfigEEPROM.h"
#include "RobotController.h"
#include "WorkflowManager.h"
#include "CommsSerial.h"

// API Endpoints
#include "RestAPIEndpoints.h"
RestAPIEndpoints restAPIEndpoints;

// Web server
#include "RdWebServer.h"
const int webServerPort = 80;
RdWebServer* pWebServer = NULL;
#include "GenResources.h"

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
CommandInterpreter _commandInterpreter(&_workflowManager, &_robotController);
CommsSerial _commsSerial(0);
ConfigEEPROM configEEPROM;
bool eepromNeedsWriting = false;

// Time to wait before making a pending write to EEPROM - which takes up-to several seconds and
// blocks motion and web activity
static const unsigned long ROBOT_IDLE_BEFORE_WRITE_EEPROM_SECS = 60;
static const unsigned long WEB_IDLE_BEFORE_WRITE_EEPROM_SECS = 30;

// Note that the value here for maxLen must be bigger than the value returned for restAPI_GetSettings()
// This is to ensure the web-app doesn't return a string that is too long
static const char* EEPROM_CONFIG_LOCATION_STR =
    "{\"base\": 0, \"maxLen\": 2010}";

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
    " \"blockDistanceMM\":1.0, \"homingAxis1OffsetDegs\":20.0,"
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

static const char* ROBOT_DEFAULT_SEQUENCE_COMMANDS =
    "{"
    " \"testSequence\":"
    "  {\"commands\":\"G28;testPattern\"},"
    " \"testSequence2\":"
    "  {\"commands\":\"G28;G28\"}"
    "}";

static const char* ROBOT_DEFAULT_PATTERN_COMMANDS =
    "{"
    " \"pattern1\":"
    "  {"
    "  \"setup\":\"angle=0;diam=10\","
    "  \"loop\":\"x=diam*sin(angle*3);y=diam*cos(angle*3);diam=diam+0.5;angle=angle+0.0314;stop=angle>6.28\""
    "  }"
    "}";

static const char* ROBOT_CONFIG_STR = ROBOT_CONFIG_STR_SANDTABLESCARA;

static const char* WORKFLOW_CONFIG_STR =
    "{\"CommandQueue\": { \"cmdQueueMaxLen\":50 } }";

// Post settings information via API
void restAPI_PostSettings(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    Log.trace("RestAPI PostSettings method %d contentLen %d", apiMsg._method, apiMsg._msgContentLen);
    if (apiMsg._pMsgHeader)
        Log.trace("RestAPI PostSettings header len %d", strlen(apiMsg._pMsgHeader));
    // Store the settings in EEPROM
    configEEPROM.setConfigData((const char*)apiMsg._pMsgContent);
    // Set flag to indicate EEPROM needs to be written
    eepromNeedsWriting = true;
    // Apply the config data
    String patternsStr = ConfigManager::getString("/patterns", "{}", configEEPROM.getConfigData());
    _commandInterpreter.setPatterns(patternsStr);
    String sequencesStr = ConfigManager::getString("/sequences", "{}", configEEPROM.getConfigData());
    _commandInterpreter.setSequences(sequencesStr);
    // Result
    retStr = "{\"ok\"}";
}

// Get settings information via API
void restAPI_GetSettings(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    Log.trace("RestAPI GetSettings method %d contentLen %d", apiMsg._method, apiMsg._msgContentLen);
    // Get settings from each sub-element
    const char* patterns = _commandInterpreter.getPatterns();
    const char* sequences = _commandInterpreter.getSequences();
    Log.trace("RestAPI GetSettings patterns %s", patterns);
    Log.trace("RestAPI GetSettings sequences %s", sequences);
    retStr = "{\"maxCfgLen\":2000, \"name\":\"Sand Table\",\"patterns\":";
    retStr += patterns;
    retStr += ", \"sequences\":";
    retStr += sequences;
    retStr += ", \"startup\":\"\"}";
}

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

    // Add API endpoints
    restAPIEndpoints.addEndpoint("postsettings", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_PostSettings);
    restAPIEndpoints.addEndpoint("getsettings", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_GetSettings);

    // Construct web server
    pWebServer = new RdWebServer();

    // Configure web server
    if (pWebServer)
    {
        // Add resources to web server
        pWebServer->addStaticResources(genResources, genResourcesCount);
        pWebServer->addRestAPIEndpoints(&restAPIEndpoints);
        // Start the web server
        pWebServer->start(webServerPort);
    }

    // Init robot controller and workflow manager
    _robotController.init(ROBOT_CONFIG_STR);
    _workflowManager.init(WORKFLOW_CONFIG_STR);

    // Configure the command interpreter
    bool configLoaded = false;
    const char* pConfig = configEEPROM.getConfigData();
    if (*pConfig == '{' || *pConfig == '[')
    {
        Log.info("Getting configuration from EEPROM");
        String patternsStr = ConfigManager::getString("/patterns", "{}", configEEPROM.getConfigData());
        _commandInterpreter.setPatterns(patternsStr);
        String sequencesStr = ConfigManager::getString("/sequences", "{}", configEEPROM.getConfigData());
        _commandInterpreter.setSequences(sequencesStr);
        configLoaded = true;
    }
    if (!configLoaded)
    {
        Log.info("Setting default configuration");
        _commandInterpreter.setSequences(ROBOT_DEFAULT_SEQUENCE_COMMANDS);
        _commandInterpreter.setPatterns(ROBOT_DEFAULT_PATTERN_COMMANDS);
    }

    // Check for cmdsAtStart
    String cmdsAtStart = ConfigManager::getString("cmdsAtStart", "", ROBOT_CONFIG_STR);
    _commandInterpreter.process(cmdsAtStart);
}

long initialMemory = System.freeMemory();
long lowestMemory = System.freeMemory();

// Local IP Addr as string
char _localIPStr[20];
char *localIPStr()
{
    IPAddress ipA = WiFi.localIP();

    sprintf(_localIPStr, "%d.%d.%d.%d", ipA[0], ipA[1], ipA[2], ipA[3]);
    return _localIPStr;
}
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
        if (lowestMemory > System.freeMemory())
            lowestMemory = System.freeMemory();
        if (loopTimeAvgWinLen > 0)
        {
            Log.info("Avg loop time %0.3fus (val %lu) initMem %d mem %d lowMem %d wkFlowItems %d canAccept %d IP %s",
            1.0 * loopWindowSumMicros / loopTimeAvgWinLen,
            lastLoopStartMicros, initialMemory,
            System.freeMemory(), lowestMemory,
            _workflowManager.numWaiting(),
            _commandInterpreter.canAcceptCommand(),
            localIPStr());
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

    // Service the command interpreter (which pumps the workflow queue)
    _commandInterpreter.service();

    // Service the robot controller
    _robotController.service();

    // Check if eeprom contents need to be written - which is a time consuming process
    if (eepromNeedsWriting)
    {
        // Check for robot idle, web server idle and no commands pending
        bool robotActive = _robotController.wasActiveInLastNSeconds(ROBOT_IDLE_BEFORE_WRITE_EEPROM_SECS);
        bool webActive = ((pWebServer) && (pWebServer->wasActiveInLastNSeconds(WEB_IDLE_BEFORE_WRITE_EEPROM_SECS)));
        if (!(robotActive || webActive))
        {
            Log.info("Writing to EEPROM - could take a few seconds ...");
            configEEPROM.writeToEEPROM();
            Log.info("Write to EEPROM done");
            eepromNeedsWriting = false;
        }
    }

    // Service the web server
    if (pWebServer)
    {
        pWebServer->service();
    }

}
