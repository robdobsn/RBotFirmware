// REST API for RBotFirmware
// Rob Dobson 2018

#include "RestAPIRobot.h"
#include "Utils.h"

void RestAPIRobot::apiQueryStatus(String &reqStr, String &respStr)
{
    _commandInterface.queryStatus(respStr);
}

void RestAPIRobot::apiGetRobotConfigurations(String &reqStr, String &respStr)
{
    Log.notice("RestAPIRobot: GetRobotConfigurations\n");
    RobotConfigurations::getRobotConfigurations(respStr);
}

void RestAPIRobot::apiRobotConfiguration(String &reqStr, String &respStr)
{
    Log.notice("RestAPIRobot: RobotConfiguration\n");
    respStr = RobotConfigurations::getConfig(reqStr.c_str());
}

void RestAPIRobot::apiGetSettings(String &reqStr, String &respStr)
{
    Log.notice("RestAPIRobot: GetSettings\n");
    _commandInterface.getRobotConfig(respStr);
}

void RestAPIRobot::apiPostSettings(String &reqStr, String &respStr)
{
    Log.notice("RestAPIRobot: PostSettings\n");
    // Result
    Utils::setJsonBoolResult(respStr, true);      
}

void RestAPIRobot::apiPostSettingsBody(String& reqStr, uint8_t *pData, size_t len, size_t index, size_t total)
{
    Log.notice("RestAPIRobot: PostSettings len %d\n", len);
    // Store the settings
    _commandInterface.setRobotConfig(pData, len);
}

void RestAPIRobot::apiExec(String &reqStr, String &respStr)
{
    Log.notice("RestAPIRobot: Exec\n");
    WorkItem workItem(RestAPIEndpoints::removeFirstArgStr(reqStr.c_str()).c_str());
    _commandInterface.addWorkItem(workItem, respStr);
}

void RestAPIRobot::apiPattern(String &reqStr, String &respStr)
{
    Log.notice("RestAPIRobot: Pattern\n");
    WorkItem workItem(RestAPIEndpoints::removeFirstArgStr(reqStr.c_str()).c_str());
    _commandInterface.addWorkItem(workItem, respStr);
}

void RestAPIRobot::apiSequence(String &reqStr, String &respStr)
{
    Log.notice("RestAPIRobot: Sequence\n");
    WorkItem workItem(RestAPIEndpoints::removeFirstArgStr(reqStr.c_str()).c_str());
    _commandInterface.addWorkItem(workItem, respStr);
}

void RestAPIRobot::apiPlayFile(String &reqStr, String &respStr)
{
    Log.notice("RestAPIRobot: playFile\n");
    WorkItem workItem(RestAPIEndpoints::removeFirstArgStr(reqStr.c_str()).c_str());
    _commandInterface.addWorkItem(workItem, respStr);
}

void RestAPIRobot::setup(RestAPIEndpoints &endpoints)
{
    // Get robot types
    endpoints.addEndpoint("getRobotConfigurations", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                            std::bind(&RestAPIRobot::apiGetRobotConfigurations, this, std::placeholders::_1, std::placeholders::_2),
                            "Get robot types");

    // Get robot type config
    endpoints.addEndpoint("getRobotTypeConfig", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
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

    // Run pattern
    endpoints.addEndpoint("pattern", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                            std::bind(&RestAPIRobot::apiPattern, this, std::placeholders::_1, std::placeholders::_2),
                            "Run pattern");

    // Run sequence
    endpoints.addEndpoint("sequence", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                            std::bind(&RestAPIRobot::apiSequence, this, std::placeholders::_1, std::placeholders::_2),
                            "Run sequence");

    // Get status
    endpoints.addEndpoint("status", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                            std::bind(&RestAPIRobot::apiQueryStatus, this, std::placeholders::_1, std::placeholders::_2),
                            "Query status");
};


