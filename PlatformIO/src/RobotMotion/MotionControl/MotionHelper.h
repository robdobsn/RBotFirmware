// RBotFirmware
// Rob Dobson 2016-18

#pragma once

#include "../AxesParams.h"
#include "../AxisPosition.h"
#include "RobotCommandArgs.h"
#include "MotionPlanner.h"
#include "RampGenerator/RampGenerator.h"
#include "MotionHoming.h"
#include "Trinamics/TrinamicsController.h"
#include "MotorEnabler.h"

class MotionHelper
{
public:
    static constexpr float blockDistanceMM_default = 0.0f;
    static constexpr float junctionDeviation_default = 0.05f;
    static constexpr float distToTravelMM_ignoreBelow = 0.01f;
    static constexpr int pipelineLen_default = 100;
    static constexpr uint32_t MAX_TIME_BEFORE_STOP_COMPLETE_MS = 500;

private:
    // Pause
    bool _isPaused;
    // Block distance
    float _blockDistanceMM;
    // Allow all out of bounds movement
    bool _allowAllOutOfBounds;
    // Axes parameters
    AxesParams _axesParams;
    // Robot attributes
    String _robotAttributes;
    // Callbacks for coordinate conversion etc
    ptToActuatorFnType _ptToActuatorFn;
    actuatorToPtFnType _actuatorToPtFn;
    correctStepOverflowFnType _correctStepOverflowFn;
    convertCoordsFnType _convertCoordsFn;
    setRobotAttributesFnType _setRobotAttributes;
    // Relative motion
    bool _moveRelative;
    // Planner used to plan the pipeline of motion
    MotionPlanner _motionPlanner;
    // Last position the robot was commanded to go to
    AxisPosition _lastCommandedAxisPos;
    // Motion pipeline
    MotionPipeline _motionPipeline;
    // Trinamic Controller
    TrinamicsController _trinamicsController;
    // Actuators (motors etc)
    RampGenerator _rampGenerator;
    // Homing
    MotionHoming _motionHoming;
    // Motor enabler
    MotorEnabler _motorEnabler;

    // Split-up movement blocks to be added to pipeline
    // Number of blocks to add
    int _blocksToAddTotal;
    // Current block to be added
    int _blocksToAddCurBlock;
    // Start position for block generation
    AxisFloats _blocksToAddStartPos;
    // End position for block generation
    AxisFloats _blocksToAddEndPos;
    // Deltas for each axis for block generation
    AxisFloats _blocksToAddDelta;
    // Command args for block generation
    RobotCommandArgs _blocksToAddCommandArgs;

    // Handling of stop
    bool _stopRequested;
    bool _stopRequestTimeMs;

    // Debug
    unsigned long _debugLastPosDispMs;

public:
    MotionHelper();
    ~MotionHelper();

    void setTransforms(ptToActuatorFnType ptToActuatorFn, actuatorToPtFnType actuatorToPtFn,
                       correctStepOverflowFnType correctStepOverflowFn,
                       convertCoordsFnType convertCoordsFn, setRobotAttributesFnType setRobotAttributes);

    void configure(const char *robotConfigJSON);

    // Can accept
    bool canAccept();
    // Pause (or un-pause) all motion
    void pause(bool pauseIt);
    // Check if paused
    bool isPaused();
    // Stop
    void stop();
    // Check if idle
    bool isIdle();

    double getStepsPerUnit(int axisIdx)
    {
        return _axesParams.getStepsPerUnit(axisIdx);
    }

    double getStepsPerRot(int axisIdx)
    {
        return _axesParams.getStepsPerRot(axisIdx);
    }

    double getunitsPerRot(int axisIdx)
    {
        return _axesParams.getunitsPerRot(axisIdx);
    }

    long gethomeOffSteps(int axisIdx)
    {
        return _axesParams.gethomeOffSteps(axisIdx);
    }

    AxesParams &getAxesParams()
    {
        return _axesParams;
    }

    void setCurPositionAsHome(int axisIdx);

    bool moveTo(RobotCommandArgs &args);
    void setMotionParams(RobotCommandArgs &args);
    void getCurStatus(RobotCommandArgs &args);
    void getRobotAttributes(String& robotAttrs);
    void goHome(RobotCommandArgs &args);
    int getLastCompletedNumberedCmdIdx()
    {
        return max(_rampGenerator.getLastCompletedNumberedCmdIdx(), _trinamicsController.getLastCompletedNumberedCmdIdx());
    }
    void service();

    unsigned long getLastActiveUnixTime()
    {
        return _motorEnabler.getLastActiveUnixTime();
    }

    // Test code
    void debugShowBlocks();
    void debugShowTopBlock();
    void debugShowTiming();
    String getDebugStr();
    int testGetPipelineCount();
    bool testGetPipelineBlock(int elIdx, MotionBlock &elem);
    void setIntrumentationMode(const char *testModeStr)
    {
        _rampGenerator.setInstrumentationMode(testModeStr);
    }

#ifdef UNIT_TEST
    MotionHoming* testGetMotionHoming()
    {
        return &_motionHoming;
    }
#endif

private:
    bool isInBounds(double v, double b1, double b2)
    {
        return (v > fmin(b1, b2) && v < fmax(b1, b2));
    }
    void setCurPosActualPosition();
    bool addToPlanner(RobotCommandArgs &args);
    void blocksToAddProcess();
};
