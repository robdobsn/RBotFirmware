// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "Utils.h"
#include "RobotBase.h"
#include "MotionController.h"
#include "math.h"

class RobotGeistBot : public RobotBase
{
public:
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

    static bool ptToActuator(PointND& pt, PointND& actuatorCoords, AxisParams axisParams[], int numAxes)
    {
        // Trig for required position (azimuth is measured clockwise from North)
        double reqAlphaRads = atan2(pt._pt[0], pt._pt[1]);
        if (reqAlphaRads < 0)
            reqAlphaRads = 2 * M_PI + reqAlphaRads;
        double reqLinearMM = sqrt(pt._pt[0] * pt._pt[0] + pt._pt[1] * pt._pt[1]);

        // Log.trace("xyToActuator x %0.2f y %0.2f ax0StNow %d ax1StNow %d rqAlphaD %0.2f rqLinMM %0.2f",
        //         xy[0], xy[1], axisParams[0]._stepsFromHome, axisParams[1]._stepsFromHome,
        //         reqAlphaRads * 180 / M_PI, reqLinearMM);

        // Get current position
        PointND axisPosns;
        for (int i = 0; i < MAX_AXES; i++)
            axisPosns._pt[i] = axisParams[i]._stepsFromHome;
        double currentPolar[NUM_ROBOT_AXES];
        actuatorToPolar(axisPosns, currentPolar, axisParams, 2);

        // Calculate shortest azimuth distance
        double alphaDiffRads = reqAlphaRads - currentPolar[0];
        bool alphaRotateCw = alphaDiffRads > 0;
        if (fabs(alphaDiffRads) > M_PI)
        {
            alphaDiffRads = 2 * M_PI - fabs(alphaDiffRads);
            alphaRotateCw = !alphaRotateCw;
        }
        double alphaDiffDegs = fabs(alphaDiffRads) * 180 / M_PI;

        // Linear distance
        double linearDiffMM = reqLinearMM - currentPolar[1];

        // Convert to steps - note that to keep the linear position constant the linear stepper needs to step one step in the same
        // direction as the arm rotation stepper - so the linear stepper steps required is the sum of the rotation and linear steps
        double actuator0Diff = alphaDiffDegs * axisParams[0].stepsPerUnit() * (alphaRotateCw ? 1 : -1);
        double actuator1Diff = linearDiffMM * axisParams[1].stepsPerUnit() + actuator0Diff;
        actuatorCoords._pt[0] = axisParams[0]._stepsFromHome + actuator0Diff;
        actuatorCoords._pt[1] = axisParams[1]._stepsFromHome + actuator1Diff;

        // Log.trace("xyToActuator reqAlphaD %0.2f curAlphaD %0.2f alphaDiffD %0.2f aRotCw %d linDiffMM %0.2f ax0Diff %0.2f ax1Diff %0.2f ax0Tgt %0.2f ax1Tgt %0.2f",
        //                 reqAlphaRads * 180 / M_PI, currentPolar[0] * 180 / M_PI, alphaDiffDegs, alphaRotateCw,
        //                 linearDiffMM, actuator0Diff, actuator1Diff, actuatorCoords[0], actuatorCoords[1]);
        //
        // Log.trace("bounds check X (En%d) %d, Y (En%d) %d", axisParams[1]._minValValid, reqLinearMM < axisParams[1]._minVal,
        //             axisParams[1]._maxValValid, reqLinearMM > axisParams[1]._maxVal);

        // Cross check
        double checkPolarCoords[NUM_ROBOT_AXES];
        actuatorToPolar(actuatorCoords, checkPolarCoords, axisParams, 2);
        double checkX = checkPolarCoords[1] * sin(checkPolarCoords[0]);
        double checkY = checkPolarCoords[1] * cos(checkPolarCoords[0]);
        double checkErr = sqrt((checkX-pt._pt[0]) * (checkX-pt._pt[0]) + (checkY-pt._pt[1]) * (checkY-pt._pt[1]));
        if (checkErr > 0.1)
            Log.trace("check reqX %02.f checkX %0.2f, reqY %0.2f checkY %0.2f, error %0.2f, %s", pt._pt[0], checkX, pt._pt[1], checkY,
                            checkErr, (checkErr>0.1) ? "****** FAILED ERROR CHECK" : "");

        // Check machine bounds for linear axis
        if (axisParams[1]._minValValid && reqLinearMM < axisParams[1]._minVal)
            return false;
        if (axisParams[1]._maxValValid && reqLinearMM > axisParams[1]._maxVal)
            return false;
        return true;
    }

