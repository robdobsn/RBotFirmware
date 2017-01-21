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
        SEEK_FOR_LINEAR,
    };
    HOMING_STATE _homingState;
    unsigned long _homeReqTime;
    static const int _timeBetweenHomingStepsUs = 500;
    int _homingStepsDone;
    int _homingStepsRequired;

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
            case SEEK_FOR_LINEAR:
            {
                // Check for timeout
                if (millis() > _homeReqTime + 15000)
                {
                    _homingState = HOMING_STATE_IDLE;
                    Log.info("Homing Timed Out");
                }

                // Check for rotation endstop if seeking it
                if (_homingState == ROTATE_TO_ENDSTOP)
                {
                    if (_motionController.isAtEndStop(0,0))
                    {
                        _homingState = SEEK_FOR_LINEAR;
                        _homingStepsDone = 0;
                        _homingStepsRequired = 3000;
                        Log.info("Homing - at rotate endstop");
                    }
                }

                // Check if we are ready for the next step
                unsigned long lastStepMicros = _motionController.getAxisLastStepMicros(0);
                if (Utils::isTimeout(micros(), lastStepMicros, _timeBetweenHomingStepsUs))
                {
                    // Check which direction to rotate
                    bool rotateDirection = (_homingState == SEEK_FOR_LINEAR);

                    // Rotate
                    if (_homingState == ROTATE_TO_ENDSTOP || _homingState == SEEK_FOR_LINEAR)
                        _motionController.step(0, rotateDirection);
                    if (_homingState == ROTATE_TO_ENDSTOP || _homingState == SEEK_FOR_LINEAR)
                        _motionController.step(1, rotateDirection);
                }

                // Count homing steps in this stage
                _homingStepsDone++;

                // Check for max steps in this stage
                if (_homingStepsRequired != 0)
                {
                    if (_homingStepsDone >= _homingStepsRequired)
                    {
                        if (_homingState == SEEK_FOR_LINEAR)
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
