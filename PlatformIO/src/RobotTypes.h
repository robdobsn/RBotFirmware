// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

#include "RdJson.h"

class RobotTypes
{
  public:
    static const char *_robotConfigs[];
    static const int _numRobotTypes;

    static const char *getConfig(const char *robotTypeName)
    {
        Log.notice("RobotTypes: Requesting %s, there are %d default types", robotTypeName, _numRobotTypes);
        for (int configIdx = 0; configIdx < _numRobotTypes; configIdx++)
        {
            String robotType = RdJson::getString("robotType", "", _robotConfigs[configIdx]);
            if (robotType.equals(robotTypeName))
            {
                Log.notice("RobotTypes: Config for %s found", robotTypeName);
                return _robotConfigs[configIdx];
            }
        }
        return "{}";
    }

    // Return list of robot types
    static const void getRobotTypes(String &retStr)
    {
        retStr = "[";
        for (int configIdx = 0; configIdx < _numRobotTypes; configIdx++)
        {
            String robotType = RdJson::getString("robotType", "", _robotConfigs[configIdx]);
            if (configIdx != 0)
                retStr += ",";
            retStr += "\"" + robotType + "\"";
        }
        retStr += "]";
    }
};
