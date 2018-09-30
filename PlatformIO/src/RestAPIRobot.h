// REST API for RBotFirmware
// Rob Dobson 2018

#pragma once
#include <Arduino.h>
#include "RestAPIEndpoints.h"
#include "CommandInterface.h"
#include "RobotTypes.h"

class RestAPIRobot
{
  private:
    CommandInterface &_commandInterface;

  public:
    RestAPIRobot(CommandInterface &commandInterface) : _commandInterface(commandInterface)
    {
    }
 
    void apiQueryStatus(String &reqStr, String &respStr)
    {
        _commandInterface.queryStatus(respStr);
    }

    void apiGetRobotTypes(String &reqStr, String &respStr)
    {
        Log.notice("RestAPIRobot: GetRobotTypes\n");
        RobotTypes::getRobotTypes(respStr);
    }

    void apiGetRobotTypeConfig(String &reqStr, String &respStr)
    {
        Log.notice("RestAPIRobot: GetRobotTypeConfig\n");
        respStr = RobotTypes::getConfig(reqStr.c_str());
    }

    void apiGetSettings(String &reqStr, String &respStr)
    {
        Log.notice("RestAPIRobot: GetSettings\n");
        _commandInterface.getRobotConfig(respStr);
    }

    void apiPostSettings(String &reqStr, String &respStr)
    {
        Log.notice("RestAPIRobot: PostSettings\n");
        // Result
        Utils::setJsonBoolResult(respStr, true);      
    }

    void apiPostSettingsBody(uint8_t *pData, size_t len, size_t index, size_t total)
    {
        Log.notice("RestAPIRobot: PostSettings len %d\n", len);
        // Store the settings
        _commandInterface.setRobotConfig(pData, len);
    }

    void apiExec(String &reqStr, String &respStr)
    {
        Log.notice("RestAPIRobot: Exec\n");
        _commandInterface.process(RestAPIEndpoints::removeFirstArgStr(reqStr.c_str()).c_str(), respStr);
    }

    void apiPattern(String &reqStr, String &respStr)
    {
        Log.notice("RestAPIRobot: Pattern\n");
        _commandInterface.process(RestAPIEndpoints::removeFirstArgStr(reqStr.c_str()).c_str(), respStr);
    }

    void apiSequence(String &reqStr, String &respStr)
    {
        Log.notice("RestAPIRobot: Sequence\n");
        _commandInterface.process(RestAPIEndpoints::removeFirstArgStr(reqStr.c_str()).c_str(), respStr);
    }

     void setup(RestAPIEndpoints &endpoints)
    {
        // Get robot types
        endpoints.addEndpoint("getRobotTypes", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                              std::bind(&RestAPIRobot::apiGetRobotTypes, this, std::placeholders::_1, std::placeholders::_2),
                              "Get robot types");

        // Get robot type config
        endpoints.addEndpoint("getRobotTypeConfig", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                              std::bind(&RestAPIRobot::apiGetRobotTypeConfig, this, std::placeholders::_1, std::placeholders::_2),
                              "Get config for a robot type");

        // Set robot settings
        endpoints.addEndpoint("postsettings", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_POST,
                              std::bind(&RestAPIRobot::apiPostSettings, this, std::placeholders::_1, std::placeholders::_2),
                              "Set settings for robot", "application/json", NULL, true, NULL, 
                              std::bind(&RestAPIRobot::apiPostSettingsBody, this, std::placeholders::_1, std::placeholders::_2, 
                                                                                    std::placeholders::_3, std::placeholders::_4));

        // Get robot settings
        endpoints.addEndpoint("getsettings", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                              std::bind(&RestAPIRobot::apiGetSettings, this, std::placeholders::_1, std::placeholders::_2),
                              "Get settings for robot");

        // Exec command
        endpoints.addEndpoint("exec", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                              std::bind(&RestAPIRobot::apiExec, this, std::placeholders::_1, std::placeholders::_2),
                              "Exec robot command");

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

    }


};