    static void actuatorToPolar(PointND& actuatorCoords, double polarCoordsAzFirst[], AxisParams axisParams[], int numAxes)
    {
        // Calculate azimuth
        int alphaSteps = (abs(int(actuatorCoords._pt[0])) % (int)(axisParams[0]._stepsPerRotation));
        double alphaDegs = alphaSteps / axisParams[0].stepsPerUnit();
        if (actuatorCoords._pt[0] < 0)
            alphaDegs = axisParams[0]._unitsPerRotation-alphaDegs;
        polarCoordsAzFirst[0] = alphaDegs * M_PI / 180;

        // Calculate linear position (note that this robot has interaction between azimuth and linear motion as the rack moves
        // if the pinion gear remains still and the arm assembly moves around it) - so the required linear calculation uses the
        // difference in linear and arm rotation steps
        long linearStepsFromHome = actuatorCoords._pt[1] - actuatorCoords._pt[0];
        polarCoordsAzFirst[1] = linearStepsFromHome / axisParams[1].stepsPerUnit();

        // Log.trace("actuatorToPolar c0 %0.2f c1 %0.2f alphaSteps %d alphaDegs %0.2f linStpHm %d rotD %0.2f lin %0.2f", actuatorCoords[0], actuatorCoords[1],
        //     alphaSteps, alphaDegs, linearStepsFromHome, polarCoordsAzFirst[0] * 180 / M_PI, polarCoordsAzFirst[1]);
    }

    static void actuatorToPt(PointND& actuatorCoords, PointND& pt, AxisParams axisParams[], int numAxes)
    {
        double polarCoords[NUM_ROBOT_AXES];
        actuatorToPolar(actuatorCoords, polarCoords, axisParams, numAxes);

        // Trig
        pt._pt[0] = polarCoords[1] * sin(polarCoords[0]);
        pt._pt[1] = polarCoords[1] * cos(polarCoords[0]);
        // Log.trace("actuatorToXy curX %0.2f curY %0.2f", xy[0], xy[1]);
    }

