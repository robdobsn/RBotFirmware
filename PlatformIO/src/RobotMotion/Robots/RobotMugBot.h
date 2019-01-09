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

    RobotMugBot(const char* pRobotTypeName, MotionHelper& motionHelper) :
        RobotBase(pRobotTypeName, motionHelper)
    {
        _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow, convertCoords, setRobotAttributes);
    }

    // Convert a cartesian point to actuator coordinates
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

            // Log.notice("ptToActuator %F -> %F (homeOffVal %F, homeOffSteps %d)\n",
            //         pt.getVal(axisIdx), actuatorCoords._pt[axisIdx],
            //         axesParams.getHomeOffsetVal(axisIdx), axesParams.gethomeOffSteps(axisIdx));
        }
        return ptWasValid;
    }

    // Convert actuator values to cartesian point
    static void actuatorToPt(AxisInt32s& targetActuator, AxisFloats& outPt, AxisPosition& curPos, AxesParams& axesParams)
    {
        // Perform conversion
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            float ptVal = targetActuator.getVal(axisIdx) - axesParams.gethomeOffSteps(axisIdx);
            ptVal = ptVal / axesParams.getStepsPerUnit(axisIdx) + axesParams.getHomeOffsetVal(axisIdx);
            outPt.setVal(axisIdx, ptVal);
            // Log.notice("actuatorToPt %d %F -> %F (perunit %F)\n", axisIdx, actuatorCoords.getVal(axisIdx),
            //                 ptVal, axesParams.getStepsPerUnit(axisIdx));
        }
    }

    // Correct overflow (necessary for continuous rotation robots)
    static void correctStepOverflow(AxisPosition& curPos, AxesParams& axesParams)
    {
    }

    // Convert coordinates in place
    static void convertCoords(RobotCommandArgs& cmdArgs, AxesParams& axesParams)
    {
    }
    
    // Set robot attributes
    static void setRobotAttributes(AxesParams& axesParams, String& robotAttributes)
    {
        // Calculate max and min cartesian size of robot
        float xMin = 0, xMax = 0, yMin = 0, yMax = 0;
        axesParams.getMaxVal(0, xMin);
        bool axis0MaxValid = axesParams.getMaxVal(0, xMax);
        axesParams.getMaxVal(1, yMin);
        bool axis1MaxValid = axesParams.getMaxVal(1, yMax);
        // If not valid set to some values to avoid arithmetic errors
        if (!axis0MaxValid)
            xMax = 100;
        if (!axis1MaxValid)
            yMax = 100;

        // Set attributes
        constexpr int MAX_ATTR_STR_LEN = 400;
        char attrStr[MAX_ATTR_STR_LEN];
        sprintf(attrStr, "{\"sizeX\":%0.2f,\"sizeY\":%0.2f,\"sizeZ\":%0.2f,\"originX\":%0.2f,\"originY\":%0.2f,\"originZ\":%0.2f}",
                fabsf(xMax-xMin), fabsf(yMax-yMin), 0.0,
                0.0, 0.0, 0.0);
        robotAttributes = attrStr;
    }
};
