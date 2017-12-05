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

    static bool ptToActuator(MotionPipelineElem& motionElem, PointND& actuatorCoords, AxisParams axisParams[], int numAxes)
    {
        bool isValid = true;
        for (int i = 0; i < MAX_AXES; i++)
        {
            // Axis val from home point
            double axisValFromHome = motionElem._pt2MM.getVal(i) - axisParams[i]._homeOffsetVal;
            // Convert to steps and add offset to home in steps
            actuatorCoords.setVal(i, axisValFromHome * axisParams[i].stepsPerUnit()
                            + axisParams[i]._homeOffsetSteps);

            // Check machine bounds
            bool thisAxisValid = true;
            if (axisParams[i]._minValValid && motionElem._pt2MM.getVal(i) < axisParams[i]._minVal)
                thisAxisValid = false;
            if (axisParams[i]._maxValValid && motionElem._pt2MM.getVal(i) > axisParams[i]._maxVal)
                thisAxisValid = false;
            Log.trace("ptToActuator (%s) %f -> %f (homeOffVal %f, homeOffSteps %ld)", thisAxisValid ? "OK" : "INVALID",
                motionElem._pt2MM.getVal(i), actuatorCoords._pt[i], axisParams[i]._homeOffsetVal, axisParams[i]._homeOffsetSteps);
            isValid &= thisAxisValid;
        }
        return isValid;
    }

    static void actuatorToPt(PointND& actuatorCoords, PointND& pt, AxisParams axisParams[], int numAxes)
    {
        for (int i = 0; i < MAX_AXES; i++)
        {
            double ptVal = actuatorCoords.getVal(i) - axisParams[i]._homeOffsetSteps;
            ptVal = ptVal / axisParams[i].stepsPerUnit() + axisParams[i]._homeOffsetVal;
            if (axisParams[i]._minValValid && ptVal < axisParams[i]._minVal)
                ptVal = axisParams[i]._minVal;
            if (axisParams[i]._maxValValid && ptVal > axisParams[i]._maxVal)
                ptVal = axisParams[i]._maxVal;
            pt.setVal(i, ptVal);
            Log.trace("actuatorToPt %d %f -> %f (perunit %f)", i, actuatorCoords.getVal(i), ptVal, axisParams[i].stepsPerUnit());
        }
    }

    static void correctStepOverflow(AxisParams axisParams[], int numAxes)
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
        _motionHelper.setAxisParams(robotConfigStr);

        return true;
    }

    bool canAcceptCommand()
    {
        // Check if motionHelper is can accept a command
        return _motionHelper.canAcceptCommand();
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
