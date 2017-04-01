// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "Utils.h"
#include "RobotBase.h"
#include "MotionController.h"
#include "math.h"

class RobotMugBot : public RobotBase
{
public:
    // Defaults
    static constexpr int maxHomingSecs_default = 30;
    static constexpr int _homingLinearFastStepTimeUs = 100;
    static constexpr int _homingLinearSlowStepTimeUs = 500;

public:

    static bool ptToActuator(PointND& pt, PointND& actuatorCoords, AxisParams axisParams[], int numAxes)
    {
        // Note that the rotation angle comes straight from the Y parameter
        // This means that drawings in the range 0 .. 240mm height (assuming 1:1 scaling is chosen)
        // will translate directly to the surface of the mug and makes the drawing
        // mug-radius independent

        bool isValid = true;
        for (int i = 0; i < MAX_AXES; i++)
        {
            // Axis val from home point
            double axisValFromHome = pt.getVal(i) - axisParams[i]._servoHomeVal;
            // Convert to steps and add offset to home in steps
            actuatorCoords.setVal(i, axisValFromHome * axisParams[i].stepsPerUnit()
                            + axisParams[i]._servoHomeSteps);
            // Check machine bounds
            if (axisParams[i]._minValValid && pt.getVal(i) < axisParams[i]._minVal)
                isValid = false;
            if (axisParams[i]._maxValValid && pt.getVal(i) > axisParams[i]._maxVal)
                isValid = false;
        }

        Log.info("actuatorCoords (%s) %f, %f, %f", isValid ? "OK" : "INVALID",
            actuatorCoords._pt[0], actuatorCoords._pt[1], actuatorCoords._pt[2]);

        return isValid;
    }

    // static void actuatorToPolar(PointND actuatorCoords, PointND polarCoordsAzFirst, AxisParams axisParams[], int numAxes)
    // {
    // }

    static void actuatorToPt(PointND& actuatorCoords, PointND& pt, AxisParams axisParams[], int numAxes)
    {
        for (int i = 0; i < MAX_AXES; i++)
        {
            pt.setVal(i, actuatorCoords.getVal(i) / axisParams[i].stepsPerUnit());
        }
    }

    static void correctStepOverflow(AxisParams axisParams[], int numAxes)
    {
    }

private:
    // Homing state
    typedef enum HOMING_STATE
    {
        HOMING_STATE_IDLE,
        HOMING_STATE_INIT,
        HOMING_STATE_SEEK_ENDSTOP,
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
    typedef enum HOMING_SEEK_TYPE { HSEEK_NONE, HSEEK_ON, HSEEK_OFF } HOMING_SEEK_TYPE;
    HOMING_SEEK_TYPE _homingSeekAxis1Endstop0;
    // the following values determine which stepper moves during the current homing stage
    typedef enum HOMING_STEP_TYPE { HSTEP_NONE, HSTEP_FORWARDS, HSTEP_BACKWARDS } HOMING_STEP_TYPE;
    HOMING_STEP_TYPE _homingAxis1Step;
    double _timeBetweenHomingStepsUs;

    // MotionController for the robot motion
    MotionController _motionController;

public:
    RobotMugBot(const char* pRobotTypeName) :
        RobotBase(pRobotTypeName)
    {
        _homingState = HOMING_STATE_IDLE;
        _homeReqTime = 0;
        _homingStepsDone = 0;
        _homingStepsLimit = 0;
        _maxHomingSecs = maxHomingSecs_default;
        _timeBetweenHomingStepsUs = _homingLinearSlowStepTimeUs;
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

        // Info
        Log.info("%s maxHome %ds", _robotTypeName.c_str(), _maxHomingSecs);

        return true;
    }

    void home(RobotCommandArgs& args)
    {
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
        _homingSeekAxis1Endstop0 = HSEEK_NONE;
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
                // If we are at the endstop we need to move away from it first
                _homingStateNext = HOMING_STATE_SEEK_ENDSTOP;
                _homingSeekAxis1Endstop0 = HSEEK_OFF;
                // Move away from endstop if needed
                _homingAxis1Step = HSTEP_FORWARDS;
                _timeBetweenHomingStepsUs = _homingLinearFastStepTimeUs;
                // _homingStepsLimit = 10000;
                // _homingApplyStepLimit = true;
                Log.info("Homing started");
                break;
            }
            case HOMING_STATE_SEEK_ENDSTOP:
            {
                // Rotate to the rotation endstop
                _homingStateNext = HOMING_STATE_COMPLETE;
                _homingSeekAxis1Endstop0 = HSEEK_ON;
                // To purely rotate both steppers must turn in the same direction
                _homingAxis1Step = HSTEP_BACKWARDS;
                _timeBetweenHomingStepsUs = _homingLinearSlowStepTimeUs;
                // _homingStepsLimit = 100000;
                // _homingApplyStepLimit = true;
                Log.trace("Homing to end stop");
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
        bool endstop1Val = _motionController.isAtEndStop(1,0);
        if (((_homingSeekAxis1Endstop0 == HSEEK_ON) && endstop1Val) || ((_homingSeekAxis1Endstop0 == HSEEK_OFF) && !endstop1Val))
        {
            homingSetNewState(_homingStateNext);
        }

        // Check if we are ready for the next step
        unsigned long lastStepMicros = _motionController.getAxisLastStepMicros(1);
        if (Utils::isTimeout(micros(), lastStepMicros, _timeBetweenHomingStepsUs))
        {
            // Axis 1
            if (_homingAxis1Step != HSTEP_NONE)
                _motionController.step(1, _homingAxis1Step == HSTEP_FORWARDS);

            // Count homing steps in this stage
            _homingStepsDone++;

            // Check for step limit in this stage
            if (_homingApplyStepLimit && (_homingStepsDone >= _homingStepsLimit))
                homingSetNewState(_homingStateNext);
        }

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
