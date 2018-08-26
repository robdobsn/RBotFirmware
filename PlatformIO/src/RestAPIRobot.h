// REST API for RBotFirmware
// Rob Dobson 2018

#pragma once
#include <Arduino.h>
#include "RestAPIEndpoints.h"
#include "RobotController.h"
#include "ConfigNVS.h"
#include "RestAPISystem.h"

class RestAPIRobot
{
  private:
    RobotController &_robotController;
    ConfigNVS &_robotConfig;
    RestAPISystem &_restAPISystem;

  public:
    RestAPIRobot(RobotController &robotController, ConfigNVS &robotConfig, RestAPISystem &restAPISystem) : 
                _robotController(robotController), _robotConfig(robotConfig), _restAPISystem(restAPISystem)
    {
    }

    void apiQueryStatus(String &reqStr, String &respStr)
    {
        String innerJsonStr;
        int hashUsedBits = 0;
        // System health
        String healthStrSystem;
        hashUsedBits += _restAPISystem.reportHealth(hashUsedBits, NULL, &healthStrSystem);
        if (innerJsonStr.length() > 0)
            innerJsonStr += ",";
        innerJsonStr += healthStrSystem;
        // Robot info
        String healthStrRobot;
        hashUsedBits += reportHealth(hashUsedBits, NULL, &healthStrRobot);
        if (innerJsonStr.length() > 0)
            innerJsonStr += ",";
        innerJsonStr += healthStrRobot;
        // System information
        respStr = "{" + innerJsonStr + "}";
    }

    void apiRobotControl(String &reqStr, String &respStr)
    {
        // String shadeNumStr = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 1);
        // int shadeNum = shadeNumStr.toInt();
        // String shadeCmdStr = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 2);
        // String shadeDurationStr = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 3);

        // if ((shadeNum < 1) || (shadeNum > _windowShades.getMaxNumShades()))
        // {
        //     Utils::setJsonBoolResult(respStr, false);
        //     return;
        // }
        // int shadeIdx = shadeNum - 1;
        // //    if (!pWindowShades->canAcceptCommand(shadeIdx, shadeCmdStr))
        // //    {
        // //        retStr = String("{ \"rslt\": \"busy\" }");
        // //        return;
        // //    }
        // bool rslt = _windowShades.doCommand(shadeIdx, shadeCmdStr, shadeDurationStr);
        Utils::setJsonBoolResult(respStr, rslt);
    }

    void apiWipeConfig(String &reqStr, String &respStr)
    {
        // _shadesConfig.clear();
        // Log.notice("RestAPIShades: config cleared\n");
        // Utils::setJsonBoolResult(respStr, true);
    }

    void apiRobotConfig(String &reqStr, String &respStr)
    {
        // String configStr = "{";
        // // Window name
        // String shadeWindowName = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 1);
        // if (shadeWindowName.length() == 0)
        //     shadeWindowName = "Window Shades";
        // configStr.concat("\"name\":\"");
        // configStr.concat(shadeWindowName);
        // configStr.concat("\"");

        // // Number of shades
        // String numShadesStr = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 2);
        // int numShades = numShadesStr.toInt();
        // if (numShades < 1)
        // {
        //     numShades = 1;
        // }
        // if (numShades > windowShades.getMaxNumShades())
        // {
        //     numShades = windowShades.getMaxNumShades();
        // }
        // numShadesStr = String(numShades);
        // configStr.concat(",\"numShades\":\"");
        // configStr.concat(numShadesStr);
        // configStr.concat("\"");

        // // Shade names
        // for (int i = 0; i < numShades; i++)
        // {
        //     String shadeName = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 3 + i);
        //     if (shadeName.length() == 0)
        //         shadeName = "Shade " + String(i + 1);
        //     configStr.concat(",\"sh");
        //     configStr.concat(i);
        //     configStr.concat("\":\"");
        //     configStr.concat(shadeName);
        //     configStr.concat("\"");
        // }
        // configStr.concat("}");

        // // Debug
        // Log.trace("Writing config %s\n", configStr.c_str());

        // // Store in config
        // shadesConfig.setConfigData(configStr.c_str());
        // shadesConfig.writeConfig();

        // // Return the query result
        // String initialContent = "\"pgm\": \"Shades Control\",";
        // String healthStr = "";
        // reportHealth(0, NULL, &healthStr);
        respStr = "{" + initialContent + healthStr + "}";
    }

