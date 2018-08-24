// RBotFirmware
// Rob Dobson 2016-2017

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
const int webServerPort = 80;
WebServer webServer;

// MQTT
#include "MQTTManager.h"
MQTTManager mqttManager(wifiManager, restAPIEndpoints);

#include "RobotController.h"
#include "WorkflowManager.h"
#include "CommsSerial.h"
#include "RobotTypes.h"
#include "RBotDefaults.h"

// Hardware config
static const char *hwConfigJSON = {
    "{"
    "\"unitName\": \"Front Door\","
    "\"doorNames\":[\"Main\",\"Inner\"],"
    "\"doorConfig\":"
        "["
        "{\"strikePin\":\"D0\", \"strikeOn\":0,\"sensePin\":\"D3\", \"senseOpen\":0,\"unlockForSecs\":10},"
        "{\"strikePin\":\"D1\", \"strikeOn\":0,\"sensePin\":\"\", \"senseOpen\":1,\"unlockForSecs\":30}"
        "],"
    "\"bellSensePin\":\"D2\","
    "\"bellSensePress\":1,"
    "\"buzzerPin\":\"A4\","
    "\"buzzerOnLevel\":1,"
    "\"rfidEnable\":1,"
    "\"wifiLed\":{\"ledPin\":\"15\",\"ledOnMs\":200,\"ledShortOffMs\":200,\"ledLongOffMs\":750}"

    // "\"unitName\": \"Front Door\","
    // "\"doorNames\":[\"Main\",\"Inner\"],"
    // "\"strikePin0\":\"D0\","
    // "\"strikeOn0\":0,"
    // "\"sensePin0\":\"D3\","
    // "\"senseOpen0\":0,"
    // "\"unlockForSecs0\":10,"
    // "\"strikePin1\":\"D1\","
    // "\"strikeOn1\":0,"
    // "\"sensePin1\":\"\","
    // "\"senseOpen1\":1,"
    // "\"unlockForSecs1\":30,"
    // "\"bellSensePin\":\"D2\","
    // "\"bellSensePress\":1,"
    // "\"buzzerPin\":\"A4\","
    // "\"buzzerOnLevel\":1,"
    // "\"rfidEnable\":1"
    "}"
};

// Config for hardware
ConfigBase hwConfig(hwConfigJSON);

// Config for robot control
ConfigNVS robotConfig("robot", 9500);

// Config for WiFi
ConfigNVS wifiConfig("wifi", 100);

// Config for MQTT
ConfigNVS mqttConfig("mqtt", 200);

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

// workflow manager
WorkflowManager _workflowManager;

// Command interpreter
CommandInterpreter _commandInterpreter(&_workflowManager, &_robotController);

// Serial comms
// CommsSerial _commsSerial(0);

// REST API System
#include "RestAPISystem.h"
RestAPISystem restAPISystem;

// REST API Robot
//#include "RestAPIRobot.h"
//RestAPIRobot restAPIRobot(_robotController, _workflowManager, _commandInterpreter, mqttManager, restAPISystem);

// Debug loop used to time main loop
#include "DebugLoopTimer.h"

// Debug loop timer and callback function
void debugLoopInfoCallback(String &infoStr)
{
  String ipAddr = WiFi.localIP();
  infoStr = String::format(" IP %s MEM %d Q %d R %d", ipAddr.c_str(),
        System.freeMemory(), _workflowManager.numWaiting(),
        _commandInterpreter.canAcceptCommand());
  infoStr += _robotController.getDebugStr();

    infoStr = "RBot SSID " + WiFi.SSID() + " IP " + WiFi.localIP().toString() + 
         " Q " + String(_workflowManager.numWaiting()) + 
         " R " + String(_commandInterpreter.canAcceptCommand());
}
DebugLoopTimer debugLoopTimer(10000, debugLoopInfoCallback);

// Serial console - for configuration
#include "SerialConsole.h"
SerialConsole serialConsole;
const int SERIAL_PORT = 0;

// Setup
void setup()
{
    // Logging
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);

    // Message
    String systemName = "RBot";
    Log.notice("%s (built %s %s)\n", systemName.c_str(), __DATE__, __TIME__);

    // Status Led
    wifiStatusLed.setup(hwConfig, "wifiLed");

    // Setup door control
    doorControl.setup(hwConfig);

    // Door config
    doorConfig.setup();

    // User config
    userConfig.setup();

    // WiFi Config
    wifiConfig.setup();

    // MQTT Config
    mqttConfig.setup();

    // Add API endpoints
    restAPISystem.setup(restAPIEndpoints);
    restAPIDoor.setup(restAPIEndpoints);
    restAPIUser.setup(restAPIEndpoints);

    // Serial console
    serialConsole.setup(SERIAL_PORT, restAPIEndpoints);

    // WiFi Manager
    wifiManager.setup(&wifiConfig, systemName.c_str(), &wifiStatusLed);

    // Web server
    webServer.setup(webServerPort);
    webServer.addEndpoints(restAPIEndpoints);

    // MQTT
    mqttManager.setup(&mqttConfig);

    // Add debug blocks
    debugLoopTimer.blockAdd(0, "Web");
    debugLoopTimer.blockAdd(1, "Serial");
    debugLoopTimer.blockAdd(2, "Cmd");
    debugLoopTimer.blockAdd(3, "MQTT");
    debugLoopTimer.blockAdd(4, "Robot");

    // Reconfigure the robot and other settings
    // reconfigure();

    // Handle statup commands
    // handleStartupCommands();
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

    // Service commands
    if (wifiManager.isConnected())
    {
        // Service the command interpreter (which pumps the workflow queue)
        debugLoopTimer.blockStart(2);
        _commandInterpreter.service();
        debugLoopTimer.blockEnd(2);
    }

    // Service the status LED
    wifiStatusLed.service();

    // Serial console
    debugLoopTimer.blockStart(1);
    serialConsole.service();
    debugLoopTimer.blockEnd(1);

    // Service the robot controller
    debugLoopTimer.blockStart(4);
    _robotController.service();
    debugLoopTimer.blockEnd(4);

    // Service MQTT
    debugLoopTimer.blockStart(3);
    mqttManager.service();
    debugLoopTimer.blockEnd(3);
}
