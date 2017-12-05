// RBotFirmware
// Rob Dobson 2016-2017

#include "ConfigEEPROM.h"
#include "RobotController.h"
#include "WorkflowManager.h"
#include "CommsSerial.h"
#include "ParticleCloud.h"
#include "DebugLoopTimer.h"

// Web server
#include "RdWebServer.h"
const int webServerPort = 80;
RdWebServer* pWebServer = NULL;
#include "GenResources.h"

// API Endpoints
RestAPIEndpoints restAPIEndpoints;

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

// Application info
static const char* APPLICATION_NAME = "RBotFirmware";
SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// Robot controller
RobotController _robotController;

// workflow manager
WorkflowManager _workflowManager;

// Command interpreter
CommandInterpreter _commandInterpreter(&_workflowManager, &_robotController);

// Serial comms
CommsSerial _commsSerial(0);

// Note that the value here for maxLen must be bigger than the value returned for restAPI_GetSettings()
// This is to ensure the web-app doesn't return a string that is too long
static const int EEPROM_CONFIG_BASE = 0;
static const int EEPROM_CONFIG_MAXLEN = 2010;

// Configuration
ConfigManager configManager;
ConfigEEPROM configPersistence(EEPROM_CONFIG_BASE, EEPROM_CONFIG_MAXLEN);

// Debug loop timer and callback function
void debugLoopInfoCallback(String& infoStr)
{
  String ipAddr = WiFi.localIP();
  infoStr = String::format(" IP %s MEM %d Q %d R %d", ipAddr.c_str(),
        System.freeMemory(), _workflowManager.numWaiting(),
        _commandInterpreter.canAcceptCommand());
}
DebugLoopTimer debugLoopTimer(10000, debugLoopInfoCallback);

// Particle Cloud
ParticleCloud* pParticleCloud = NULL;

// Time to wait before making a pending write to EEPROM - which takes up-to several seconds and
// blocks motion and web activity
static const unsigned long ROBOT_IDLE_BEFORE_WRITE_EEPROM_SECS = 5;
static const unsigned long WEB_IDLE_BEFORE_WRITE_EEPROM_SECS = 5;
static const unsigned long MAX_MS_CONFIG_DIRTY = 10000;
unsigned long configDirtyStartMs = 0;
#define WRITE_TO_EEPROM_ENABLED 1

// Mugbot on PiHat 1.1
// linear axis 1/8 microstepping,
// rotary axis 1/16 microstepping
// static const char* ROBOT_CONFIG_STR_MUGBOT_PIHAT_1_1 =
//     "{\"robotType\": \"MugBot\", \"xMaxMM\":150, \"yMaxMM\":120, "
//     " \"stepEnablePin\":\"D4\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":60.0,"
//     " \"axis0\": { \"stepPin\": \"A7\", \"dirnPin\":\"A6\", \"maxSpeed\":5.0, \"acceleration\":2.0,"
//     " \"stepsPerRotation\":3200, \"unitsPerRotation\":360, \"minVal\":0, \"maxVal\":240}, "
//     " \"axis1\": { \"stepPin\": \"A5\", \"dirnPin\":\"A4\", \"maxSpeed\":75.0, \"acceleration\":5.0,"
//     " \"stepsPerRotation\":1600, \"unitsPerRotation\":2.0, \"minVal\":0, \"maxVal\":78, "
//     " \"endStop0\": { \"sensePin\": \"D7\", \"activeLevel\":1, \"inputType\":\"INPUT_PULLUP\"}},"
//     " \"axis2\": { \"servoPin\": \"D0\", \"isServoAxis\": 1, \"homeOffsetVal\": 120, \"homeOffsetSteps\": 1666,"
//     " \"minVal\":0, \"maxVal\":180, \"stepsPerRotation\":2000, \"unitsPerRotation\":360 },"
//     "}";
//
// static const char* ROBOT_CONFIG_STR_GEISTBOT =
//     "{\"robotType\": \"GeistBot\", \"xMaxMM\":400, \"yMaxMM\":400, "
//     " \"stepEnablePin\":\"A2\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
//     " \"maxHomingSecs\":120, \"homingLinOffsetDegs\":70, \"homingCentreOffsetMM\":4,"
//     " \"homingRotCentreDegs\":3.7, \"cmdsAtStart\":\"G28;ModSpiral\", "
//     " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":75.0, \"acceleration\":5.0,"
//     " \"stepsPerRotation\":12000, \"unitsPerRotation\":360, \"isDominantAxis\":1,"
//     " \"endStop0\": { \"sensePin\": \"A6\", \"activeLevel\":1, \"inputType\":\"INPUT_PULLUP\"}},"
//     " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":75.0, \"acceleration\":5.0,"
//     " \"stepsPerRotation\":12000, \"unitsPerRotation\":44.8, \"minVal\":0, \"maxVal\":195, "
//     " \"endStop0\": { \"sensePin\": \"A7\", \"activeLevel\":0, \"inputType\":\"INPUT_PULLUP\"}},"
//     "}";
//
// static const char* ROBOT_CONFIG_STR_SANDTABLESCARA =
//     "{\"robotType\": \"SandTableScara\", \"xMaxMM\":190, \"yMaxMM\":190, "
//     " \"stepEnablePin\":\"A2\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
//     " \"blockDistanceMM\":1.0, \"homingAxis1OffsetDegs\":20.0,"
//     " \"maxHomingSecs\":120, \"cmdsAtStart\":\"G28\","
//     " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":75.0, \"acceleration\":5.0,"
//     " \"minNsBetweenSteps\":1000000,"
//     " \"stepsPerRotation\":9600, \"unitsPerRotation\":628.318,"
//     " \"endStop0\": { \"sensePin\": \"A6\", \"activeLevel\":0, \"inputType\":\"INPUT_PULLUP\"}},"
//     " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":75.0, \"acceleration\":5.0,"
//     " \"minNsBetweenSteps\":1000000,"
//     " \"stepsPerRotation\":9600, \"unitsPerRotation\":628.318, \"homeOffsetSteps\": 0,"
//     " \"endStop0\": { \"sensePin\": \"A7\", \"activeLevel\":0, \"inputType\":\"INPUT_PULLUP\"}},"
//     "}";

