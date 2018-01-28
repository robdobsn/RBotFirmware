// RBotFirmware
// Rob Dobson 2016-18

#pragma once

#include "application.h"
#include "AxesParams.h"
#include "AxisPosition.h"
#include "RobotCommandArgs.h"
#include "MotionPlanner.h"
#include "MotionIO.h"
#include "MotionActuator.h"

class MotionHelper
{
public:
  static constexpr float blockDistanceMM_default    = 0.0f;
  static constexpr float junctionDeviation_default  = 0.05f;
  static constexpr float distToTravelMM_ignoreBelow = 0.01f;
  static constexpr int pipelineLen_default          = 100;

private:
  // Pause
  bool _isPaused;
  // Robot dimensions
  float _xMaxMM;
  float _yMaxMM;
  // Block distance
  float _blockDistanceMM;
  // Axes parameters
  AxesParams _axesParams;
  // Callbacks for coordinate conversion etc
  ptToActuatorFnType _ptToActuatorFn;
  actuatorToPtFnType _actuatorToPtFn;
  correctStepOverflowFnType _correctStepOverflowFn;
  // Relative motion
  bool _moveRelative;
  // Planner used to plan the pipeline of motion
  MotionPlanner _motionPlanner;
  // Axis Current Motion
  AxisPosition _curAxisPosition;
  // Motion pipeline
  MotionPipeline _motionPipeline;
  // Motion IO (Motors and end-stops)
  MotionIO _motionIO;
  // Motion
  MotionActuator _motionActuator;

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

  // Debug
  unsigned long _debugLastPosDispMs;

public:
  MotionHelper();
  ~MotionHelper();

  void setTransforms(ptToActuatorFnType ptToActuatorFn, actuatorToPtFnType actuatorToPtFn,
                     correctStepOverflowFnType correctStepOverflowFn);

  void configure(const char* robotConfigJSON);

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

  void jumpHome(int axisIdx)
  {
    Log.info("JUMP HOME IS NO LONGER IMPLEMENTED");
  }

  bool isEndStopValid(int axisIdx, int endStopIdx)
  {
    Log.info("isEndStopValid IS NO LONGER IMPLEMENTED");
    return true;
  }

  bool isAtEndStop(int axisIdx, int endStopIdx)
  {
    Log.info("isAtEndStop IS NO LONGER IMPLEMENTED");
    return true;
  }

  double getStepsPerUnit(int axisIdx)
  {
    return _axesParams.getStepsPerUnit(axisIdx);
  }

  double getStepsPerRotation(int axisIdx)
  {
    return _axesParams.getStepsPerRotation(axisIdx);
  }

  double getUnitsPerRotation(int axisIdx)
  {
    return _axesParams.getUnitsPerRotation(axisIdx);
  }

  long getHomeOffsetSteps(int axisIdx)
  {
    return _axesParams.getHomeOffsetSteps(axisIdx);
  }

  AxesParams& getAxesParams()
  {
    return _axesParams;
  }

  void setCurPositionAsHome(AxisFloats& pt);
  void setCurPositionAsHome(bool xIsHome, bool yIsHome, bool zIsHome);

  bool moveTo(RobotCommandArgs& args);
  void setMotionParams(RobotCommandArgs& args);
  void getCurStatus(RobotCommandArgs& args);
  void service(bool processPipeline);

  unsigned long getLastActiveUnixTime()
  {
    return _motionIO.getLastActiveUnixTime();
  }

  // Test code
  void debugShowBlocks();
  void debugShowTiming();
  int testGetPipelineCount();
  bool testGetPipelineBlock(int elIdx, MotionBlock& elem);
  void setTestMode(const char* testModeStr)
  {
    _motionActuator.setTestMode(testModeStr);
  }

private:
  bool isInBounds(double v, double b1, double b2)
  {
    return(v > fmin(b1, b2) && v < fmax(b1, b2));
  }

  bool configureRobot(const char* robotConfigJSON);
  bool addToPlanner(RobotCommandArgs& args);
  void blocksToAddProcess();
};