    static void correctStepOverflow(AxisParams axisParams[], int numAxes)
    {
        int rotationSteps = (int)(axisParams[0]._stepsPerRotation);
        // Debug
        bool showDebug = false;
        if (axisParams[0]._stepsFromHome > rotationSteps || axisParams[0]._stepsFromHome <= -rotationSteps)
        {
            Log.trace("CORRECTING ax0 %d ax1 %d", axisParams[0]._stepsFromHome, axisParams[1]._stepsFromHome);
            showDebug = true;
        }
        // Bring steps from home values back within a single rotation
        while (axisParams[0]._stepsFromHome > rotationSteps)
        {
            axisParams[0]._stepsFromHome -= rotationSteps;
            axisParams[1]._stepsFromHome -= rotationSteps;
        }
        while (axisParams[0]._stepsFromHome <= -rotationSteps)
        {
            axisParams[0]._stepsFromHome += rotationSteps;
            axisParams[1]._stepsFromHome += rotationSteps;
        }
        if (showDebug)
            Log.trace("CORRECTED ax0 %d ax1 %d", axisParams[0]._stepsFromHome, axisParams[1]._stepsFromHome);
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
    unsigned long _homeReqTime;
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

    // MotionController for the robot motion
    MotionController _motionController;

public:
    RobotGeistBot(const char* pRobotTypeName) :
        RobotBase(pRobotTypeName)
    {
        _homingState = HOMING_STATE_IDLE;
        _homeReqTime = 0;
        _homingStepsDone = 0;
        _homingStepsLimit = 0;
        _maxHomingSecs = maxHomingSecs_default;
        _timeBetweenHomingStepsUs = _homingRotateSlowStepTimeUs;
        _motionController.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
    }

    // Set config
    bool init(const char* robotConfigStr)
    {
        // Info
        // Log.info("Constructing %s from %s", _robotTypeName.c_str(), robotConfigStr);

        // Init motion controller from config
        _motionController.setAxisParams(robotConfigStr);

        // Get params specific to GeistBot
        _maxHomingSecs = ConfigManager::getLong("maxHomingSecs", maxHomingSecs_default, robotConfigStr);
        _homingLinOffsetDegs = ConfigManager::getDouble("homingLinOffsetDegs", homingLinOffsetDegs_default, robotConfigStr);
        _homingCentreOffsetMM = ConfigManager::getDouble("homingCentreOffsetMM", homingCentreOffsetMM_default, robotConfigStr);
        _homingRotCentreDegs = ConfigManager::getDouble("homingRotCentreDegs", homingRotCentreDegs_default, robotConfigStr);

        // Info
        Log.info("%s maxHome %ds linOff %0.3fd ctrOff %0.3fmm rotCtr %0.3fd",
                    _robotTypeName.c_str(), _maxHomingSecs, _homingLinOffsetDegs, _homingCentreOffsetMM, _homingRotCentreDegs);

        return true;
    }

    void home(RobotCommandArgs& args)
    {
        // GeistBot can only home X & Y axes together so ignore params
        // Info
        Log.info("%s home x%d, y%d, z%d", _robotTypeName.c_str(), args.valid.X(), args.valid.Y(), args.valid.Z());

        // Set homing state
        homingSetNewState(HOMING_STATE_INIT);
    }

    bool canAcceptCommand()
    {
        // Check if homing
        if (_homingState != HOMING_STATE_IDLE)
            return false;

        // Check if motionController is can accept a command
        return _motionController.canAcceptCommand();
    }

    void moveTo(RobotCommandArgs& args)
    {
        _motionController.moveTo(args);
    }

    void homingSetNewState(HOMING_STATE newState)
    {
        // Debug
        if (_homingStepsDone != 0)
            Log.trace("Changing state ... HomingSteps %d", _homingStepsDone);

        // Reset homing vars
        _homingStepsDone = 0;
        _homingStepsLimit = 0;
        _homingApplyStepLimit = false;
        _homingSeekAxis0Endstop0 = HSEEK_NONE;
        _homingSeekAxis1Endstop0 = HSEEK_NONE;
        _homingAxis0Step = HSTEP_NONE;
        _homingAxis1Step = HSTEP_NONE;
        HOMING_STATE prevState = _homingState;
        _homingState = newState;

        // Handle the specfics of the new homing state
        switch(newState)
        {
            case HOMING_STATE_IDLE:
            {
                break;
            }
            case HOMING_STATE_INIT:
            {
                _homeReqTime = millis();
                // If we are at the rotation endstop we need to move away from it first
                _homingStateNext = ROTATE_TO_ENDSTOP;
                _homingSeekAxis0Endstop0 = HSEEK_OFF;
                // To purely rotate both steppers must turn in the same direction
                _homingAxis0Step = HSTEP_FORWARDS;
                _homingAxis1Step = HSTEP_FORWARDS;
                _timeBetweenHomingStepsUs = _homingRotateFastStepTimeUs;
                Log.info("Homing started");
                break;
            }
            case ROTATE_TO_ENDSTOP:
            {
                // Rotate to the rotation endstop
                _homingStateNext = ROTATE_FOR_HOME_SET;
                _homingSeekAxis0Endstop0 = HSEEK_ON;
                // To purely rotate both steppers must turn in the same direction
                _homingAxis0Step = HSTEP_BACKWARDS;
                _homingAxis1Step = HSTEP_BACKWARDS;
                Log.trace("Homing - rotating to end stop 0");
                break;
            }
            case ROTATE_FOR_HOME_SET:
            {
                // Rotate to the rotation endstop
                _homingStateNext = ROTATE_TO_LINEAR_SEEK_ANGLE;
                _homingStepsLimit = _homingRotCentreDegs * _motionController.getStepsPerUnit(0);
                _homingApplyStepLimit = true;
                // To purely rotate both steppers must turn in the same direction
                _homingAxis0Step = HSTEP_BACKWARDS;
                _homingAxis1Step = HSTEP_BACKWARDS;
                Log.trace("Homing - rotating to home position 0");
                break;
            }
            case ROTATE_TO_LINEAR_SEEK_ANGLE:
            {
                _homingStateNext = LINEAR_SEEK_ENDSTOP;
                _motionController.axisIsHome(0);
                _homingStepsLimit = _homingLinOffsetDegs * _motionController.getStepsPerUnit(0);
                _homingApplyStepLimit = true;
                // To purely rotate both steppers must turn in the same direction
                _homingAxis0Step = HSTEP_FORWARDS;
                _homingAxis1Step = HSTEP_FORWARDS;
                Log.trace("Homing - at rotate endstop, prep linear seek %d steps back", _homingStepsLimit);
                break;
            }
            case LINEAR_SEEK_ENDSTOP:
            {
                // Seek the linear end stop
                _homingStateNext = LINEAR_CLEAR_ENDSTOP;
                _homingSeekAxis1Endstop0 = HSEEK_ON;
                // For linear motion just the rack and pinion stepper needs to rotate
                // - forwards is towards the centre in this case
                _homingAxis0Step = HSTEP_NONE;
                _homingAxis1Step = HSTEP_BACKWARDS;
                _timeBetweenHomingStepsUs = _homingLinearFastStepTimeUs;
                Log.trace("Homing - linear seek");
                break;
            }
            case LINEAR_CLEAR_ENDSTOP:
            {
                // If we are at the linear endstop we need to move away from it first
                _homingStateNext = LINEAR_FULLY_CLEAR_ENDSTOP;
                _homingSeekAxis1Endstop0 = HSEEK_OFF;
                // For linear motion just the rack and pinion stepper needs to rotate
                // - forwards is towards the centre in this case
                _homingAxis0Step = HSTEP_NONE;
                _homingAxis1Step = HSTEP_FORWARDS;
                Log.trace("Homing - clear endstop");
                break;
            }
            case LINEAR_FULLY_CLEAR_ENDSTOP:
            {
                // Move a little from the further from the endstop
                _homingStateNext = LINEAR_SLOW_ENDSTOP;
                _homingStepsLimit = 5 * _motionController.getStepsPerUnit(1);
                _homingApplyStepLimit = true;
                // For linear motion just the rack and pinion stepper needs to rotate
                // - forwards is towards the centre in this case
                _homingAxis0Step = HSTEP_NONE;
                _homingAxis1Step = HSTEP_FORWARDS;
                Log.trace("Homing - nudge away from linear endstop");
                break;
            }
            case LINEAR_SLOW_ENDSTOP:
            {
                // Seek the linear end stop
                _homingStateNext = OFFSET_TO_CENTRE;
                _homingSeekAxis1Endstop0 = HSEEK_ON;
                // For linear motion just the rack and pinion stepper needs to rotate
                // - forwards is towards the centre in this case
                _homingAxis0Step = HSTEP_NONE;
                _homingAxis1Step = HSTEP_BACKWARDS;
                _timeBetweenHomingStepsUs = _homingLinearSlowStepTimeUs;
                Log.trace("Homing - linear seek");
                break;
            }
            case OFFSET_TO_CENTRE:
            {
                // Move a little from the end stop to reach centre of machine
                _homingStateNext = ROTATE_TO_HOME;
                _homingStepsLimit = _homingCentreOffsetMM * _motionController.getStepsPerUnit(1);
                _homingApplyStepLimit = true;
                // For linear motion just the rack and pinion stepper needs to rotate
                // - forwards is towards the centre in this case
                _homingAxis0Step = HSTEP_NONE;
                _homingAxis1Step = HSTEP_BACKWARDS;
                Log.trace("Homing - offet to centre");
                break;
            }
            case ROTATE_TO_HOME:
            {
                _homingStateNext = HOMING_STATE_COMPLETE;
                _homingStepsLimit = abs(_motionController.getAxisStepsFromHome(0));
                _homingApplyStepLimit = true;
                // To purely rotate both steppers must turn in the same direction
                _homingAxis0Step = HSTEP_BACKWARDS;
                _homingAxis1Step = HSTEP_BACKWARDS;
                Log.trace("Homing - rotating back home %d steps", _homingStepsLimit);
                break;
            }
            case HOMING_STATE_COMPLETE:
            {
                _motionController.axisIsHome(1);
                _homingState = HOMING_STATE_IDLE;
                Log.info("Homing - complete");
                break;
            }
        }
    }

    bool homingService()
    {
        // Check for idle
        if (_homingState == HOMING_STATE_IDLE)
            return false;

        // Check for timeout
        if (millis() > _homeReqTime + (_maxHomingSecs * 1000))
        {
            Log.info("Homing Timed Out");
            homingSetNewState(HOMING_STATE_IDLE);
        }

        // Check for endstop if seeking them
        bool endstop0Val = _motionController.isAtEndStop(0,0);
        bool endstop1Val = _motionController.isAtEndStop(1,0);
        if (((_homingSeekAxis0Endstop0 == HSEEK_ON) && endstop0Val) || ((_homingSeekAxis0Endstop0 == HSEEK_OFF) && !endstop0Val))
        {
            homingSetNewState(_homingStateNext);
        }
        if (((_homingSeekAxis1Endstop0 == HSEEK_ON) && endstop1Val) || ((_homingSeekAxis1Endstop0 == HSEEK_OFF) && !endstop1Val))
        {
            homingSetNewState(_homingStateNext);
        }

        // Check if we are ready for the next step
        unsigned long lastStepMicros = _motionController.getAxisLastStepMicros(0);
        unsigned long lastStepMicros1 = _motionController.getAxisLastStepMicros(1);
        if (lastStepMicros < lastStepMicros1)
            lastStepMicros = lastStepMicros1;
        if (Utils::isTimeout(micros(), lastStepMicros, _timeBetweenHomingStepsUs))
        {
            // Axis 0
            if (_homingAxis0Step != HSTEP_NONE)
                _motionController.step(0, _homingAxis0Step == HSTEP_FORWARDS);

            // Axis 1
            if (_homingAxis1Step != HSTEP_NONE)
                _motionController.step(1, _homingAxis1Step == HSTEP_FORWARDS);

            // Count homing steps in this stage
            _homingStepsDone++;

            // Check for step limit in this stage
            if (_homingApplyStepLimit && (_homingStepsDone >= _homingStepsLimit))
                homingSetNewState(_homingStateNext);

            // // Debug
            // if (_homingStepsDone % 250 == 0)
            // {
            //     const char* pStr = "ROT_ACW";
            //     if (_homingAxis0Step == HSTEP_NONE)
            //     {
            //         if (_homingAxis1Step == HSTEP_NONE)
            //             pStr = "NONE";
            //         else if (_homingAxis1Step == HSTEP_FORWARDS)
            //             pStr = "LIN_FOR";
            //         else
            //             pStr = "LIN_BAK";
            //     }
            //     else if (_homingAxis0Step == HSTEP_FORWARDS)
            //     {
            //         pStr = "ROT_CW";
            //     }
            //     Serial.printlnf("HomingSteps %d %s", _homingStepsDone, pStr);
            // }
        }

        // // Check for linear endstop if seeking it
        // if (_homingState == LINEAR_TO_ENDSTOP)
        // {
        //     if (_motionController.isAtEndStop(1,0))
        //     {
        //         _homingState = OFFSET_TO_CENTRE;
        //         _homingStepsLimit = _homingCentreOffsetMM * _motionController.getAxisParams(1)._stepsPerUnit;
        //         _homingStepsDone = 0;
        //         Log.info("Homing - %d steps to centre", _homingStepsLimit);
        //         break;
        //     }
        // }

        return true;

    }

    void service()
    {
        // Service homing activity
        bool homingActive = homingService();

        // Service the motion controller
        _motionController.service(!homingActive);
    }

};
