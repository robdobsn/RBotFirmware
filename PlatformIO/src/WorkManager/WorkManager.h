// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

// #define DEBUG_WORK_ITEM_SERVICE 1

#include <Arduino.h>
#include "LedStrip.h"
#include "WorkItemQueue.h"
#include "Evaluators/EvaluatorPatterns.h"
#include "Evaluators/EvaluatorSequences.h"
#include "Evaluators/EvaluatorFiles.h"
#include "Evaluators/EvaluatorThetaRhoLine.h"
#include "RobotCommandArgs.h"

class ConfigBase;
class RobotController;
class RestAPISystem;
class FileManager;
class CommandScheduler;

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
    CommandScheduler& _commandScheduler;

    // Evaluators
    EvaluatorPatterns _evaluatorPatterns;
    EvaluatorSequences _evaluatorSequences;
    EvaluatorFiles _evaluatorFiles;
    EvaluatorThetaRhoLine _evaluatorThetaRhoLine;

    // Status updates
    RobotCommandArgs _statusLastCmdArgs;
    unsigned long _statusLastHashVal;
    unsigned long _statusReportLastCheck;
    unsigned long _statusAlwaysLastCheck;
    // Time between status change checks
    const unsigned long STATUS_CHECK_MS = 250;
    // A status update will always be sent (even if no change) after this time
    const unsigned long STATUS_ALWAYS_UPDATE_MS = 10000;

    // Debug
#ifdef DEBUG_WORK_ITEM_SERVICE
    uint32_t _debugLastWorkServiceMs;
    static const uint32_t DEBUG_BETWEEN_WORK_ITEM_SERVICES_MS = 100;
#endif

public:
    WorkManager(ConfigBase& mainConfig,
                ConfigBase &robotConfig, 
                RobotController &robotController,
                LedStrip &ledStrip,
                RestAPISystem &restAPISystem,
                FileManager& fileManager,
                CommandScheduler& commandScheduler);

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

    // Check status changed
    bool checkStatusChanged();

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
    void evaluatorsSetConfig(const char* configJson, const char* jsonPath, const char* robotAttributes);

    // Can be processed
    bool canBeProcessed(WorkItem& workItem);
};
