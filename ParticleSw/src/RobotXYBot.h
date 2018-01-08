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

    static bool ptToActuator(AxisFloats& pt, AxisFloats& actuatorCoords, AxesParams& axesParams)
    {
        // Check machine bounds and fix the value if required
        bool ptWasValid = axesParams.ptInBounds(pt, true);

        // Perform conversion
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            // Axis val from home point
            float axisValFromHome = pt.getVal(axisIdx) - axesParams.getHomeOffsetVal(axisIdx);
            // Convert to steps and add offset to home in steps
            actuatorCoords.setVal(axisIdx, axisValFromHome * axesParams.getStepsPerUnit(axisIdx)
                            + axesParams.getHomeOffsetSteps(axisIdx));

            Log.trace("ptToActuator %f -> %f (homeOffVal %f, homeOffSteps %ld)",
                    pt.getVal(axisIdx), actuatorCoords._pt[axisIdx],
                    axesParams.getHomeOffsetVal(axisIdx), axesParams.getHomeOffsetSteps(axisIdx));
        }
        return ptWasValid;
    }

    static void actuatorToPt(AxisFloats& actuatorCoords, AxisFloats& pt, AxesParams& axesParams)
    {
        // Perform conversion
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            float ptVal = actuatorCoords.getVal(axisIdx) - axesParams.getHomeOffsetSteps(axisIdx);
            ptVal = ptVal / axesParams.getStepsPerUnit(axisIdx) + axesParams.getHomeOffsetVal(axisIdx);
            pt.setVal(axisIdx, ptVal);
            Log.trace("actuatorToPt %d %f -> %f (perunit %f)", axisIdx, actuatorCoords.getVal(axisIdx),
                            ptVal, axesParams.getStepsPerUnit(axisIdx));
        }
    }

    static void correctStepOverflow(AxesParams& axesParams)
    {
        // Not necessary for a non-continuous rotation bot
    }

private:
    // MotionHelper for the robot motion
    MotionHelper& _motionHelper;

public:
    RobotXYBot(const char* pRobotTypeName, MotionHelper& motionHelper) :
        RobotBase(pRobotTypeName), _motionHelper(motionHelper)
    {
        _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
    }

    // Set config
    bool init(const char* robotConfigStr)
    {
        // Info
        // Log.info("Constructing %s from %s", _robotTypeName.c_str(), robotConfigStr);

        // Init motion controller from config
        _motionHelper.configure(robotConfigStr);

        return true;
    }

    bool canAcceptCommand()
    {
        // Check if motionHelper is can accept a command
        return _motionHelper.canAccept();
    }

    void moveTo(RobotCommandArgs& args)
    {
        _motionHelper.moveTo(args);
    }

    void setMotionParams(RobotCommandArgs& args)
    {
        _motionHelper.setMotionParams(args);
    }

    void getCurStatus(RobotCommandArgs& args)
    {
        _motionHelper.getCurStatus(args);
    }

    // Pause (or un-pause) all motion
    void pause(bool pauseIt)
    {
        _motionHelper.pause(pauseIt);
    }

    // Check if paused
    bool isPaused()
    {
        return _motionHelper.isPaused();
    }

    // Stop
    void stop()
    {
        _motionHelper.stop();
    }

    void service()
    {
        // Service the motion controller
        _motionHelper.service(true);
    }

    bool wasActiveInLastNSeconds(unsigned int nSeconds)
    {
        return ((unsigned long)Time.now() < _motionHelper.getLastActiveUnixTime() + nSeconds);
    }
};
