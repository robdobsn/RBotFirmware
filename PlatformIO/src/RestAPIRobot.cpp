// REST API for RBotFirmware
// Rob Dobson 2018

#include "RestAPIRobot.h"
#include "Utils.h"

static const char* MODULE_PREFIX = "RestAPIRobot: ";

void RestAPIRobot::apiQueryStatus(String &reqStr, String &respStr)
{
    _workManager.queryStatus(respStr);
}

void RestAPIRobot::apiGetRobotTypes(String &reqStr, String &respStr)
{
    Log.notice("%sGetRobotTypes\n", MODULE_PREFIX);
    RobotConfigurations::getRobotTypes(respStr);
}

void RestAPIRobot::apiRobotConfiguration(String &reqStr, String &respStr)
{
    Log.notice("%sRobotConfiguration\n", MODULE_PREFIX);
    String robotName = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 1);
    respStr = RobotConfigurations::getConfig(robotName.c_str());
}

void RestAPIRobot::apiGetSettings(String &reqStr, String &respStr)
{
    Log.verbose("%sGetSettings %s\n", MODULE_PREFIX, respStr.c_str());
    _workManager.getRobotConfig(respStr);
}

void RestAPIRobot::apiPostSettings(String &reqStr, String &respStr)
{
    Log.notice("%sPostSettings %s\n", MODULE_PREFIX, reqStr.c_str());
    // Result
    Utils::setJsonBoolResult(respStr, true);      
}

void RestAPIRobot::apiSetLed(String &reqStr, String &respStr)
{
    Log.notice("%sSetLed %s\n", MODULE_PREFIX, reqStr.c_str());
    // Result
    Utils::setJsonBoolResult(respStr, true);      
}

void RestAPIRobot::apiPostSettingsBody(String& reqStr, uint8_t *pData, size_t len, size_t index, size_t total)
{
    Log.notice("%sPostSettingsBody len %d\n", MODULE_PREFIX, len);
    // Store the settings
    _workManager.setRobotConfig(pData, len);
}

void RestAPIRobot::apiSetLedBody(String& reqStr, uint8_t *pData, size_t len, size_t index, size_t total)
{
    Log.notice("%sSetLedBody len %d, %s\n", MODULE_PREFIX, len, reqStr.c_str());
    // Store the settings
    _workManager.setLedStripConfig(pData, len);
}

void RestAPIRobot::apiExec(String &reqStr, String &respStr)
{
    Log.notice("%sExec %s\n", MODULE_PREFIX, reqStr.c_str());
    WorkItem workItem(RestAPIEndpoints::removeFirstArgStr(reqStr.c_str()).c_str());
    _workManager.addWorkItem(workItem, respStr);
}

void RestAPIRobot::apiPlayFile(String &reqStr, String &respStr)
{
    Log.notice("%splayFile %s\n", MODULE_PREFIX, reqStr.c_str());
    WorkItem workItem(RestAPIEndpoints::removeFirstArgStr(reqStr.c_str()).c_str());
    _workManager.addWorkItem(workItem, respStr);
}

void RestAPIRobot::setup(RestAPIEndpoints &endpoints)
{
    // Get robot types
    endpoints.addEndpoint("getRobotTypes", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                            std::bind(&RestAPIRobot::apiGetRobotTypes, this, std::placeholders::_1, std::placeholders::_2),
                            "Get robot types");

    // Get robot type config
    endpoints.addEndpoint("getRobotConfiguration", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                            std::bind(&RestAPIRobot::apiRobotConfiguration, this, std::placeholders::_1, std::placeholders::_2),
                            "Get config for a robot type");

    // Set robot settings
    endpoints.addEndpoint("postsettings", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_POST,
                            std::bind(&RestAPIRobot::apiPostSettings, this, std::placeholders::_1, std::placeholders::_2),
                            "Set settings for robot", "application/json", NULL, true, NULL, 
                            std::bind(&RestAPIRobot::apiPostSettingsBody, this, 
                            std::placeholders::_1, std::placeholders::_2, 
                            std::placeholders::_3, std::placeholders::_4,
                            std::placeholders::_5));

    // Get robot settings
    endpoints.addEndpoint("getsettings", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                            std::bind(&RestAPIRobot::apiGetSettings, this, std::placeholders::_1, std::placeholders::_2),
                            "Get settings for robot");

    // Exec command
    endpoints.addEndpoint("exec", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                            std::bind(&RestAPIRobot::apiExec, this, std::placeholders::_1, std::placeholders::_2),
                            "Exec robot command");

    // Play file
    endpoints.addEndpoint("playFile", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                            std::bind(&RestAPIRobot::apiPlayFile, this, std::placeholders::_1, std::placeholders::_2),
                            "Play file filename ... ~ for / in filename");
                            
    // Get status
    endpoints.addEndpoint("status", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                            std::bind(&RestAPIRobot::apiQueryStatus, this, std::placeholders::_1, std::placeholders::_2),
                            "Query status");
                            
    // Set LED Strip
    endpoints.addEndpoint("setled", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_POST,
                            std::bind(&RestAPIRobot::apiSetLed, this, std::placeholders::_1, std::placeholders::_2),
                            "Set LED Strip Settings", "application/json", NULL, true, NULL, 
                            std::bind(&RestAPIRobot::apiSetLedBody, this, 
                            std::placeholders::_1, std::placeholders::_2, 
                            std::placeholders::_3, std::placeholders::_4,
                            std::placeholders::_5));
};


