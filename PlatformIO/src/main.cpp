// RBotFirmware
// Rob Dobson 2016-2018

// API used for web, MQTT and BLE (future)
//   Get version:    /v                   - returns version info
//   Set WiFi:       /w/ssss/pppp/hhhh    - ssss = ssid, pppp = password - assumes WPA2, hhhh = hostname
//                                        - does not clear previous WiFi so clear first if required
//   Clear WiFi:     /wc                  - clears all stored SSID, etc
//   Set MQTT:       /mq/ss/ii/oo/pp      - ss = server, ii and oo are in/out topics, pp = port
//                                        - in topics / should be replaced by ~
//                                        - (e.g. /devicename/in becomes ~devicename~in)
//   Check updates:  /checkupdate         - check for updates on the update server
//   Reset:          /reset               - reset device
//   Log level:      /loglevel/lll        - Logging level (for MQTT and HTTP)
//                                        - lll one of v (verbose), t (trace), n (notice), w (warning), e (error), f (fatal)
//   Log to MQTT:    /logmqtt/en/topic    - Control logging to MQTT
//                                        - en = 0 or 1 for off/on, topic is the topic logging messages are sent to
//   Log to HTTP:    /loghttp/en/ip/po/ur - Control logging to HTTP
//                                        - en = 0 or 1 for off/on
//                                        - ip is the IP address of the computer to log to (or hostname) and po is the port
//                                        - ur is the HTTP url logging messages are POSTed to
//   Log to serial:  /logserial/en/port   - Control logging to serial
//                                        - en = 0 or 1 for off/on
//                                        - port is the port number 0 = standard USB port
//   Log to cmd:     /logcmd/en           - Control logging to command port (extra serial if configured)
//                                        - en = 0 or 1 for off/on

// System type - this is duplicated here to make it easier for automated updater which parses the systemName = "aaaa" line
#define SYSTEM_TYPE_NAME "RBotFirmware"
const char* systemType = "RBotFirmware";

// System version
const char* systemVersion = "2.015.001";

// Build date
const char* buildDate = __DATE__;

// Build date
const char* buildTime = __TIME__;

// Arduino
#include <Arduino.h>

// Logging
#include <ArduinoLog.h>

// WiFi
#include <WiFi.h>

// Utils
#include <Utils.h>

// Status LED
#include "StatusIndicator.h"
StatusIndicator wifiStatusLed;

// Config
#include "ConfigNVS.h"
#include "ConfigFile.h"

// // WiFi Manager
#include "WiFiManager.h"
WiFiManager wifiManager;

// File manager
#include "FileManager.h"
FileManager fileManager;

// API Endpoints
#include "RestAPIEndpoints.h"
RestAPIEndpoints restAPIEndpoints;

// Web server
#include "WebServer.h"
#include "WebAutogenResources.h"
WebServer webServer;

// MQTT
#include "MQTTManager.h"
MQTTManager mqttManager(wifiManager, restAPIEndpoints);

// Firmware update
#include <RdOTAUpdate.h>
RdOTAUpdate otaUpdate;

// Hardware config
static const char *hwConfigJSON = {
    "{"
    "\"unitName\":" SYSTEM_TYPE_NAME ","
    "\"wifiEnabled\":1,"
    "\"mqttEnabled\":0,"
    "\"webServerEnabled\":1,"
    "\"webServerPort\":80,"
    "\"OTAUpdate\":{\"enabled\":1,\"server\":\"domoticzoff\",\"port\":5076,\"directOk\":1},"
    "\"serialConsole\":{\"portNum\":0},"
    "\"commandSerial\":{\"portNum\":-1,\"baudRate\":115200},"
    "\"defaultRobotType\":\"SandTableScara\""
    "}"
};

// Config for hardware
ConfigBase hwConfig(hwConfigJSON);

// Config for robot control
ConfigNVS robotConfig("robot", 2000);

// Config for WiFi
ConfigNVS wifiConfig("wifi", 100);

// Config for MQTT
ConfigNVS mqttConfig("mqtt", 200);

// Config for network logging
ConfigNVS netLogConfig("netLog", 200);

// Config for LED Strip
ConfigNVS ledStripConfig("ledStrip", 100);

// LED Strip
#include "LedStrip.h"
LedStrip ledStrip(ledStripConfig);

