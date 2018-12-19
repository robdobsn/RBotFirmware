// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

#include <Arduino.h>
#include "LedStrip.h"
#include "WorkItemQueue.h"
#include "Evaluators/EvaluatorPatterns.h"
#include "Evaluators/EvaluatorSequences.h"
#include "Evaluators/EvaluatorFiles.h"
#include "Evaluators/EvaluatorThetaRhoLine.h"

class ConfigBase;
class RobotController;
class RestAPISystem;
class FileManager;

// Work Manager - handles all workflow for the robot
class WorkManager
{
private:
    ConfigBase& _systemConfig;
    ConfigBase& _robotConfig;
    RobotController& _robotController;
    LedStrip& _ledStrip;
    WorkItemQueue _workItemQueue;
    RestAPISystem& _restAPISystem;
    FileManager& _fileManager;

    // Evaluators
    EvaluatorPatterns _evaluatorPatterns;
    EvaluatorSequences _evaluatorSequences;
    EvaluatorFiles _evaluatorFiles;
    EvaluatorThetaRhoLine _evaluatorThetaRhoLine;

public:
    WorkManager(ConfigBase& mainConfig,
                ConfigBase &robotConfig, 
                RobotController &robotController,
                LedStrip &ledStrip,
                RestAPISystem &restAPISystem,
                FileManager& fileManager);

    // Check if queue can accept a work item
    bool canAcceptWorkItem();

    // Queue info
    bool queueIsEmpty();

    // Call frequently to pump the queue
    void service();

    // Configuration of the robot
    void getRobotConfig(String& respStr);
    bool setRobotConfig(const uint8_t* pData, int len);
    bool setLedStripConfig(const uint8_t* pData, int len);

    // Apply configuration
    void reconfigure();

    // Process startup actions
    void handleStartupCommands();

    // Get status report
    void queryStatus(String &respStr);

    // Add a work item to the queue
    void addWorkItem(WorkItem& workItem, String &retStr, int cmdIdx = -1);

    // Get debug string
    String getDebugStr();

private:
    // Execute an item of work
    bool execWorkItem(WorkItem& workItem);

    // Process a single 
    void processSingle(const char *pCmdStr, String &retStr);

    // Stop Evaluators
    void evaluatorsStop();

    // Service evaluators
    void evaluatorsService();

    // Check evaluators busy
    bool evaluatorsBusy(bool includeFileEvaluator);

    // Set config
    void evaluatorsSetConfig(const char* configJson);
};
