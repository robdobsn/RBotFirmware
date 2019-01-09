// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "Utils.h"
#include "RobotBase.h"
#include "math.h"

class RobotGeistBot : public RobotBase
{
private:
    static const int NUM_ROBOT_AXES = 2;
    // Defaults
    static constexpr int _homingRotateFastStepTimeUs = 1000;
    static constexpr int _homingRotateSlowStepTimeUs = 3000;
    static constexpr int _homingLinearFastStepTimeUs = 100;
    static constexpr int _homingLinearSlowStepTimeUs = 500;
    static constexpr int maxHomingSecs_default = 1000;
    static constexpr double homingLinOffsetDegs_default = 30;
    static constexpr double homingRotCentreDegs_default = 2;
    static constexpr int homingLinMaxSteps_default = 100;
    static constexpr double homingCentreOffsetMM_default = 20;

public:

    RobotGeistBot(const char* pRobotTypeName, MotionHelper& motionHelper) :
        RobotBase(pRobotTypeName, motionHelper)
    {
        _homingState = HOMING_STATE_IDLE;
        _homeReqMillis = 0;
        _homingStepsDone = 0;
        _homingStepsLimit = 0;
        _maxHomingSecs = maxHomingSecs_default;
        _timeBetweenHomingStepsUs = _homingRotateSlowStepTimeUs;
        _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow, convertCoords, setRobotAttributes);
    }

    static bool ptToActuator(AxisFloats& targetPt, AxisFloats& outActuator, AxisPosition& curPos, AxesParams& axesParams, bool allowOutOfBounds)
    {
        // // Trig for required position (azimuth is measured clockwise from North)
        // PointND& pt = motionElem._pt2MM;
        // double reqAlphaRads = atan2(pt._pt[0], pt._pt[1]);
        // if (reqAlphaRads < 0)
        //     reqAlphaRads = 2 * M_PI + reqAlphaRads;
        // double reqLinearMM = sqrt(pt._pt[0] * pt._pt[0] + pt._pt[1] * pt._pt[1]);
        //
        // // Log.trace("xyToActuator x %F y %F ax0StNow %d ax1StNow %d rqAlphaD %F rqLinMM %F\n",
        // //         xy[0], xy[1], axisParams[0]._stepsFromHome, axisParams[1]._stepsFromHome,
        // //         reqAlphaRads * 180 / M_PI, reqLinearMM);
        //
        // // Get current position
        // PointND axisPosns;
        // for (int i = 0; i < MAX_AXES; i++)
        //     axisPosns._pt[i] = axisParams[i]._stepsFromHome;
        // double currentPolar[NUM_ROBOT_AXES];
        // actuatorToPolar(axisPosns, currentPolar, axisParams, 2);
        //
        // // Calculate shortest azimuth distance
        // double alphaDiffRads = reqAlphaRads - currentPolar[0];
        // bool alphaRotateCw = alphaDiffRads > 0;
        // if (fabs(alphaDiffRads) > M_PI)
        // {
        //     alphaDiffRads = 2 * M_PI - fabs(alphaDiffRads);
        //     alphaRotateCw = !alphaRotateCw;
        // }
        // double alphaDiffDegs = fabs(alphaDiffRads) * 180 / M_PI;
        //
        // // Linear distance
        // double linearDiffMM = reqLinearMM - currentPolar[1];
        //
        // // Convert to steps - note that to keep the linear position constant the linear stepper needs to step one step in the same
        // // direction as the arm rotation stepper - so the linear stepper steps required is the sum of the rotation and linear steps
        // double actuator0Diff = alphaDiffDegs * axisParams[0].stepsPerUnit() * (alphaRotateCw ? 1 : -1);
        // double actuator1Diff = linearDiffMM * axisParams[1].stepsPerUnit() + actuator0Diff;
        // actuatorCoords._pt[0] = axisParams[0]._stepsFromHome + actuator0Diff;
        // actuatorCoords._pt[1] = axisParams[1]._stepsFromHome + actuator1Diff;
        //
        // // Log.trace("xyToActuator reqAlphaD %F curAlphaD %F alphaDiffD %F aRotCw %d linDiffMM %F ax0Diff %F ax1Diff %F ax0Tgt %F ax1Tgt %F\n",
        // //                 reqAlphaRads * 180 / M_PI, currentPolar[0] * 180 / M_PI, alphaDiffDegs, alphaRotateCw,
        // //                 linearDiffMM, actuator0Diff, actuator1Diff, actuatorCoords[0], actuatorCoords[1]);
        // //
        // // Log.trace("bounds check X (En%d) %d, Y (En%d) %d\n", axisParams[1]._minValValid, reqLinearMM < axisParams[1]._minVal,
        // //             axisParams[1]._maxValValid, reqLinearMM > axisParams[1]._maxVal);
        //
        // // Cross check
        // double checkPolarCoords[NUM_ROBOT_AXES];
        // actuatorToPolar(actuatorCoords, checkPolarCoords, axisParams, 2);
        // double checkX = checkPolarCoords[1] * sin(checkPolarCoords[0]);
        // double checkY = checkPolarCoords[1] * cos(checkPolarCoords[0]);
        // double checkErr = sqrt((checkX-pt._pt[0]) * (checkX-pt._pt[0]) + (checkY-pt._pt[1]) * (checkY-pt._pt[1]));
        // if (checkErr > 0.1)
        //     Log.trace("check reqX %F checkX %F, reqY %F checkY %F, error %F, %s\n", pt._pt[0], checkX, pt._pt[1], checkY,
        //                     checkErr, (checkErr>0.1) ? "****** FAILED ERROR CHECK" : "");
        //
        // // Check machine bounds for linear axis
        // if (axisParams[1]._minValValid && reqLinearMM < axisParams[1]._minVal)
        //     return false;
        // if (axisParams[1]._maxValValid && reqLinearMM > axisParams[1]._maxVal)
        //     return false;
        return true;
    }

    static void actuatorToPolar(AxisInt32s& actuatorCoords, float polarCoordsAzFirst[], AxesParams& axesParams)
    {
        // Calculate azimuth
        int alphaSteps = (abs(int(actuatorCoords.getVal(0))) % (int)(axesParams.getStepsPerRot(0)));
        double alphaDegs = alphaSteps / axesParams.getStepsPerUnit(0);
        if (actuatorCoords.getVal(0) < 0)
            alphaDegs = axesParams.getunitsPerRot(0)-alphaDegs;
        polarCoordsAzFirst[0] = AxisUtils::d2r(alphaDegs);

        // Calculate linear position (note that this robot has interaction between azimuth and linear motion as the rack moves
        // if the pinion gear remains still and the arm assembly moves around it) - so the required linear calculation uses the
        // difference in linear and arm rotation steps
        long linearStepsFromHome = actuatorCoords.getVal(1) - actuatorCoords.getVal(0);
        polarCoordsAzFirst[1] = linearStepsFromHome / axesParams.getStepsPerUnit(1);

        // Log.trace("actuatorToPolar c0 %F c1 %F alphaSteps %d alphaDegs %F linStpHm %d rotD %F lin %F\n", actuatorCoords[0], actuatorCoords[1],
        //     alphaSteps, alphaDegs, linearStepsFromHome, polarCoordsAzFirst[0] * 180 / M_PI, polarCoordsAzFirst[1]);
    }

    static void actuatorToPt(AxisInt32s& targetActuator, AxisFloats& outPt, AxisPosition& curPos, AxesParams& axesParams)
    {
        float polarCoords[NUM_ROBOT_AXES];
        actuatorToPolar(targetActuator, polarCoords, axesParams);

        // Trig
        outPt._pt[0] = polarCoords[1] * sinf(polarCoords[0]);
        outPt._pt[1] = polarCoords[1] * cosf(polarCoords[0]);
        // Log.trace("actuatorToXy curX %F curY %F\n", xy[0], xy[1]);
    }

    static void correctStepOverflow(AxisPosition& curPos, AxesParams& axesParams)
    {
        int rotationSteps = (int)(axesParams.getStepsPerRot(0));
        // Debug
        bool showDebug = false;
        if (axesParams.gethomeOffSteps(0) > rotationSteps || axesParams.gethomeOffSteps(0) <= -rotationSteps)
        {
            Log.trace("CORRECTING ax0 %d ax1 %d\n",
                            axesParams.gethomeOffSteps(0), axesParams.gethomeOffSteps(1));
            showDebug = true;
        }
        // Bring steps from home values back within a single rotation
        while (axesParams.gethomeOffSteps(0) > rotationSteps)
        {
            axesParams.sethomeOffSteps(0, axesParams.gethomeOffSteps(0) - rotationSteps);
            axesParams.sethomeOffSteps(1, axesParams.gethomeOffSteps(1) - rotationSteps);
        }
        while (axesParams.gethomeOffSteps(0) <= -rotationSteps)
        {
            axesParams.sethomeOffSteps(0, axesParams.gethomeOffSteps(0) + rotationSteps);
            axesParams.sethomeOffSteps(1, axesParams.gethomeOffSteps(1) + rotationSteps);
        }
        if (showDebug)
            Log.trace("CORRECTED ax0 %d ax1 %d\n",
                            axesParams.gethomeOffSteps(0), axesParams.gethomeOffSteps(1));
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

private:
    // Homing state
    typedef enum HOMING_STATE
    {
        HOMING_STATE_IDLE,
        HOMING_STATE_INIT,
        ROTATE_FROM_ENDSTOP,
        ROTATE_TO_ENDSTOP,
        ROTATE_FOR_HOME_SET,
        ROTATE_TO_LINEAR_SEEK_ANGLE,
        LINEAR_SEEK_ENDSTOP,
        LINEAR_CLEAR_ENDSTOP,
        LINEAR_FULLY_CLEAR_ENDSTOP,
        LINEAR_SLOW_ENDSTOP,
        OFFSET_TO_CENTRE,
        ROTATE_TO_HOME,
        HOMING_STATE_COMPLETE
    } HOMING_STATE;
    HOMING_STATE _homingState;
    HOMING_STATE _homingStateNext;

    // Homing variables
    unsigned long _homeReqMillis;
    int _homingStepsDone;
    int _homingStepsLimit;
    bool _homingApplyStepLimit;
    unsigned long _maxHomingSecs;
    double _homingLinOffsetDegs;
    double _homingRotCentreDegs;
    double _homingCentreOffsetMM;
    typedef enum HOMING_SEEK_TYPE { HSEEK_NONE, HSEEK_ON, HSEEK_OFF } HOMING_SEEK_TYPE;
    HOMING_SEEK_TYPE _homingSeekAxis0Endstop0;
    HOMING_SEEK_TYPE _homingSeekAxis1Endstop0;
    // the following values determine which stepper moves during the current homing stage
    typedef enum HOMING_STEP_TYPE { HSTEP_NONE, HSTEP_FORWARDS, HSTEP_BACKWARDS } HOMING_STEP_TYPE;
    HOMING_STEP_TYPE _homingAxis0Step;
    HOMING_STEP_TYPE _homingAxis1Step;
    double _timeBetweenHomingStepsUs;

    // Set config
//     bool init(const char* robotConfigStr)
//     {
//         // Info
//         // Log.notice("Constructing %s from %s\n", _robotTypeName.c_str(), robotConfigStr);
//
//         // Init motion controller from config
//         _motionHelper.configure(robotConfigStr);
//
//         // Get params specific to this robot
//         _maxHomingSecs = RdJson::getLong("maxHomingSecs", maxHomingSecs_default, robotConfigStr);
//         _homingLinOffsetDegs = RdJson::getDouble("homingLinOffsetDegs", homingLinOffsetDegs_default, robotConfigStr);
//         _homingCentreOffsetMM = RdJson::getDouble("homingCentreOffsetMM", homingCentreOffsetMM_default, robotConfigStr);
//         _homingRotCentreDegs = RdJson::getDouble("homingRotCentreDegs", homingRotCentreDegs_default, robotConfigStr);
//
//         // Info
//         Log.notice("%s maxHome %ds linOff %Fd ctrOff %Fmm rotCtr %Fd\n",
//                     _robotTypeName.c_str(), _maxHomingSecs, _homingLinOffsetDegs, _homingCentreOffsetMM, _homingRotCentreDegs);
//
//         return true;
//     }
//
//     // void goHome(RobotCommandArgs& args)
//     // {
//     //     // GeistBot can only home X & Y axes together so ignore params
//     //     Log.notice("%s goHome\n", _robotTypeName.c_str());
//     //
//     //     // Set homing state
//     //     homingSetNewState(HOMING_STATE_INIT);
//     // }
//
//     bool canAcceptCommand()
//     {
//         // Check if homing
//         if (_homingState != HOMING_STATE_IDLE)
//             return false;
//
//         // Check if motionHelper can accept a command
//         return _motionHelper.canAccept();
//     }
//
//     void moveTo(RobotCommandArgs& args)
//     {
//         _motionHelper.moveTo(args);
//     }
//
//     void setMotionParams(RobotCommandArgs& args)
//     {
//         _motionHelper.setMotionParams(args);
//     }
//
//     void getCurStatus(RobotCommandArgs& args)
//     {
//         _motionHelper.getCurStatus(args);
//     }
//
//     bool lastTimeActiveMillis(unsigned long& lastMillis)
//     {
// 	return 0;
//     }
//
// private:
//     // void homingSetNewState(HOMING_STATE newState)
//     // {
//     //     // Debug
//     //     if (_homingStepsDone != 0)
//     //         Log.trace("Changing state ... HomingSteps %d\n", _homingStepsDone);
//     //
//     //     // Reset homing vars
//     //     _homingStepsDone = 0;
//     //     _homingStepsLimit = 0;
//     //     _homingApplyStepLimit = false;
//     //     _homingSeekAxis0Endstop0 = HSEEK_NONE;
//     //     _homingSeekAxis1Endstop0 = HSEEK_NONE;
//     //     _homingAxis0Step = HSTEP_NONE;
//     //     _homingAxis1Step = HSTEP_NONE;
//     //     _homingState = newState;
//     //
//     //     // Handle the specfics of the new homing state
//     //     switch(newState)
//     //     {
//     //         case HOMING_STATE_IDLE:
//     //         {
//     //             break;
//     //         }
//     //         case HOMING_STATE_INIT:
//     //         {
//     //             _homeReqMillis = millis();
//     //             // If we are at the rotation endstop we need to move away from it first
//     //             _homingStateNext = ROTATE_TO_ENDSTOP;
//     //             _homingSeekAxis0Endstop0 = HSEEK_OFF;
//     //             // To purely rotate both steppers must turn in the same direction
//     //             _homingAxis0Step = HSTEP_FORWARDS;
//     //             _homingAxis1Step = HSTEP_FORWARDS;
//     //             _timeBetweenHomingStepsUs = _homingRotateFastStepTimeUs;
//     //             Log.notice("Homing started\n");
//     //             break;
//     //         }
//     //         case ROTATE_TO_ENDSTOP:
//     //         {
//     //             // Rotate to the rotation endstop
//     //             _homingStateNext = ROTATE_FOR_HOME_SET;
//     //             _homingSeekAxis0Endstop0 = HSEEK_ON;
//     //             // To purely rotate both steppers must turn in the same direction
//     //             _homingAxis0Step = HSTEP_BACKWARDS;
//     //             _homingAxis1Step = HSTEP_BACKWARDS;
//     //             Log.trace("Homing - rotating to end stop 0\n");
//     //             break;
//     //         }
//     //         case ROTATE_FOR_HOME_SET:
//     //         {
//     //             // Rotate to the rotation endstop
//     //             _homingStateNext = ROTATE_TO_LINEAR_SEEK_ANGLE;
//     //             _homingStepsLimit = _homingRotCentreDegs * _motionHelper.getStepsPerUnit(0);
//     //             _homingApplyStepLimit = true;
//     //             // To purely rotate both steppers must turn in the same direction
//     //             _homingAxis0Step = HSTEP_BACKWARDS;
//     //             _homingAxis1Step = HSTEP_BACKWARDS;
//     //             Log.trace("Homing - rotating to home position 0\n");
//     //             break;
//     //         }
//     //         case ROTATE_TO_LINEAR_SEEK_ANGLE:
//     //         {
//     //             _homingStateNext = LINEAR_SEEK_ENDSTOP;
//     //             _motionHelper.setCurPositionAsHome(true,false,false);
//     //             _homingStepsLimit = _homingLinOffsetDegs * _motionHelper.getStepsPerUnit(0);
//     //             _homingApplyStepLimit = true;
//     //             // To purely rotate both steppers must turn in the same direction
//     //             _homingAxis0Step = HSTEP_FORWARDS;
//     //             _homingAxis1Step = HSTEP_FORWARDS;
//     //             Log.trace("Homing - at rotate endstop, prep linear seek %d steps back\n", _homingStepsLimit);
//     //             break;
//     //         }
//     //         case LINEAR_SEEK_ENDSTOP:
//     //         {
//     //             // Seek the linear end stop
//     //             _homingStateNext = LINEAR_CLEAR_ENDSTOP;
//     //             _homingSeekAxis1Endstop0 = HSEEK_ON;
//     //             // For linear motion just the rack and pinion stepper needs to rotate
//     //             // - forwards is towards the centre in this case
//     //             _homingAxis0Step = HSTEP_NONE;
//     //             _homingAxis1Step = HSTEP_BACKWARDS;
//     //             _timeBetweenHomingStepsUs = _homingLinearFastStepTimeUs;
//     //             Log.trace("Homing - linear seek\n");
//     //             break;
//     //         }
//     //         case LINEAR_CLEAR_ENDSTOP:
//     //         {
//     //             // If we are at the linear endstop we need to move away from it first
//     //             _homingStateNext = LINEAR_FULLY_CLEAR_ENDSTOP;
//     //             _homingSeekAxis1Endstop0 = HSEEK_OFF;
//     //             // For linear motion just the rack and pinion stepper needs to rotate
//     //             // - forwards is towards the centre in this case
//     //             _homingAxis0Step = HSTEP_NONE;
//     //             _homingAxis1Step = HSTEP_FORWARDS;
//     //             Log.trace("Homing - clear endstop\n");
//     //             break;
//     //         }
//     //         case LINEAR_FULLY_CLEAR_ENDSTOP:
//     //         {
//     //             // Move a little from the further from the endstop
//     //             _homingStateNext = LINEAR_SLOW_ENDSTOP;
//     //             _homingStepsLimit = 5 * _motionHelper.getStepsPerUnit(1);
//     //             _homingApplyStepLimit = true;
//     //             // For linear motion just the rack and pinion stepper needs to rotate
//     //             // - forwards is towards the centre in this case
//     //             _homingAxis0Step = HSTEP_NONE;
//     //             _homingAxis1Step = HSTEP_FORWARDS;
//     //             Log.trace("Homing - nudge away from linear endstop\n");
//     //             break;
//     //         }
//     //         case LINEAR_SLOW_ENDSTOP:
//     //         {
//     //             // Seek the linear end stop
//     //             _homingStateNext = OFFSET_TO_CENTRE;
//     //             _homingSeekAxis1Endstop0 = HSEEK_ON;
//     //             // For linear motion just the rack and pinion stepper needs to rotate
//     //             // - forwards is towards the centre in this case
//     //             _homingAxis0Step = HSTEP_NONE;
//     //             _homingAxis1Step = HSTEP_BACKWARDS;
//     //             _timeBetweenHomingStepsUs = _homingLinearSlowStepTimeUs;
//     //             Log.trace("Homing - linear seek\n");
//     //             break;
//     //         }
//     //         case OFFSET_TO_CENTRE:
//     //         {
//     //             // Move a little from the end stop to reach centre of machine
//     //             _homingStateNext = ROTATE_TO_HOME;
//     //             _homingStepsLimit = _homingCentreOffsetMM * _motionHelper.getStepsPerUnit(1);
//     //             _homingApplyStepLimit = true;
//     //             // For linear motion just the rack and pinion stepper needs to rotate
//     //             // - forwards is towards the centre in this case
//     //             _homingAxis0Step = HSTEP_NONE;
//     //             _homingAxis1Step = HSTEP_BACKWARDS;
//     //             Log.trace("Homing - offet to centre\n");
//     //             break;
//     //         }
//     //         case ROTATE_TO_HOME:
//     //         {
//     //             _homingStateNext = HOMING_STATE_COMPLETE;
//     //             _homingStepsLimit = abs(_motionHelper.gethomeOffSteps(0));
//     //             _homingApplyStepLimit = true;
//     //             // To purely rotate both steppers must turn in the same direction
//     //             _homingAxis0Step = HSTEP_BACKWARDS;
//     //             _homingAxis1Step = HSTEP_BACKWARDS;
//     //             Log.trace("Homing - rotating back home %d steps\n", _homingStepsLimit);
//     //             break;
//     //         }
//     //         case HOMING_STATE_COMPLETE:
//     //         {
//     //             _motionHelper.setCurPositionAsHome(false,true,false);
//     //             _homingState = HOMING_STATE_IDLE;
//     //             Log.notice("Homing - complete\n");
//     //             break;
//     //         }
//     //     }
//     // }
//
//     bool homingService()
//     {
//         // // Check for idle
//         // if (_homingState == HOMING_STATE_IDLE)
//         //     return false;
//         //
//         // // Check for timeout
//         // if (millis() > _homeReqMillis + (_maxHomingSecs * 1000))
//         // {
//         //     Log.notice("Homing Timed Out\n");
//         //     homingSetNewState(HOMING_STATE_IDLE);
//         // }
//         //
//         // // Check for endstop if seeking them
//         // bool endstop0Val = _motionHelper.isAtEndStop(0,0);
//         // bool endstop1Val = _motionHelper.isAtEndStop(1,0);
//         // if (((_homingSeekAxis0Endstop0 == HSEEK_ON) && endstop0Val) || ((_homingSeekAxis0Endstop0 == HSEEK_OFF) && !endstop0Val))
//         // {
//         //     homingSetNewState(_homingStateNext);
//         // }
//         // if (((_homingSeekAxis1Endstop0 == HSEEK_ON) && endstop1Val) || ((_homingSeekAxis1Endstop0 == HSEEK_OFF) && !endstop1Val))
//         // {
//         //     homingSetNewState(_homingStateNext);
//         // }
//         //
//         // // Check if we are ready for the next step
//         // unsigned long lastStepMicros = _motionHelper.getAxisLastStepMicros(0);
//         // unsigned long lastStepMicros1 = _motionHelper.getAxisLastStepMicros(1);
//         // if (lastStepMicros < lastStepMicros1)
//         //     lastStepMicros = lastStepMicros1;
//         // if (Utils::isTimeout(micros(), lastStepMicros, _timeBetweenHomingStepsUs))
//         // {
//         //     // Axis 0
//         //     if (_homingAxis0Step != HSTEP_NONE)
//         //         _motionHelper.step(0, _homingAxis0Step == HSTEP_FORWARDS);
//         //
//         //     // Axis 1
//         //     if (_homingAxis1Step != HSTEP_NONE)
//         //         _motionHelper.step(1, _homingAxis1Step == HSTEP_FORWARDS);
//         //
//         //     // Count homing steps in this stage
//         //     _homingStepsDone++;
//         //
//         //     // Check for step limit in this stage
//         //     if (_homingApplyStepLimit && (_homingStepsDone >= _homingStepsLimit))
//         //         homingSetNewState(_homingStateNext);
//         //
//         //     // // Debug
//         //     // if (_homingStepsDone % 250 == 0)
//         //     // {
//         //     //     const char* pStr = "ROT_ACW";
//         //     //     if (_homingAxis0Step == HSTEP_NONE)
//         //     //     {
//         //     //         if (_homingAxis1Step == HSTEP_NONE)
//         //     //             pStr = "NONE";
//         //     //         else if (_homingAxis1Step == HSTEP_FORWARDS)
//         //     //             pStr = "LIN_FOR";
//         //     //         else
//         //     //             pStr = "LIN_BAK";
//         //     //     }
//         //     //     else if (_homingAxis0Step == HSTEP_FORWARDS)
//         //     //     {
//         //     //         pStr = "ROT_CW";
//         //     //     }
//         //     //     Log.trace("HomingSteps %d %s\n", _homingStepsDone, pStr);
//         //     // }
//         // }
//         //
//         // // // Check for linear endstop if seeking it
//         // // if (_homingState == LINEAR_TO_ENDSTOP)
//         // // {
//         // //     if (_motionHelper.isAtEndStop(1,0))
//         // //     {
//         // //         _homingState = OFFSET_TO_CENTRE;
//         // //         _homingStepsLimit = _homingCentreOffsetMM * _motionHelper.getAxisParams(1)._stepsPerUnit;
//         // //         _homingStepsDone = 0;
//         // //         Log.notice("Homing - %d steps to centre\n", _homingStepsLimit);
//         // //         break;
//         // //     }
//         // // }
//         //
//         return true;
//
//     }
//
// public:
//     // Pause (or un-pause) all motion
//     void pause(bool pauseIt)
//     {
//         _motionHelper.pause(pauseIt);
//     }
//
//     // Check if paused
//     bool isPaused()
//     {
//         return _motionHelper.isPaused();
//     }
//
//     // Stop
//     void stop()
//     {
//         _motionHelper.stop();
//     }
//
//     void service()
//     {
//         // Service homing activity
//         bool homingActive = homingService();
//
//         // Service the motion controller
//         _motionHelper.service(!homingActive);
//     }
//
//     bool wasActiveInLastNSeconds(unsigned int nSeconds)
//     {
//         if (_homingState != HOMING_STATE_IDLE)
//             return true;
//         return ((unsigned long)Time.now() < _motionHelper.getLastActiveUnixTime() + nSeconds);
//     }
};