// static const char* ROBOT_CONFIG_STR_AIRHOCKEY =
//     "{\"robotType\": \"HockeyBot\", \"xMaxMM\":350, \"yMaxMM\":400, "
//     " \"stepEnablePin\":\"A2\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
//     " \"cmdsAtStart\":\"\", "
//     " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":5000.0, \"acceleration\":5000.0,"
//     " \"stepsPerRotation\":3200, \"unitsPerRotation\":62, \"minNsBetweenSteps\":1000},"
//     " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":5000.0, \"acceleration\":5000.0,"
//     " \"stepsPerRotation\":3200, \"unitsPerRotation\":62, \"minNsBetweenSteps\":1000}"
//     "}";

static const char* ROBOT_CONFIG_STR_XY =
    "{\"robotType\": \"XYBot\", \"xMaxMM\":500, \"yMaxMM\":500, "
    " \"stepEnablePin\":\"A2\", \"stepEnableActiveLevel\":1, \"stepDisableSecs\":1.0,"
    " \"cmdsAtStart\":\"\", "
    " \"axis0\": { \"stepPin\": \"D2\", \"dirnPin\":\"D3\", \"maxSpeed\":100.0, \"acceleration\":10.0,"
    " \"stepsPerRotation\":3200, \"unitsPerRotation\":60, \"minNsBetweenSteps\":10000},"
    " \"axis1\": { \"stepPin\": \"D4\", \"dirnPin\":\"D5\", \"maxSpeed\":100.0, \"acceleration\":10.0,"
    " \"stepsPerRotation\":3200, \"unitsPerRotation\":60, \"minNsBetweenSteps\":10000}"
    "}";

static const char* ROBOT_DEFAULT_SEQUENCE_COMMANDS =
    "{}";

static const char* ROBOT_DEFAULT_PATTERN_COMMANDS =
    "{}";
    /*" \"pattern1\":"
    "  {"
    "  \"setup\":\"angle=0;diam=10\","
    "  \"loop\":\"x=diam*sin(angle*3);y=diam*cos(angle*3);diam=diam+0.5;angle=angle+0.0314;stop=angle>6.28\""
    "  }"
    "}";*/

static const char* ROBOT_CONFIG_STR = ROBOT_CONFIG_STR_XY;

static const char* WORKFLOW_CONFIG_STR =
    "{\"CommandQueue\": { \"cmdQueueMaxLen\":50 } }";

// Post settings information via API
void restAPI_PostSettings(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    Log.trace("RestAPI PostSettings method %d contentLen %d", apiMsg._method, apiMsg._msgContentLen);
    if (apiMsg._pMsgHeader)
        Log.trace("RestAPI PostSettings header len %d", strlen(apiMsg._pMsgHeader));
    // Store the settings
    configManager.setConfigData((const char*)apiMsg._pMsgContent);
    // Set flag to indicate EEPROM needs to be written
    configPersistence.setDirty();
    // Apply the config data
    String patternsStr = RdJson::getString("/patterns", "{}", configManager.getConfigData());
    _commandInterpreter.setPatterns(patternsStr);
    String sequencesStr = RdJson::getString("/sequences", "{}", configManager.getConfigData());
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
    String runAtStart = RdJson::getString("startup", "", configManager.getConfigData());
    Log.trace("RestAPI GetSettings patterns %s", patterns);
    Log.trace("RestAPI GetSettings sequences %s", sequences);
    Log.trace("RestAPI GetSettings startup %s", runAtStart.c_str());
    retStr = "{\"maxCfgLen\":2000, \"name\":\"Sand Table\",\"patterns\":";
    retStr += patterns;
    retStr += ", \"sequences\":";
    retStr += sequences;
    retStr += ", \"startup\":\"";
    RdJson::escapeString(runAtStart);
    retStr += runAtStart.c_str();
    retStr += "\"}";
}

