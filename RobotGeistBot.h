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
        HOMING_STATE_WAIT,
    };
    HOMING_STATE _homingState;
    unsigned long _homeReqTime;

public:
    RobotGeistBot(const char* pRobotTypeName) :
        RobotPolar(pRobotTypeName)
    {
        _homingState = HOMING_STATE_IDLE;
        _homeReqTime = 0;
    }

    void home(RobotCommandArgs& args)
    {
        // GeistBot can only home X & Y axes together so ignore params
        // Info
        Log.info("%s home x%d, y%d, z%d", _robotTypeName.c_str(), args.xValid, args.yValid, args.zValid);

        // Set homing state
        _homingState = HOMING_STATE_INIT;
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
                _homingState = HOMING_STATE_WAIT;
                break;
            }
            case HOMING_STATE_WAIT:
            {
                if (millis() > _homeReqTime + 15000)
                {
                    _homingState = HOMING_STATE_IDLE;
                    Log.info("Done Homing");
                }
                break;
            }
        }
    }
};