    void setup(RestAPIEndpoints &endpoints)
    {
        // Control shade
        endpoints.addEndpoint("shade", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                              std::bind(&RestAPIRobot::apiRobotControl, this, std::placeholders::_1, std::placeholders::_2),
                              "Control Shades - /1..N/up|stop|down/pulse|on|off");
        // Query status
        endpoints.addEndpoint("q", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                              std::bind(&RestAPIRobot::apiQueryStatus, this, std::placeholders::_1, std::placeholders::_2),
                              "Query status");
        // Wipe config
        endpoints.addEndpoint("wipeall", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                              std::bind(&RestAPIRobot::apiWipeConfig, this, std::placeholders::_1, std::placeholders::_2),
                              "Wipe shades configuration");
        // Shades config
        endpoints.addEndpoint("shadecfg", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET,
                              std::bind(&RestAPIRobot::apiRobotConfig, this, std::placeholders::_1, std::placeholders::_2),
                              "Set shades configuration");
    }

    int reportHealth(int bitPosStart, unsigned long *pOutHash, String *pOutStr)
    {
        // Get information on status
        String shadeWindowName = _robotConfig.getString("name", "");
        // if (shadeWindowName.length() == 0)
        //     shadeWindowName = "Window Shades";
        // // Log.notice("Shades name %s\n", shadeWindowName.c_str());
        // int numShades = _shadesConfig.getLong("numShades", 0);
        // if (numShades < 1)
        // {
        //     numShades = 1;
        // }
        // if (numShades > _windowShades.getMaxNumShades())
        // {
        //     numShades = _windowShades.getMaxNumShades();
        // }
        // String numShadesStr = String(numShades);

        // Generate hash if required - no changes
        if (pOutHash)
        {
            *pOutHash += 0;
        }

        // Generate JSON string if needed
        if (pOutStr)
        {
            // // Get light levels
            // String lightLevelsJSON = _windowShades.getLightLevelsJSON();

            // // Shades
            // String shadesDetailStr;
            // shadesDetailStr = "\"shades\": [";
            // // Add name for each shade
            // for (int i = 0; i < numShades; i++)
            // {
            //     String shadeName = _shadesConfig.getString(("sh" + String(i)).c_str(), "");
            //     if (shadeName.length() == 0)
            //         shadeName = "Shade " + String(i + 1);
            //     shadesDetailStr.concat("{\"name\": \"");
            //     shadesDetailStr.concat(shadeName);
            //     shadesDetailStr.concat("\", \"num\": \"");
            //     shadesDetailStr.concat(String(i + 1));
            //     bool isBusy = _windowShades.isBusy(i);
            //     shadesDetailStr.concat("\", \"busy\": \"");
            //     shadesDetailStr.concat(String(isBusy ? "1" : "0"));
            //     shadesDetailStr.concat("\"}");
            //     if (i != numShades - 1)
            //     {
            //         shadesDetailStr.concat(",");
            //     }
            // }
            // shadesDetailStr.concat("]");
            // // Shade name and number
            // String shadesStr = "\"numShades\":\"" + numShadesStr + "\",\"name\":\"" + shadeWindowName + "\"";
            // // Compile output string
            // String sOut = shadesStr + "," + shadesDetailStr + "," + lightLevelsJSON;
            // *pOutStr = sOut;
        }
        // Return number of bits in hash
        return 1;
    }
};
