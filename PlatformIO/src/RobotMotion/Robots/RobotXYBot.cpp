// RBotFirmware
// Rob Dobson 2017-18

#include <Arduino.h>
#include "RobotXYBot.h"
#include "../MotionControl/MotionHelper.h"
#include "Utils.h"
#include "math.h"

RobotXYBot::RobotXYBot(const char* pRobotTypeName, MotionHelper& motionHelper) :
    RobotBase(pRobotTypeName, motionHelper)
{
    _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow, convertCoords);
}

bool RobotXYBot::ptToActuator(AxisFloats& targetPt, AxisFloats& outActuator, 
                AxisPosition& curPos, AxesParams& axesParams, bool allowOutOfBounds)
{
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

        Log.trace("ptToActuator %F -> %F (homeOffVal %F, homeOffSteps %d)\n",
                targetPt.getVal(axisIdx), outActuator._pt[axisIdx],
                axesParams.getHomeOffsetVal(axisIdx), axesParams.gethomeOffSteps(axisIdx));
    }
    return ptWasValid;
}

void RobotXYBot::actuatorToPt(AxisFloats& targetActuator, AxisFloats& outPt, 
                AxisPosition& curPos, AxesParams& axesParams)
{
    // Perform conversion
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        float ptVal = targetActuator.getVal(axisIdx) - axesParams.gethomeOffSteps(axisIdx);
        ptVal = ptVal / axesParams.getStepsPerUnit(axisIdx) + axesParams.getHomeOffsetVal(axisIdx);
        outPt.setVal(axisIdx, ptVal);
        Log.trace("actuatorToPt %d %F -> %F (perunit %F)\n", axisIdx, targetActuator.getVal(axisIdx),
                        ptVal, axesParams.getStepsPerUnit(axisIdx));
    }
}

void RobotXYBot::correctStepOverflow(AxisPosition& curPos, AxesParams& axesParams)
{
    // Not necessary for a non-continuous rotation bot
}

void RobotXYBot::convertCoords(RobotCommandArgs& cmdArgs, AxesParams& axesParams)
{
}
