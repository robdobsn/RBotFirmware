// RBotFirmware
// Rob Dobson 2016

#pragma once

#include <Arduino.h>
#include "RobotConsts.h"

class StepperMotor
{
  private:
    // Motor type
    RobotConsts::MOTOR_TYPE _motorType;

    // Motor operates in reverse (i.e. direction low = forwards)
    bool _motorDirectionReversed;

    // Minimum width of stepping pulse
    int _minPulseWidthUs;

    // Pins for the motor
    int _pinStep;
    int _pinDirectionSingle;

    // Mulitplexer value instead of direction pin
    int _pinDirectionMux1, _pinDirectionMux2, _pinDirectionMux3;
    int _muxDirectionIdx;

    // Current step level and direction
    bool _stepCurActive;
    bool _curDirVal;

  public:
    // For MOTOR_TYPE_DRIVER two pins are used step & direction
    StepperMotor(RobotConsts::MOTOR_TYPE motorType, int pinStep, int pinDirectionSingle, 
                int pinDirectionMux1, int pinDirectionMux2, int pinDirectionMux3, 
                int muxDirectionIdx, bool directionReversed)
    {
        if (motorType == RobotConsts::MOTOR_TYPE_DRIVER)
        {
            if (pinStep != -1)
            {
                _motorType = motorType;
                _motorDirectionReversed = directionReversed;
                _minPulseWidthUs = 1;
                // Setup the pins
                pinMode(pinStep, OUTPUT);
                digitalWrite(pinStep, false);
                _pinStep = pinStep;
                _pinDirectionSingle = pinDirectionSingle;
                if (_pinDirectionSingle >= 0)
                    pinMode(_pinDirectionSingle, OUTPUT);
                _pinDirectionMux1 = pinDirectionMux1;
                _pinDirectionMux2 = pinDirectionMux2;
                _pinDirectionMux3 = pinDirectionMux3;
                _muxDirectionIdx = muxDirectionIdx;
                if (_pinDirectionMux1 >= 0)
                    pinMode(_pinDirectionMux1, OUTPUT);
                if (_pinDirectionMux2 >= 0)
                    pinMode(_pinDirectionMux2, OUTPUT);
                if (_pinDirectionMux3 >= 0)
                    pinMode(_pinDirectionMux3, OUTPUT);
                _stepCurActive = false;
                _curDirVal = false;
            }
        }
        else
        {
            motorType = RobotConsts::MOTOR_TYPE_NONE;
        }
    }

    ~StepperMotor()
    {
        deinit();
    }

    void deinit()
    {
        _motorType = RobotConsts::MOTOR_TYPE_NONE;
        // Free up pins
        if (_pinStep >= 0)
            pinMode(_pinStep, INPUT);
        if (_pinDirectionSingle >= 0)
            pinMode(_pinDirectionSingle, INPUT);
        if (_pinDirectionMux1 >= 0)
            pinMode(_pinDirectionMux1, INPUT);
        if (_pinDirectionMux2 >= 0)
            pinMode(_pinDirectionMux2, INPUT);
        if (_pinDirectionMux3 >= 0)
            pinMode(_pinDirectionMux3, INPUT);
    }

    // Set direction
    void IRAM_ATTR setDirection(bool dirn)
    {
        bool dirnVal = _motorDirectionReversed ? dirn : !dirn;
        if (_pinDirectionSingle >= 0)
        {
            digitalWrite(_pinDirectionSingle, dirnVal);
        }
        else 
        {
            // Record for when the actual step takes place
            _curDirVal = dirnVal;
        }
    }

    bool IRAM_ATTR stepEnd()
    {
        if (_stepCurActive)
        {
            _stepCurActive = false;
            digitalWrite(_pinStep, false);
            return true;
        }
        return false;
    }

    void IRAM_ATTR stepStart()
    {
        if (_pinStep >= 0)
        {
            if (_pinDirectionSingle < 0)
            {
                if (_pinDirectionMux1 >= 0)
                    digitalWrite(_pinDirectionMux1, _curDirVal ? 1 : ((_muxDirectionIdx & 0x01) != 0));
                if (_pinDirectionMux2 >= 0)
                    digitalWrite(_pinDirectionMux2, _curDirVal ? 1 : ((_muxDirectionIdx & 0x02) != 0));
                if (_pinDirectionMux3 >= 0)
                    digitalWrite(_pinDirectionMux3, _curDirVal ? 1 : ((_muxDirectionIdx & 0x04) != 0));
            }

            digitalWrite(_pinStep, true);
            _stepCurActive = true;
        }

    }

    // void stepSync(bool direction)
    // {
    //     if ((_pinDirection < 0) || (_pinStep < 0))
    //         return;

    //     // Set direction
    //     digitalWrite(_pinDirection, _motorDirectionReversed ? direction : !direction);

    //     // Step
    //     digitalWrite(_pinStep, true);
    //     delayMicroseconds(_minPulseWidthUs);
    //     digitalWrite(_pinStep, false);
    // }

    // void getPins(int &stepPin, int &dirnPin, bool &dirnReverse)
    // {
    //     stepPin = _pinStep;
    //     dirnPin = _pinDirection;
    //     dirnReverse = _motorDirectionReversed;
    // }

    RobotConsts::MOTOR_TYPE getMotorType()
    {
        return _motorType;
    }
};
