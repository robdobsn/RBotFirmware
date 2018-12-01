// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "Utils.h"
#include "RobotBase.h"
#include "math.h"

class RobotMugBot : public RobotBase
{
public:
    // Defaults
    static constexpr int maxHomingSecs_default = 30;
    static constexpr int _homingLinearFastStepTimeUs = 15;
    static constexpr int _homingLinearSlowStepTimeUs = 24;

public:

    static bool ptToActuator(AxisFloats& targetPt, AxisFloats& outActuator, AxisPosition& curPos, AxesParams& axesParams, bool allowOutOfBounds)
    {
        // Note that the rotation angle comes straight from the Y parameter
        // This means that drawings in the range 0 .. 240mm height (assuming 1:1 scaling is chosen)
        // will translate directly to the surface of the mug and makes the drawing
        // mug-radius independent

        // Check machine bounds and fix the value if required
        bool ptWasValid = axesParams.ptInBounds(targetPt, !allowOutOfBounds);

        // Perform conversion
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            // Axis val from home point
            float axisValFromHome = targetPt.getVal(axisIdx) - axesParams.getHomeOffsetVal(axisIdx);
            // Convert to steps and add offset to home in steps
            outActuator.setVal(axisIdx, axisValFromHome * axesParams.getStepsPerUnit(axisIdx)
                            + axesParams.gethomeOffSteps(axisIdx));

            // Log.notice("ptToActuator %f -> %f (homeOffVal %f, homeOffSteps %ld)\n",
            //         pt.getVal(axisIdx), actuatorCoords._pt[axisIdx],
            //         axesParams.getHomeOffsetVal(axisIdx), axesParams.gethomeOffSteps(axisIdx));
        }
        return ptWasValid;
    }

    static void actuatorToPt(AxisFloats& targetActuator, AxisFloats& outPt, AxisPosition& curPos, AxesParams& axesParams)
    {
        // Perform conversion
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            float ptVal = targetActuator.getVal(axisIdx) - axesParams.gethomeOffSteps(axisIdx);
            ptVal = ptVal / axesParams.getStepsPerUnit(axisIdx) + axesParams.getHomeOffsetVal(axisIdx);
            outPt.setVal(axisIdx, ptVal);
            // Log.notice("actuatorToPt %d %f -> %f (perunit %f)\n", axisIdx, actuatorCoords.getVal(axisIdx),
            //                 ptVal, axesParams.getStepsPerUnit(axisIdx));
        }
    }

    static void correctStepOverflow(AxisPosition& curPos, AxesParams& axesParams)
    {
    }

    static void convertCoords(RobotCommandArgs& cmdArgs, AxesParams& axesParams)
    {
    }

public:
    RobotMugBot(const char* pRobotTypeName, MotionHelper& motionHelper) :
        RobotBase(pRobotTypeName, motionHelper)
    {
        _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow, convertCoords);
    }
};
