// RBotFirmware
// Rob Dobson 2016-18

#pragma once

//#define DEBUG_TEST_DUMP 1
// #define DEBUG_MOTIONPLANNER_INFO 1
#ifdef DEBUG_MOTIONPLANNER_INFO
// #define DEBUG_MOTIONPLANNER_DETAILED_INFO 1
#endif

#include "../AxisPosition.h"
#include "../../RobotCommandArgs.h"
#include "MotionPipeline.h"

typedef bool (*ptToActuatorFnType)(AxisFloats &targetPt, AxisFloats &outActuator, AxisPosition &curPos, AxesParams &axesParams, bool allowOutOfBounds);
typedef void (*actuatorToPtFnType)(AxisInt32s &targetActuator, AxisFloats &outPt, AxisPosition &curPos, AxesParams &axesParams);
typedef void (*correctStepOverflowFnType)(AxisPosition &curPos, AxesParams &axesParams);
typedef void (*convertCoordsFnType)(RobotCommandArgs& cmdArgs, AxesParams &axesParams);
typedef void (*setRobotAttributesFnType)(AxesParams& axesParams, String& robotAttributes);

class MotionPlanner
{
  private:
    // Minimum planner speed mm/s
    float _minimumPlannerSpeedMMps;
    // Junction deviation
    float _junctionDeviation;

    // Structure to store details on last processed block
    struct MotionBlockSequentialData
    {
        AxisFloats _unitVectors;
        float _maxParamSpeedMMps;
    };
    // Data on previously processed block
    bool _prevMotionBlockValid;
    MotionBlockSequentialData _prevMotionBlock;

  public:
    MotionPlanner()
    {
        _prevMotionBlockValid = false;
        _minimumPlannerSpeedMMps = 0;
        // Configure the motion pipeline - these values will be changed in config
        _junctionDeviation = 0;
    }

    void configure(float junctionDeviation);

    // Entry point for adding a motion block
    bool moveTo(RobotCommandArgs &args,
                AxisFloats &destActuatorCoords,
                AxisPosition &curAxisPositions,
                AxesParams &axesParams, MotionPipeline &motionPipeline);

    void debugDumpQueue(const char *comStr, MotionPipeline &motionPipeline, unsigned int minQLen);

    void recalculatePipeline(MotionPipeline &motionPipeline, AxesParams &axesParams);

    // Entry point for adding a motion block
    bool moveToStepwise(RobotCommandArgs &args,
                        AxisPosition &curAxisPositions,
                        AxesParams &axesParams, MotionPipeline &motionPipeline);
};
