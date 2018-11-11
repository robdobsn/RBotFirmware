// RBotFirmware
// Rob Dobson 2016-18

#pragma once

class MotionHelper;

#define DBG_HOMING_LVL notice

class MotionHoming
{
public:
    static constexpr int maxHomingSecs_default = 1000;
    static constexpr int homing_baseCommandIndex = 10000;

    bool _isHomedOk;
    String _homingSequence;
    bool _homingInProgress;
    RobotCommandArgs _axesToHome;
    unsigned int _homingStrPos;
    bool _commandInProgress;
    RobotCommandArgs _curCommand;
    int _maxHomingSecs;
    unsigned long _homeReqMillis;
    MotionHelper *_pMotionHelper;
    int _homingCurCommandIndex;

    MotionHoming(MotionHelper *pMotionHelper)
    {
        _pMotionHelper = pMotionHelper;
        _homingInProgress = false;
        _homingStrPos = 0;
        _commandInProgress = false;
        _isHomedOk = false;
        _maxHomingSecs = maxHomingSecs_default;
        _homeReqMillis = 0;
        _homingCurCommandIndex = homing_baseCommandIndex;
    }

    void configure(const char *configJSON);
    bool isHomingInProgress();
    void homingStart(RobotCommandArgs &args);
    void service(AxesParams &axesParams);
    bool extractAndExecNextCmd(AxesParams &axesParams);

private:
    void moveTo(RobotCommandArgs &args);
    int getLastCompletedNumberedCmdIdx();
    void setAtHomePos(int axisIdx);
};
