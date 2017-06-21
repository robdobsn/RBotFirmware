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
    const char* getSequences();
    void setPatterns(const char* configStr);
    const char* getPatterns();
    bool canAcceptCommand();
    bool queueIsEmpty();
    bool processSingle(const char* pCmdStr);
    bool process(const char* pCmdStr, int cmdIdx = -1);
    void service();
};
