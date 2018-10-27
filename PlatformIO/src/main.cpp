// RBotFirmware
// Rob Dobson 2016-2017

// System type
const char *systemType = "RBotFirmware";

// System version
const char *systemVersion = "2.002.001";

// Build date
const char *buildDate = __DATE__;

// Build date
const char *buildTime = __TIME__;

// Arduino
#include <Arduino.h>

// Logging
#include <ArduinoLog.h>

// WiFi
#include <WiFi.h>

// Utils
#include <Utils.h>

// Status LED
#include "StatusLed.h"
StatusLed wifiStatusLed;

// Config
#include "ConfigNVS.h"

// // WiFi Manager
#include "WiFiManager.h"
WiFiManager wifiManager;

// API Endpoints
#include "RestAPIEndpoints.h"
RestAPIEndpoints restAPIEndpoints;

// Web server
#include "WebServer.h"
WebServer webServer;

// MQTT
#include "MQTTManager.h"
MQTTManager mqttManager(wifiManager, restAPIEndpoints);

// MQTTLog
#include "NetLog.h"
NetLog netLog(Serial, mqttManager);

// Firmware update
#include <RdOTAUpdate.h>
RdOTAUpdate otaUpdate;

// Hardware config
static const char *hwConfigJSON = {
    "{"
    "\"wifiEnabled\":1,"
    "\"mqttEnabled\":0,"
    "\"webServerEnabled\":1,"
    "\"webServerPort\":80,"
    "\"OTAUpdate\":{\"enabled\":0,\"server\":\"domoticzoff\",\"port\":5076},"
    "\"wifiLed\":{\"ledPin\":\"\",\"ledOnMs\":200,\"ledShortOffMs\":200,\"ledLongOffMs\":750},"
    "\"serialConsole\":{\"portNum\":0},"
    "\"defaultRobotType\":\"SandTableScara\""
    "}"};

// Config for hardware
ConfigBase hwConfig(hwConfigJSON);

#include "RobotController.h"
#include "WorkflowManager.h"
#include "RobotTypes.h"

// Config for robot control
ConfigNVS robotConfig("robot", 9500);

// Config for WiFi
ConfigNVS wifiConfig("wifi", 100);

// Config for MQTT
ConfigNVS mqttConfig("mqtt", 200);

// Config for network logging
ConfigNVS netLogConfig("netLog", 200);

//define RUN_TESTS_CONFIG
//#define RUN_TEST_WORKFLOW
#ifdef RUN_TEST_CONFIG
#include "TestConfigManager.h"
#endif
#ifdef RUN_TEST_WORKFLOW
#include "TestWorkflowGCode.h"
#endif

// Robot controller
RobotController _robotController;

// Workflow manager
WorkflowManager _workflowManager;

// REST API System
#include "RestAPISystem.h"
RestAPISystem restAPISystem(wifiManager, mqttManager, otaUpdate, netLog, systemType, systemVersion);

// Command extender
#include "CommandExtender.h"
CommandExtender _commandExtender;

// Command interface
#include "CommandInterface.h"
CommandInterface _commandInterface(hwConfig,
                robotConfig, 
                _robotController,
                _workflowManager,
                restAPISystem,
                _commandExtender);

// REST API Robot
#include "RestAPIRobot.h"
RestAPIRobot restAPIRobot(_commandInterface);

// Debug loop used to time main loop
#include "DebugLoopTimer.h"

// Debug loop timer and callback function
void debugLoopInfoCallback(String &infoStr)
{
    if (wifiManager.isEnabled())
        infoStr = wifiManager.getHostname() + " SSID " + WiFi.SSID() + " IP " + WiFi.localIP().toString() + " Heap " + String(ESP.getFreeHeap());
    else
        infoStr = "Heap " + String(ESP.getFreeHeap());
    // infoStr += " Q " + String(_workflowManager.numWaiting()) + " R " + String(_commandInterpreter.canAcceptCommand();
    infoStr += _robotController.getDebugStr();
}
DebugLoopTimer debugLoopTimer(10000, debugLoopInfoCallback);

// Serial console - for configuration
#include "SerialConsole.h"
SerialConsole serialConsole;

// Setup
void setup()
{
    // Logging
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_NOTICE, &Serial);

    // Message
    Log.notice("%s %s (built %s %s)\n", systemType, systemVersion, buildDate, buildTime);

    // Status Led
    wifiStatusLed.setup(hwConfig, "wifiLed");

    // WiFi Config
    wifiConfig.setup();

    // MQTT Config
    mqttConfig.setup();

    // NetLog Config
    netLogConfig.setup();

    // Robot config
    robotConfig.setup();

    // Firmware update
    otaUpdate.setup(hwConfig, systemType, systemVersion);

    // Add API endpoints
    restAPISystem.setup(restAPIEndpoints);
    restAPIRobot.setup(restAPIEndpoints);

    // Serial console
    serialConsole.setup(hwConfig, restAPIEndpoints);

    // WiFi Manager
    wifiManager.setup(hwConfig, &wifiConfig, systemType, &wifiStatusLed);

    // Web server
    webServer.setup(hwConfig);
    webServer.addEndpoints(restAPIEndpoints);

    // MQTT
    mqttManager.setup(hwConfig, &mqttConfig);

    // Network logging
    netLog.setup(&netLogConfig, wifiManager.getHostname().c_str());

    // Command extender
    _commandExtender.setup(&_commandInterface);

    // Add debug blocks
    debugLoopTimer.blockAdd(0, "Web");
    debugLoopTimer.blockAdd(1, "Serial");
    debugLoopTimer.blockAdd(2, "MQTT");
    debugLoopTimer.blockAdd(3, "OTA");
    debugLoopTimer.blockAdd(4, "Robot");
    debugLoopTimer.blockAdd(5, "CMD");

    // Reconfigure the robot and other settings
    _commandInterface.reconfigure();

    // Handle statup commands
    _commandInterface.handleStartupCommands();
}

// Loop
void loop()
{

    // Debug loop Timing
    debugLoopTimer.service();

    // Service WiFi
    wifiManager.service();

    // Service the web server
    if (wifiManager.isConnected())
    {
        // Begin the web server
        debugLoopTimer.blockStart(0);
        webServer.begin(true);
        debugLoopTimer.blockEnd(0);
    }

    // Service the status LED
    wifiStatusLed.service();

    // Service the system API (restart)
    restAPISystem.service();

    // Serial console
    debugLoopTimer.blockStart(1);
    serialConsole.service();
    debugLoopTimer.blockEnd(1);

    // Service MQTT
    debugLoopTimer.blockStart(2);
    mqttManager.service();
    debugLoopTimer.blockEnd(2);

    // Service OTA Update
    debugLoopTimer.blockStart(3);
    otaUpdate.service();
    debugLoopTimer.blockEnd(3);

    // Service NetLog
    netLog.service();

    // Service the robot controller
    debugLoopTimer.blockStart(4);
    _robotController.service();
    debugLoopTimer.blockEnd(4);

    // Service the command interface (which pumps the workflow queue)
    debugLoopTimer.blockStart(5);
    _commandInterface.service();
    debugLoopTimer.blockEnd(5);
}
