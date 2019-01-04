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
    _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow, convertCoords, setRobotAttributes);
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

        Log.verbose("ptToActuator %F -> %F (homeOffVal %F, homeOffSteps %d)\n",
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
        Log.verbose("actuatorToPt %d %F -> %F (perunit %F)\n", axisIdx, targetActuator.getVal(axisIdx),
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

// Set robot attributes
void RobotXYBot::setRobotAttributes(AxesParams& axesParams, String& robotAttributes)
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
