
#include "CommandInterface.h"
#include "ConfigNVS.h"
#include "RobotController.h"
#include "WorkflowManager.h"
#include "RestAPISystem.h"
#include "CommandExtender.h"
#include "GCodeInterpreter.h"
#include "RobotTypes.h"

void CommandInterface::queryStatus(String &respStr)
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
    RobotCommandArgs cmdArgs;
    _robotController.getCurStatus(cmdArgs);
    String healthStrRobot = cmdArgs.toJSON(false);
    if (innerJsonStr.length() > 0)
        innerJsonStr += ",";
    innerJsonStr += healthStrRobot;
    // System information
    respStr = "{" + innerJsonStr + "}";
}

bool CommandInterface::canAcceptCommand()
{
    return !_workflowManager.isFull();
}

bool CommandInterface::queueIsEmpty()
{
    return _workflowManager.isEmpty();
}

void CommandInterface::getRobotConfig(String &respStr)
{
    respStr = _robotConfig.getConfigData();
}

bool CommandInterface::setRobotConfig(const uint8_t *pData, int len)
{
    // Check sensible length
    if (len + 10 > _robotConfig.getMaxLen())
        return false;
    char tmpBuf[len + 1];
    memcpy(tmpBuf, pData, len);
    tmpBuf[len] = 0;
    // Make sure string is terminated
    _robotConfig.setConfigData(tmpBuf);
    // Reconfigure the robot
    reconfigure();
    // Apply the config data
    String patternsStr = RdJson::getString("/patterns", "{}", _robotConfig.getConfigData());
    _commandExtender.setPatterns(patternsStr.c_str());
    String sequencesStr = RdJson::getString("/sequences", "{}", _robotConfig.getConfigData());
    _commandExtender.setSequences(sequencesStr.c_str());
    // Store the configuration permanently
    _robotConfig.writeConfig();
    return true;
}

void CommandInterface::processSingle(const char *pCmdStr, String &retStr)
{
    const char *okRslt = "{\"rslt\":\"ok\"}";
    retStr = "{\"rslt\":\"none\"}";

    // Check if this is an immediate command
    if (strcasecmp(pCmdStr, "pause") == 0)
    {
        _robotController.pause(true);
        retStr = okRslt;
    }
    else if (strcasecmp(pCmdStr, "resume") == 0)
    {
        _robotController.pause(false);
        retStr = okRslt;
    }
    else if (strcasecmp(pCmdStr, "stop") == 0)
    {
        _robotController.stop();
        _workflowManager.clear();
        _commandExtender.stop();
        retStr = okRslt;
    }
    else
    {
        // Send the line to the workflow manager
        if (strlen(pCmdStr) != 0)
        {
            bool rslt = _workflowManager.add(pCmdStr);
            if (!rslt)
                retStr = "{\"rslt\":\"busy\"}";
            else
                retStr = okRslt;
        }
    }
    Log.trace("CommandInterface: procSingle rslt %s\n", retStr.c_str());
}

void CommandInterface::process(const char *pCmdStr, String &retStr, int cmdIdx)
{
    // Handle the case of a single string
    if (strstr(pCmdStr, ";") == NULL)
    {
        return processSingle(pCmdStr, retStr);
    }

    // Handle multiple commands (semicolon delimited)
    /*Log.trace("CmdInterp process %s\n", pCmdStr);*/
    const int MAX_TEMP_CMD_STR_LEN = 1000;
    const char *pCurStr = pCmdStr;
    const char *pCurStrEnd = pCmdStr;
    int curCmdIdx = 0;
    while (true)
    {
        // Find line end
        if ((*pCurStrEnd == ';') || (*pCurStrEnd == '\0'))
        {
            // Extract the line
            int stLen = pCurStrEnd - pCurStr;
            if ((stLen == 0) || (stLen > MAX_TEMP_CMD_STR_LEN))
                break;

            // Alloc
            char *pCurCmd = new char[stLen + 1];
            if (!pCurCmd)
                break;
            for (int i = 0; i < stLen; i++)
                pCurCmd[i] = *pCurStr++;
            pCurCmd[stLen] = 0;

            // process
            if (cmdIdx == -1 || cmdIdx == curCmdIdx)
            {
                /*Log.trace("cmdProc single %d %s\n", stLen, pCurCmd);*/
                processSingle(pCurCmd, retStr);
            }
            delete[] pCurCmd;

            // Move on
            curCmdIdx++;
            if (*pCurStrEnd == '\0')
                break;
            pCurStr = pCurStrEnd + 1;
        }
        pCurStrEnd++;
    }
}

void CommandInterface::service()
{
    // Pump the workflow here
    // Check if the RobotController can accept more
    if (_robotController.canAcceptCommand())
    {
        CommandElem cmdElem;
        bool rslt = _workflowManager.get(cmdElem);
        if (rslt)
        {
            Log.trace("CommandInterface: getWorkflow rlst=%d (waiting %d), %s\n", rslt,
                      _workflowManager.numWaiting(),
                      cmdElem.getString().c_str());

            // Check for extended commands
            rslt = _commandExtender.procCommand(cmdElem.getString().c_str());

            // Check for GCode
            if (!rslt)
                GCodeInterpreter::interpretGcode(cmdElem, &_robotController, true);
        }
    }

    // Service command extender (which pumps the state machines associated with extended commands)
    _commandExtender.service();
}

void CommandInterface::reconfigure()
{
    // Get the config data
    String configData = _robotConfig.getConfigData();

    // See if robotConfig is present
    String robotConfigStr = RdJson::getString("/robotConfig", "", configData.c_str());
    if (robotConfigStr.length() <= 0)
    {
        Log.notice("CommandInterface: No robotConfig found - defaulting\n");
        // See if there is a robotType specified in the main config
        String robotType = RdJson::getString("/robotType", "", configData.c_str());
        if (robotType.length() <= 0)
            RobotTypes::getNthRobotTypeName(0, robotType);
        // Set the default robot type
        robotConfigStr = RobotTypes::getConfig(robotType.c_str());
    }

    // Init robot controller and workflow manager
    _robotController.init(robotConfigStr.c_str());
    _workflowManager.init(robotConfigStr.c_str());

    // Configure the command interpreter
    Log.notice("CommandInterface: setting config\n");
    String patternsStr = RdJson::getString("/patterns", "{}", _robotConfig.getConfigData());
    _commandExtender.setPatterns(patternsStr.c_str());
    Log.notice("CommandInterface: patterns %s\n", patternsStr.c_str());
    String sequencesStr = RdJson::getString("/sequences", "{}", _robotConfig.getConfigData());
    _commandExtender.setSequences(sequencesStr.c_str());
    Log.notice("CommandInterface: sequences %s\n", sequencesStr.c_str());
}

void CommandInterface::handleStartupCommands()
{
    // Check for cmdsAtStart in the robot config
    String cmdsAtStart = RdJson::getString("/robotConfig/cmdsAtStart", "", _robotConfig.getConfigData());
    Log.notice("CommandInterface: cmdsAtStart <%s>\n", cmdsAtStart.c_str());
    if (cmdsAtStart.length() > 0)
    {
        String retStr;
        process(cmdsAtStart.c_str(), retStr);
    }

    // Check for startup commands in the EEPROM config
    String runAtStart = RdJson::getString("startup", "", _robotConfig.getConfigData());
    RdJson::unescapeString(runAtStart);
    Log.notice("CommandInterface: EEPROM commands <%s>\n", runAtStart.c_str());
    if (runAtStart.length() > 0)
    {
        String retStr;
        process(runAtStart.c_str(), retStr);
    }
}