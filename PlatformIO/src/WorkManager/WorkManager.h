// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

#include <Arduino.h>
#include "WorkItemQueue.h"
#include "EvaluatorPatterns.h"
#include "EvaluatorSequences.h"

class ConfigBase;
class RobotController;
class RestAPISystem;
class FileManager;

// Work Manager - handles all workflow for the robot
class WorkManager
{
private:
    ConfigBase& _mainConfig;
    ConfigBase& _robotConfig;
    RobotController& _robotController;
    WorkItemQueue _workflowManager;
    RestAPISystem& _restAPISystem;
    FileManager& _fileManager;

    // Evaluators
    EvaluatorPatterns _patternEvaluator;
    EvaluatorSequences _commandSequencer;

public:
    WorkManager(ConfigBase& mainConfig,
                ConfigBase &robotConfig, 
                RobotController &robotController,
                RestAPISystem &restAPISystem,
                FileManager& fileManager) :
                _mainConfig(mainConfig),
                _robotConfig(robotConfig),
                _robotController(robotController),
                _restAPISystem(restAPISystem),
                _fileManager(fileManager)
    {
    }

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

private:
  bool execWorkItem(WorkItem& workItem);

};
