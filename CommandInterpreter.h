// RBotFirmware
// Rob Dobson 2016

#pragma once

class WorkflowManager;
class CommandExtender;

// Command interpreter
class CommandInterpreter
{
private:
    WorkflowManager* _pWorkflowManager;
    RobotController* _pRobotController;
    CommandExtender* _pCommandExtender;

public:
    CommandInterpreter(WorkflowManager* pWorkflowManager, RobotController* pRobotController);
    void setSequences(const char* configStr);
    void setPatterns(const char* configStr);
    bool canAcceptCommand();
    bool queueIsEmpty();
    bool processSingle(const char* pCmdStr);
    bool process(const char* pCmdStr, int cmdIdx = -1);
    void service();
};
