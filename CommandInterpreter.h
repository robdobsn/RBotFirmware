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
    void setConfig(const char* configStr);
    bool canAcceptCommand();
    bool processSingle(const char* pCmdStr);
    bool process(const char* pCmdStr);
    void service();
};
