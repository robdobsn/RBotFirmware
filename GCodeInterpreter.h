// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "ConfigManager.h"
#include "CommandElem.h"
#include "RobotController.h"

class GCodeInterpreter
{

public:
    GCodeInterpreter()
    {
    }

    ~GCodeInterpreter()
    {
    }

    static bool init(const char* configStr)
    {

    }

    static bool getCmdNumber(const char* pCmdStr, int& cmdNum)
    {
        // String passed in should start with a G or M
        // And be followed immediately by a number
        if (strlen(pCmdStr) < 1)
            return false;
        const char* pStr = pCmdStr + 1;
        if (!isdigit(*pStr))
            return false;
        long rsltStr = strtol(pStr, NULL, 10);
        cmdNum = (int) rsltStr;
        return true;
    }

    static bool getGcodeCmdArgs(const char* pArgStr, RobotCommandArgs& cmdArgs)
    {
        const char* pStr = pArgStr;
        char* pEndStr = NULL;
        while (*pStr)
        {
            switch(*pStr)
            {
                case 'X':
                    cmdArgs.xValid = true;
                    cmdArgs.xVal = strtod(++pStr, &pEndStr);
                    pStr = pEndStr;
                    break;
                case 'Y':
                    cmdArgs.yValid = true;
                    cmdArgs.yVal = strtod(++pStr, &pEndStr);
                    pStr = pEndStr;
                    break;
                case 'Z':
                    cmdArgs.zValid = true;
                    cmdArgs.zVal = strtod(++pStr, &pEndStr);
                    pStr = pEndStr;
                    break;
                case 'E':
                    cmdArgs.extrudeValid = true;
                    cmdArgs.extrudeVal = strtod(++pStr, &pEndStr);
                    pStr = pEndStr;
                    break;
                case 'F':
                    cmdArgs.feedrateValid = true;
                    cmdArgs.feedrateVal = strtod(++pStr, &pEndStr);
                    pStr = pEndStr;
                    break;
                case 'S':
                    {
                        int endstopIdx = strtol(++pStr, &pEndStr, 10);
                        pStr = pEndStr;
                        if (endstopIdx == 1)
                            cmdArgs.endstopEnum = RobotEndstopArg_Check;
                        else if (endstopIdx == 0)
                            cmdArgs.endstopEnum = RobotEndstopArg_Ignore;
                        break;
                    }
                default:
                    pStr++;
                    break;
            }
        }
    }

    // Interpret GCode G commands
    static bool interpG(String& cmdStr, RobotController& robotController, bool takeAction)
    {
        // Command string as a text buffer
        const char* pCmdStr = cmdStr.c_str();

        // Command number
        int cmdNum = 0;
        bool rslt = getCmdNumber(cmdStr, cmdNum);
        if (!rslt)
            return false;

        // Get args string (separated from the GXX or MXX by a space)
        const char* pArgsStr = "";
        const char* pArgsPos = strstr(pCmdStr, " ");
        if (pArgsPos != 0)
            pArgsStr = pArgsPos + 1;
        RobotCommandArgs cmdArgs;
        rslt = getGcodeCmdArgs(pArgsStr, cmdArgs);

        // Switch on number
        switch(cmdNum)
        {
            case 0: // Move absolute
            case 1: // Move absolute
                if (takeAction)
                {
                    cmdArgs.moveRapid = (cmdNum == 0);
                    robotController.moveTo(cmdArgs);
                }
                break;
        }

        return false;
    }

    // Interpret GCode M commands
    static bool interpM(String& cmdStr, RobotController& robotController, bool takeAction)
    {
        return false;
    }

    // Interpret GCode commands
    static bool interpretGcode(CommandElem& cmd, RobotController& robotController, bool takeAction)
    {
        // Extract code
        String cmdStr = cmd.getString().trim();
        if (cmdStr.length() == 0)
            return false;

        // Check for G or M codes
        if (cmdStr.charAt(0) == 'G')
            return interpG(cmdStr, robotController, takeAction);
        else if (cmdStr.charAt(0) == 'M')
            return interpM(cmdStr, robotController, takeAction);

        // Failed
        return false;
    }

};