// CommandSerial port - used to monitor activity remotely and send commands
#include "CommandSerial.h"
CommandSerial commandSerial(fileManager);

// Serial console - for configuration
#include "SerialConsole.h"
SerialConsole serialConsole;

// NetLog
#include "NetLog.h"
NetLog netLog(Serial, mqttManager, commandSerial);

// REST API System
#include "RestAPISystem.h"
RestAPISystem restAPISystem(wifiManager, mqttManager,
                            otaUpdate, netLog, fileManager,
                            systemType, systemVersion);

// Robot controller
#include "RobotMotion/RobotController.h"
RobotController _robotController;

// Command interface
#include "WorkManager/WorkManager.h"
WorkManager _workManager(hwConfig,
                robotConfig, 
                _robotController,
                ledStrip,
                restAPISystem,
                fileManager);

// REST API Robot
#include "RestAPIRobot.h"
RestAPIRobot restAPIRobot(_workManager, fileManager);

// Debug loop used to time main loop
#include "DebugLoopTimer.h"

// Debug loop timer and callback function
void debugLoopInfoCallback(String &infoStr)
{
    if (wifiManager.isEnabled())
        infoStr = wifiManager.getHostname() + " V" + String(systemVersion) + " SSID " + WiFi.SSID() + " IP " + WiFi.localIP().toString() + " Heap " + String(ESP.getFreeHeap());
    else
        infoStr = "WiFi Disabled, Heap " + String(ESP.getFreeHeap());
    infoStr += _workManager.getDebugStr();
    infoStr += _robotController.getDebugStr();
}
DebugLoopTimer debugLoopTimer(10000, debugLoopInfoCallback);

// Setup
void setup()
{
    // Logging
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_TRACE, &netLog);

    // Message
    Log.notice("%s %s (built %s %s)\n", systemType, systemVersion, buildDate, buildTime);

    // Robot config
    robotConfig.setup();

    // Status Led
    wifiStatusLed.setup(&robotConfig, "robotConfig/wifiLed");

    // File system
    fileManager.setup(robotConfig, "robotConfig/fileManager");

    // WiFi Config
    wifiConfig.setup();

    // MQTT Config
    mqttConfig.setup();

    // NetLog Config
    netLogConfig.setup();

    // Led Strip Config
    ledStripConfig.setup();

    // Led Strip
    ledStrip.setup(&robotConfig, "robotConfig/ledStrip");

    // Serial console
    serialConsole.setup(hwConfig, restAPIEndpoints);

    // WiFi Manager
    wifiManager.setup(hwConfig, &wifiConfig, systemType, &wifiStatusLed);

    // Firmware update
    otaUpdate.setup(hwConfig, systemType, systemVersion);

    // Add API endpoints
    restAPISystem.setup(restAPIEndpoints);
    restAPIRobot.setup(restAPIEndpoints);

    // Web server
    webServer.setup(hwConfig);
    webServer.addStaticResources(__webAutogenResources, __webAutogenResourcesCount);
    webServer.addEndpoints(restAPIEndpoints);
    webServer.serveStaticFiles("/files/spiffs", "/spiffs/");
    webServer.serveStaticFiles("/files/sd", "/sd/");

    // MQTT
    mqttManager.setup(hwConfig, &mqttConfig);

    // Network logging
    netLog.setup(&netLogConfig, wifiManager.getHostname().c_str());

    // Add debug blocks
    debugLoopTimer.blockAdd(0, "Web");
    debugLoopTimer.blockAdd(1, "Console");
    debugLoopTimer.blockAdd(2, "MQTT");
    debugLoopTimer.blockAdd(3, "OTA");
    debugLoopTimer.blockAdd(4, "Robot");
    debugLoopTimer.blockAdd(5, "CMD");

    // Reconfigure the robot and other settings
    _workManager.reconfigure();

    // Handle statup commands
    _workManager.handleStartupCommands();
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

    // Service the LED Strip
    ledStrip.service();

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
    netLog.service(serialConsole.getXonXoff());

    // Service the robot controller
    debugLoopTimer.blockStart(4);
    _robotController.service();
    debugLoopTimer.blockEnd(4);

    // Service the command interface (which pumps the workflow queue)
    debugLoopTimer.blockStart(5);
    _workManager.service();
    debugLoopTimer.blockEnd(5);
}
