// RBotFirmware
// Rob Dobson 2016-18

#pragma once

#include "RobotCommandArgs.h"
#include "../AxesParams.h"

class MotionHelper;

#define DBG_HOMING_LVL notice

class MotionHoming
{
private:
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
    int _feedrateStepsPerSecForHoming;

    // Homing diagnostics
    AxisInt32s _homingStartSteps;

    // Centring
    static const int NUM_CENTRING_PHASES = 6;
    bool _doCentring;
    bool _centringInProgress;
    int _centringPhase;
    AxisInt32s _centringSteps[NUM_CENTRING_PHASES];
    RobotCommandArgs _centringReversedCommand;

public:
    MotionHoming(MotionHelper *pMotionHelper);
    void configure(const char *configJSON);
    bool isHomingInProgress();
    bool isHomedOk();
    void homingStart(RobotCommandArgs &args);
    void service(AxesParams &axesParams);
    bool extractAndExecNextCmd(AxesParams &axesParams, String& debugCmdStr);

private:
    void moveTo(RobotCommandArgs &args);
    int getLastCompletedNumberedCmdIdx();
    void setAtHomePos(int axisIdx);
    bool getInteger(unsigned int &homingStrPos, int &retInt);
    int getFeedrate(unsigned int &homingStrPos, AxesParams &axesParams, int axisIdx, int defaultValue);
    void setEndstops(unsigned int &homingStrPos, int axisIdx);
    void setCentring(unsigned int &homingStrPos, int axisIdx);
    void startCentringOperation();
    bool nextCentringOperation();
    void processHomingCommand(RobotCommandArgs& commandArgs);
    void debugShowSteps(const char* debugMsg);

};
