// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "Utils.h"
#include "RobotPolar.h"

class RobotGeistBot : public RobotPolar
{
private:
    enum HOMING_STATE
    {
        HOMING_STATE_IDLE,
        HOMING_STATE_INIT,
        ROTATE_TO_ENDSTOP,
        PREP_FOR_LINEAR_SEEK,
        LINEAR_TO_ENDSTOP,
    };
    HOMING_STATE _homingState;
    unsigned long _homeReqTime;
    static const int _timeBetweenHomingStepsUs = 500;
    int _homingStepsDone;
    int _homingStepsRequired;



    bool testWasOn;
    int testRotCount;

public:
    RobotGeistBot(const char* pRobotTypeName) :
        RobotPolar(pRobotTypeName)
    {
        _homingState = HOMING_STATE_IDLE;
        _homeReqTime = 0;
        _homingStepsDone = 0;
        _homingStepsRequired = 0;
    }

    void home(RobotCommandArgs& args)
    {
        // GeistBot can only home X & Y axes together so ignore params
        // Info
        Log.info("%s home x%d, y%d, z%d", _robotTypeName.c_str(), args.xValid, args.yValid, args.zValid);

        // Set homing state
        _homingState = HOMING_STATE_INIT;
        _homingStepsDone = 0;
        _homingStepsRequired = 0;
    }

    bool isBusy()
    {
        // Check if homing
        if (_homingState != HOMING_STATE_IDLE)
            return true;

        // Check if steppers busy
        return RobotPolar::isBusy();
    }

    void service()
    {
        // Service the base class
        RobotPolar::service();

        // Check for homing activity
        switch (_homingState)
        {
            case HOMING_STATE_INIT:
            {
                Log.info("Homing ");
                _homeReqTime = millis();
                _homingState = ROTATE_TO_ENDSTOP;
                _homingStepsDone = 0;
                _homingStepsRequired = 0;
                break;
            }
            case ROTATE_TO_ENDSTOP:
            case PREP_FOR_LINEAR_SEEK:
            {
                // Check for timeout
                if (millis() > _homeReqTime + 200000)
                {
                    _homingState = HOMING_STATE_IDLE;
                    Log.info("Homing Timed Out");
                }

                // Check for rotation endstop if seeking it
                if (_homingState == ROTATE_TO_ENDSTOP)
                {
                    if (_motionController.isAtEndStop(0,0))
                    {
                        _homingState = PREP_FOR_LINEAR_SEEK;
                        testWasOn = 1;
                        testRotCount = 0;
                        _homingStepsDone = 0;
                        _homingStepsRequired = 100000;
                        Log.info("Homing - at rotate endstop");
                    }
                }

                // Check if we are ready for the next step
                unsigned long lastStepMicros = _motionController.getAxisLastStepMicros(0);
                if (Utils::isTimeout(micros(), lastStepMicros, _timeBetweenHomingStepsUs))
                {
                    // Check which direction to rotate
                    bool rotateDirection = 0;
                    if (_homingState == PREP_FOR_LINEAR_SEEK)
                        rotateDirection = 0;

                    // Rotate
                    if (_homingState == ROTATE_TO_ENDSTOP || _homingState == PREP_FOR_LINEAR_SEEK)
                        _motionController.step(0, rotateDirection);
                    if (_homingState == ROTATE_TO_ENDSTOP || _homingState == PREP_FOR_LINEAR_SEEK)
                        _motionController.step(1, rotateDirection);
                }

                // Count homing steps in this stage
                _homingStepsDone++;

                // Check for max steps in this stage
                if (_homingStepsRequired != 0)
                {
                    if (_motionController.isAtEndStop(0,0) && !testWasOn)
                    {
                        testRotCount++;
                        Log.info("Reached endstop again - diff steps = %d, rotations %d", _homingStepsDone, testRotCount);
                        if (testRotCount >= 10)
                        {
                            _homingState = HOMING_STATE_IDLE;
                        }
                        testWasOn = true;
                    }
                    else if (!_motionController.isAtEndStop(0,0))
                    {
                        testWasOn = false;
                    }
                    if (_homingStepsDone >= _homingStepsRequired)
                    {
                        if (_homingState == PREP_FOR_LINEAR_SEEK)
                        {
                            _homingState = HOMING_STATE_IDLE;
                            Log.info("Homing - at linear point");
                        }
                    }
                }

                if (_homingStepsDone % 1000 == 0)
                {
                    Serial.printlnf("HomingSteps %d", _homingStepsDone);
                }

                break;
            }
        }
    }
};