// Exec command via API
void restAPI_Exec(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    Log.trace("RestAPI Exec method %d contentLen %d", apiMsg._method, apiMsg._msgContentLen);
    _commandInterpreter.process(apiMsg._pArgStr);
    // Result
    retStr = "{\"ok\"}";
}

// Start Pattern via API
void restAPI_Pattern(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    Log.trace("RestAPI Pattern method %d contentLen %d", apiMsg._method, apiMsg._msgContentLen);
    _commandInterpreter.process(apiMsg._pArgStr);
    // Result
    retStr = "{\"ok\"}";
}

// Start sequence via API
void restAPI_Sequence(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    Log.trace("RestAPI Sequence method %d contentLen %d", apiMsg._method, apiMsg._msgContentLen);
    _commandInterpreter.process(apiMsg._pArgStr);
    // Result
    retStr = "{\"ok\"}";
}

// Get machine status
void restAPI_Status(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    RobotCommandArgs cmdArgs;
    _robotController.getCurStatus(cmdArgs);
    retStr = cmdArgs.toJSON();
}

// Exec via particle function
const char* particleAPI_Exec(const char* cmdStr)
{
    Log.trace("ParticleAPI Exec method %s", cmdStr);
    _commandInterpreter.process(cmdStr);
    // Result
    return "{\"ok\"}";
}

