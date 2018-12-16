// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "../WorkItem.h"

class EvaluatorGCode
{

public:
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
        double rhoVal = 0;
        double thetaVal = 0;
        bool isRhoValid = false;
        bool isThetaValid = false;
        while (*pStr)
        {
            switch(toupper(*pStr))
            {
                case 'A':
                    cmdArgs.setAxisSteps(0, int(strtod(++pStr, &pEndStr)), true);
                    pStr = pEndStr;
                    break;
                case 'B':
                    cmdArgs.setAxisSteps(1, int(strtod(++pStr, &pEndStr)), true);
                    pStr = pEndStr;
                    break;
                case 'C':
                    cmdArgs.setAxisSteps(2, int(strtod(++pStr, &pEndStr)), true);
                    pStr = pEndStr;
                    break;
                case 'X':
                    cmdArgs.setAxisValMM(0, strtod(++pStr, &pEndStr), true);
                    pStr = pEndStr;
                    break;
                case 'Y':
                    cmdArgs.setAxisValMM(1, strtod(++pStr, &pEndStr), true);
                    pStr = pEndStr;
                    break;
                case 'Z':
                    cmdArgs.setAxisValMM(2, strtod(++pStr, &pEndStr), true);
                    pStr = pEndStr;
                    break;
                case 'E':
                    cmdArgs.setExtrude(strtod(++pStr, &pEndStr));
                    pStr = pEndStr;
                    break;
                case 'F':
                    cmdArgs.setFeedrate(strtod(++pStr, &pEndStr));
                    pStr = pEndStr;
                    break;
                case 'R':
                    cmdArgs.setMoveType(RobotMoveTypeArg_Relative);
                    pStr++;
                    break;
                case 'S':
                {
                        int endstopIdx = strtol(++pStr, &pEndStr, 10);
                        pStr = pEndStr;
                        if (endstopIdx == 1)
                            cmdArgs.setTestAllEndStops();
                        else if (endstopIdx == 0)
                            cmdArgs.setTestNoEndStops();
                        Log.verbose("Set to check endstops %s\n", cmdArgs.toJSON().c_str());
                        break;
                }
                case 'U':
                {
                    thetaVal = strtod(++pStr, &pEndStr);
                    pStr++;
                    isThetaValid = true;
                    break;
                }
                case 'V':
                {
                    rhoVal = strtod(++pStr, &pEndStr);
                        pStr++;
                    isRhoValid = true;
                    break;
                }
                default:
                {
                    pStr++;
                    break;
                }
            }
        }
        // Check for Theta-Rho values
        if (isThetaValid && isRhoValid)
        {
            cmdArgs.setAxisValThetaRho(0, thetaVal, true);
            cmdArgs.setAxisValThetaRho(1, rhoVal, true);
        }
	    return true;
    }

    // Interpret GCode G commands
    static bool interpG(String& cmdStr, RobotController* pRobotController, bool takeAction)
    {
        // Command string as a text buffer
        const char* pCmdStr = cmdStr.c_str();

        // Command number
        int cmdNum = 0;
        bool rslt = getCmdNumber(cmdStr.c_str(), cmdNum);
        if (!rslt)
            return false;

        // Get args string (separated from the GXX or MXX by a space)
        const char* pArgsStr = "";
        const char* pArgsPos = strstr(pCmdStr, " ");
        if (pArgsPos != 0)
            pArgsStr = pArgsPos + 1;
        RobotCommandArgs cmdArgs;
        rslt = getGcodeCmdArgs(pArgsStr, cmdArgs);

        Log.verbose("EvaluatorGCode Cmd G%d %s\n", cmdNum, pArgsStr);

        // Switch on number
        switch(cmdNum)
        {
            case 0: // Move rapid
            case 1: // Move
                if (takeAction)
                {
                    cmdArgs.setMoveRapid(cmdNum == 0);
                    pRobotController->moveTo(cmdArgs);
                }
                return true;
            case 6: // Direct stepper move
                if (takeAction)
                {
                    cmdArgs.setMoveRapid(true);
                    pRobotController->moveTo(cmdArgs);
                }
                return true;
            case 28: // Home axes
                if (takeAction)
                {
                    if (!cmdArgs.anyValid())
                      cmdArgs.setAllAxesNeedHoming();
                    pRobotController->goHome(cmdArgs);
                }
                return true;
            case 90: // Move absolute
                if (takeAction)
                {
                    cmdArgs.setMoveType(RobotMoveTypeArg_Absolute);
                    pRobotController->setMotionParams(cmdArgs);
                }
                return true;
            case 91: // Movements relative
                if (takeAction)
                {
                    cmdArgs.setMoveType(RobotMoveTypeArg_Relative);
                    pRobotController->setMotionParams(cmdArgs);
                }
                return true;
            case 92: // Set home
                if (takeAction)
                {
                    pRobotController->setHome(cmdArgs);
                }
                return true;
        }

        return false;
    }

    // Interpret GCode M commands
    static bool interpM(String& cmdStr, RobotController* pRobotController, bool takeAction)
    {
        return false;
    }

    // Interpret GCode commands
    static bool interpretGcode(WorkItem& workItem, RobotController* pRobotController, bool takeAction)
    {
        // Extract code
        String cmdStr = workItem.getString();
        cmdStr.trim();
        if (cmdStr.length() == 0)
            return false;

        // Check for G or M codes
        if (toupper(cmdStr.charAt(0)) == 'G')
            return interpG(cmdStr, pRobotController, takeAction);
        else if (toupper(cmdStr.charAt(0)) == 'M')
            return interpM(cmdStr, pRobotController, takeAction);

        // Failed
        return false;
    }

};
