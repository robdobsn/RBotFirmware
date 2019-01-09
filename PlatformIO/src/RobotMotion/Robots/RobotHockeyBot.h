// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "Utils.h"
#include "RobotBase.h"
#include "math.h"

class RobotHockeyBot : public RobotBase
{
public:

    RobotHockeyBot(const char* pRobotTypeName, MotionHelper& motionHelper) :
        RobotBase(pRobotTypeName, motionHelper)
    {
        _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow, convertCoords, setRobotAttributes);
    }

    static bool ptToActuator(AxisFloats& targetPt, AxisFloats& outActuator, AxisPosition& curPos, AxesParams& axesParams, bool allowOutOfBounds)
    {
        // // Simple scaling from one domain to another
        // bool isValid = true;
        // double aStepper = (motionElem._pt2MM.getVal(0) + motionElem._pt2MM.getVal(1)) * axisParams[0].stepsPerUnit();
        // actuatorCoords.setVal(0, aStepper);
        // double bStepper = (motionElem._pt2MM.getVal(0) - motionElem._pt2MM.getVal(1)) * axisParams[1].stepsPerUnit();
        // actuatorCoords.setVal(1, bStepper);
        //
        //
        //     // Check machine bounds
        //     // bool thisAxisValid = true;
        //     // if (axisParams[i]._minValValid && motionElem._pt2MM.getVal(i) < axisParams[i]._minVal)
        //     //     thisAxisValid = false;
        //     // if (axisParams[i]._maxValValid && motionElem._pt2MM.getVal(i) > axisParams[i]._maxVal)
        //     //     thisAxisValid = false;
        // bool axis0Valid = true;
        // bool axis1Valid = true;
        // Log.trace("ptToActuator X (%s) %F -> %F Y (%s) %F -> %F\n",
        //     axis0Valid ? "OK" : "INVALID",
        //     motionElem._pt2MM.getVal(0), actuatorCoords._pt[0],
        //     axis1Valid ? "OK" : "INVALID",
        //     motionElem._pt2MM.getVal(1), actuatorCoords._pt[1]);
        //     // isValid &&= thisAxisValid;
        // // }
        return true;
    }

    static void actuatorToPt(AxisInt32s& targetActuator, AxisFloats& outPt, AxisPosition& curPos, AxesParams& axesParams)
    {
        // double x = (actuatorCoords.getVal(0) + actuatorCoords.getVal(1)) / 2 / axisParams[0].stepsPerUnit();
        // double y = (actuatorCoords.getVal(0) - actuatorCoords.getVal(1)) / 2 / axisParams[1].stepsPerUnit();
        //
        //     // if (axisParams[i]._minValValid && ptVal < axisParams[i]._minVal)
        //     //     ptVal = axisParams[i]._minVal;
        //     // if (axisParams[i]._maxValValid && ptVal > axisParams[i]._maxVal)
        //     //     ptVal = axisParams[i]._maxVal;
        // pt.setVal(0, x);
        // pt.setVal(1, y);
        //
        // Log.trace("actuatorToPt 0 %F -> %F (perunit %F) 1 %F -> %F (perunit %F)\n",
        //         actuatorCoords.getVal(0), x, axisParams[0].stepsPerUnit(),
        //         actuatorCoords.getVal(1), y, axisParams[1].stepsPerUnit());
    }

    static void correctStepOverflow(AxisPosition& curPos, AxesParams& axesParams)
    {
    }

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

    // Set config
    // bool init(const char* robotConfigStr)
    // {
    //     // Info
    //     // Log.notice("Constructing %s from %s\n", _robotTypeName.c_str(), robotConfigStr);
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
