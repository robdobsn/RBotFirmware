// RBotFirmware
// Rob Dobson 2016

#pragma once

#include <Arduino.h>

class ConfigNVS;
class RobotController;
class WorkflowManager;
class RestAPISystem;
class CommandExtender;

// Command interface
class CommandInterface
{
  private:
    ConfigNVS& _robotConfig;
    RobotController& _robotController;
    WorkflowManager& _workflowManager;
    RestAPISystem& _restAPISystem;
    CommandExtender& _commandExtender;

  public:
    CommandInterface(ConfigNVS &robotConfig, 
                RobotController &robotController,
                WorkflowManager &workflowManager,
                RestAPISystem &restAPISystem,
                CommandExtender& commandExtender) :
                _robotConfig(robotConfig),
                _robotController(robotController),
                _workflowManager(workflowManager),
                _restAPISystem(restAPISystem),
                _commandExtender(commandExtender)
    {
    }

    // void setSequences(const char *configStr);
    // const char *getSequences();
    // void setPatterns(const char *configStr);
    // const char *getPatterns();
    bool canAcceptCommand();
    bool queueIsEmpty();
    void service();

    void getRobotConfig(String& respStr);
    bool setRobotConfig(const uint8_t* pData, int len);

    void reconfigure();
    void handleStartupCommands();

    void queryStatus(String &respStr);
    void processSingle(const char *pCmdStr, String &retStr);
    void process(const char *pCmdStr, String &retStr, int cmdIdx = -1);
};
