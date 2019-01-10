// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "../WorkItem.h"
class RobotCommandArgs;
class RobotController;

class EvaluatorGCode
{

public:
    static bool getCmdNumber(const char* pCmdStr, int& cmdNum);
    static bool getGcodeCmdArgs(const char* pArgStr, RobotCommandArgs& cmdArgs);
    // Interpret GCode G commands
    static bool interpG(String& cmdStr, RobotController* pRobotController, bool takeAction);
    // Interpret GCode M commands
    static bool interpM(String& cmdStr, RobotController* pRobotController, bool takeAction);
    // Interpret GCode commands
    static bool interpretGcode(WorkItem& workItem, RobotController* pRobotController, bool takeAction);
};
