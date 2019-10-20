// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

#include "RdJson.h"

class RobotConfigurations
{
  public:
    static const char *_robotConfigs[];
    static const int _numRobotConfigurations;

    static const char *getConfig(const char *robotTypeName)
    {
        Log.notice("RobotConfigurations: Requesting %s, there are %d default types\n", robotTypeName, _numRobotConfigurations);
        for (int configIdx = 0; configIdx < _numRobotConfigurations; configIdx++)
        {
            String robotType = RdJson::getString("robotType", "", _robotConfigs[configIdx]);
            Log.verbose("RobotConfigurations: Testing %s against %s\n", robotType.c_str(), robotTypeName);
            if (robotType.equals(robotTypeName))
            {
                Log.trace("RobotConfigurations: Config for %s found\n", robotTypeName);
                Log.trace("RobotConfigurations: config str %s\n", _robotConfigs[configIdx]);
                return _robotConfigs[configIdx];
            }
        }
        Log.trace("RobotConfigurations: Config for %s not found\n", robotTypeName);
        return "{}";
    }

    // Return list of robot types
    static const void getRobotTypes(String &retStr)
    {
        retStr = "[";
        for (int configIdx = 0; configIdx < _numRobotConfigurations; configIdx++)
        {
            String robotType = RdJson::getString("robotType", "", _robotConfigs[configIdx]);
            if (configIdx != 0)
                retStr += ",";
            retStr += "\"" + robotType + "\"";
        }
        retStr += "]";
    }

    // Get Nth robot type name
    static void getNthRobotTypeName(int n, String& robotTypeName)
    {
        if ((n < 0) || (n > _numRobotConfigurations))
            robotTypeName = "";
        robotTypeName = RdJson::getString("robotType", "", _robotConfigs[n]);
    }
};