void setup()
{
    Serial.begin(115200);
    delay(5000);
    Log.info("%s (built %s %s)", APPLICATION_NAME, __DATE__, __TIME__);
    Log.info("System version: %s", (const char*)System.version());

    // Initialise the config manager
    delay(5000);
    const char* pConfig = configPersistence.read().c_str();
    Utils::logLongStr("Main: ConfigStr", pConfig, true);
    configManager.setConfigData(pConfig);

    #ifdef RUN_TEST_CONFIG
    TestConfigManager::runTests();
    #endif

    // Add API endpoints
    restAPIEndpoints.addEndpoint("postsettings", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_PostSettings, "");
    restAPIEndpoints.addEndpoint("getsettings", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_GetSettings, "");
    restAPIEndpoints.addEndpoint("exec", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_Exec, "");
    restAPIEndpoints.addEndpoint("pattern", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_Pattern, "");
    restAPIEndpoints.addEndpoint("sequence", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_Sequence, "");
    restAPIEndpoints.addEndpoint("status", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_Status, "");

    // Construct web server
    Log.info("Main: Constructing Web Server");
    pWebServer = new RdWebServer();

    // Configure web server
    if (pWebServer)
    {
        // Add resources to web server
        pWebServer->addStaticResources(genResources, genResourcesCount);
        pWebServer->addRestAPIEndpoints(&restAPIEndpoints);
        // Start the web server
        Log.info("Main: Starting Web Server");
        pWebServer->start(webServerPort);
    }

    // Particle Cloud
    pParticleCloud = new ParticleCloud(APPLICATION_NAME, particleAPI_Exec,
                            particleAPI_ReportHealth, particleAPI_ReportHealthHash);
    pParticleCloud->RegisterVariables();

    // Init robot controller and workflow manager
    _robotController.init(ROBOT_CONFIG_STR);
    _workflowManager.init(WORKFLOW_CONFIG_STR);

    // Configure the command interpreter
    bool configLoaded = false;
    if (*pConfig == '{' || *pConfig == '[')
    {
        Log.info("Main setting config");
        String patternsStr = RdJson::getString("/patterns", "{}", configManager.getConfigData());
        _commandInterpreter.setPatterns(patternsStr);
        Log.info("Main patterns %s", patternsStr.c_str());
        String sequencesStr = RdJson::getString("/sequences", "{}", configManager.getConfigData());
        _commandInterpreter.setSequences(sequencesStr);
        Log.info("Main sequences %s", sequencesStr.c_str());
        configLoaded = true;
    }
    if (!configLoaded)
    {
        Log.info("Main setting default configuration");
        _commandInterpreter.setSequences(ROBOT_DEFAULT_SEQUENCE_COMMANDS);
        _commandInterpreter.setPatterns(ROBOT_DEFAULT_PATTERN_COMMANDS);
    }

    // Check for cmdsAtStart in the robot config
    String cmdsAtStart = RdJson::getString("cmdsAtStart", "", ROBOT_CONFIG_STR);
    Log.info("Main cmdsAtStart <%s>", cmdsAtStart.c_str());
    if (cmdsAtStart.length() > 0)
        _commandInterpreter.process(cmdsAtStart);

    // Check for startup commands in the EEPROM config
    String runAtStart = RdJson::getString("startup", "", configManager.getConfigData());
    RdJson::unescapeString(runAtStart);
    Log.info("Main startup commands <%s>", runAtStart.c_str());
    if (runAtStart.length() > 0)
        _commandInterpreter.process(runAtStart);

    // Add debug blocks
    debugLoopTimer.blockAdd(0, "Serial");
    debugLoopTimer.blockAdd(1, "Cmd");
    debugLoopTimer.blockAdd(2, "Web");
    debugLoopTimer.blockAdd(3, "Robot");
    debugLoopTimer.blockAdd(4, "Cloud");
    debugLoopTimer.blockAdd(5, "EEPROM");
    debugLoopTimer.blockAdd(6, "Dirty");
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

// String to hold status
String particleAPI_statusStr;

// Get status
char* particleAPI_ReportHealth(const char* pIdStr,
                const char* initialContentJsonElementList)
{
    particleAPI_statusStr = String::format("IP Addr %s, Low RAM %d, Workflow %d, Ready %d",
                localIPStr(), lowestMemory,
                _workflowManager.numWaiting(),
                _commandInterpreter.canAcceptCommand());
    return (char*) particleAPI_statusStr.c_str();
}

unsigned long particleAPI_ReportHealthHash()
{
    particleAPI_ReportHealth(NULL, NULL);
    unsigned long hashVal = 0;
    for (unsigned int i = 0; i < particleAPI_statusStr.length(); i++)
    {
        hashVal += particleAPI_statusStr.charAt(i);
    }
    return hashVal;
}

void loop()
{

    #ifdef RUN_TEST_WORKFLOW
    // TEST add to command queue
    __testWorkflowGCode.testLoop(_workflowManager, _robotController);
    #endif

    // Debug loop Timing
    debugLoopTimer.Service();

    // See if in listening mode - if so don't steal characters
    if (!WiFi.listening())
    {
        // Service CommsSerial
        debugLoopTimer.blockStart(0);
        _commsSerial.service(_commandInterpreter);
        debugLoopTimer.blockEnd(0);

        // Service the command interpreter (which pumps the workflow queue)
        debugLoopTimer.blockStart(1);
        _commandInterpreter.service();
        debugLoopTimer.blockEnd(1);

        // Service the web server
        if (pWebServer)
        {
            debugLoopTimer.blockStart(2);
            pWebServer->service();
            debugLoopTimer.blockEnd(2);
        }
        // Service the robot controller
        debugLoopTimer.blockStart(3);
        _robotController.service();
        debugLoopTimer.blockEnd(3);

    }

    // Service the particle cloud
    debugLoopTimer.blockStart(4);
    if (pParticleCloud)
        pParticleCloud->Service();
    debugLoopTimer.blockEnd(4);

    // Service the EEPROM write
    debugLoopTimer.blockStart(5);
    configPersistence.service();
    debugLoopTimer.blockEnd(5);

    // Check if eeprom contents need to be written - which is a time consuming process
    #ifdef WRITE_TO_EEPROM_ENABLED
    // Handle writing of EEPROM
    debugLoopTimer.blockStart(6);
    if (configPersistence.isDirty())
    {
        // This code attempts to ensure that the config never goes very long without being written
        bool configMustWrite = false;
        if (configDirtyStartMs == 0)
        {
            configDirtyStartMs = millis();
        }
        else
        {
            if (Utils::isTimeout(millis(), configDirtyStartMs, MAX_MS_CONFIG_DIRTY))
            {
                Log.info("RBotFirmware: ConfigMustWrite");
                configMustWrite = true;
                configDirtyStartMs = 0;
            }
        }
        // Check for web server idle and attempt to write when it is idle
        bool robotActive = _robotController.wasActiveInLastNSeconds(ROBOT_IDLE_BEFORE_WRITE_EEPROM_SECS);
        bool webActive = ((pWebServer) && (pWebServer->wasActiveInLastNSeconds(WEB_IDLE_BEFORE_WRITE_EEPROM_SECS)));
        if ((configMustWrite || (!robotActive && !webActive) || (ROBOT_IDLE_BEFORE_WRITE_EEPROM_SECS == 0 && WEB_IDLE_BEFORE_WRITE_EEPROM_SECS == 0)))
            configPersistence.write(configManager.getConfigData());

    }
    debugLoopTimer.blockEnd(6);
    #endif
}
