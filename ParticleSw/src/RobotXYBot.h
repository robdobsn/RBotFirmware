// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "application.h"
#include "Utils.h"
#include "RobotBase.h"
#include "MotionHelper.h"
#include "math.h"

class RobotXYBot : public RobotBase
{
public:

    static bool ptToActuator(AxisFloats& targetPt, AxisFloats& outActuator, AxisPosition& curPos, AxesParams& axesParams, bool allowOutOfBounds)
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

            Log.trace("ptToActuator %f -> %f (homeOffVal %f, homeOffSteps %ld)",
                    targetPt.getVal(axisIdx), outActuator._pt[axisIdx],
                    axesParams.getHomeOffsetVal(axisIdx), axesParams.gethomeOffSteps(axisIdx));
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
            Log.trace("actuatorToPt %d %f -> %f (perunit %f)", axisIdx, targetActuator.getVal(axisIdx),
                            ptVal, axesParams.getStepsPerUnit(axisIdx));
        }
    }

    static void correctStepOverflow(AxisPosition& curPos, AxesParams& axesParams)
    {
        // Not necessary for a non-continuous rotation bot
    }

public:
    RobotXYBot(const char* pRobotTypeName, MotionHelper& motionHelper) :
        RobotBase(pRobotTypeName, motionHelper)
    {
        _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
    }

    // // Set config
    // bool init(const char* robotConfigStr)
    // {
    //     // Info
    //     // Log.info("Constructing %s from %s", _robotTypeName.c_str(), robotConfigStr);
    //
    //     // Init motion controller from config
    //     _motionHelper.configure(robotConfigStr);
    //
    //     return true;
    // }
    //
    // bool canAcceptCommand()
    // {
    //     // Check if motionHelper is can accept a command
    //     return _motionHelper.canAccept();
    // }
    //
    // void moveTo(RobotCommandArgs& args)
    // {
    //     _motionHelper.moveTo(args);
    // }
    //
    // void setMotionParams(RobotCommandArgs& args)
    // {
    //     _motionHelper.setMotionParams(args);
    // }
    //
    // void getCurStatus(RobotCommandArgs& args)
    // {
    //     _motionHelper.getCurStatus(args);
    // }
    //
    // // Pause (or un-pause) all motion
    // void pause(bool pauseIt)
    // {
    //     _motionHelper.pause(pauseIt);
    // }
    //
    // // Check if paused
    // bool isPaused()
    // {
    //     return _motionHelper.isPaused();
    // }
    //
    // // Stop
    // void stop()
    // {
    //     _motionHelper.stop();
    // }
    //
    // void service()
    // {
    //     // Service the motion controller
    //     _motionHelper.service(true);
    // }
    //
    // bool wasActiveInLastNSeconds(unsigned int nSeconds)
    // {
    //     return ((unsigned long)Time.now() < _motionHelper.getLastActiveUnixTime() + nSeconds);
    // }
};
