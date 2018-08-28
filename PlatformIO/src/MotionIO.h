// RBotFirmware
// Rob Dobson 2016-18

#pragma once

#include <Time.h>
#include <ESP32Servo.h>
#include "StepperMotor.h"
#include "EndStop.h"
#include "Utils.h"

#ifndef SPARK
//#define BOUNDS_CHECK_ISR_FUNCTIONS    1
#endif

class MotionIO
{
  public:
    static constexpr float stepDisableSecs_default = 60.0f;

  private:
    // Stepper motors
    StepperMotor *_stepperMotors[RobotConsts::MAX_AXES];
    // Servo motors
    Servo *_servoMotors[RobotConsts::MAX_AXES];
    // Step enable
    int _stepEnablePin;
    bool _stepEnLev = true;
    // Motor enable
    float _stepDisableSecs;
    bool _motorsAreEnabled;
    unsigned long _motorEnLastMillis;
    time_t _motorEnLastUnixTime;
    // End stops
    EndStop *_endStops[RobotConsts::MAX_AXES][RobotConsts::MAX_ENDSTOPS_PER_AXIS];

  public:
    MotionIO()
    {
        // Clear axis specific values
        for (int i = 0; i < RobotConsts::MAX_AXES; i++)
        {
            _stepperMotors[i] = NULL;
            _servoMotors[i] = NULL;
            for (int j = 0; j < RobotConsts::MAX_ENDSTOPS_PER_AXIS; j++)
                _endStops[i][j] = NULL;
        }
        // Stepper management
        _stepEnablePin = -1;
        _stepEnLev = true;
        _stepDisableSecs = 60.0;
        _motorEnLastMillis = 0;
        _motorEnLastUnixTime = 0;
    }

    ~MotionIO()
    {
        deinit();
    }

    void deinit()
    {
        // disable
        if (_stepEnablePin != -1)
            pinMode(_stepEnablePin, INPUT);

        // remove motors and end stops
        for (int i = 0; i < RobotConsts::MAX_AXES; i++)
        {
            delete _stepperMotors[i];
            _stepperMotors[i] = NULL;
            if (_servoMotors[i])
                _servoMotors[i]->detach();
            delete _servoMotors[i];
            _servoMotors[i] = NULL;
            for (int j = 0; j < RobotConsts::MAX_ENDSTOPS_PER_AXIS; j++)
            {
                delete _endStops[i][j];
                _endStops[i][j] = NULL;
            }
        }
    }

    bool configureAxis(const char *axisJSON, int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return false;

        // Check the kind of motor to use
        bool isValid = false;
        String stepPinName = RdJson::getString("stepPin", "-1", axisJSON, isValid);
        if (isValid)
        {
            // Create the stepper motor for the axis
            String dirnPinName = RdJson::getString("dirnPin", "-1", axisJSON);
            int stepPin = ConfigPinMap::getPinFromName(stepPinName.c_str());
            int dirnPin = ConfigPinMap::getPinFromName(dirnPinName.c_str());
            Log.notice("Axis%d (step pin %d, dirn pin %d)\n", axisIdx, stepPin, dirnPin);
            if ((stepPin != -1 && dirnPin != -1))
                _stepperMotors[axisIdx] = new StepperMotor(RobotConsts::MOTOR_TYPE_DRIVER, stepPin, dirnPin);
        }
        else
        {
            // Create a servo motor for the axis
            String servoPinName = RdJson::getString("servoPin", "-1", axisJSON);
            long servoPin = ConfigPinMap::getPinFromName(servoPinName.c_str());
            Log.notice("Axis%d (servo pin %ld)\n", axisIdx, servoPin);
            if ((servoPin != -1))
            {
                _servoMotors[axisIdx] = new Servo();
                if (_servoMotors[axisIdx])
                    _servoMotors[axisIdx]->attach(servoPin);
            }
        }

        // End stops
        for (int endStopIdx = 0; endStopIdx < RobotConsts::MAX_ENDSTOPS_PER_AXIS; endStopIdx++)
        {
            // Get the config for endstop if present
            String endStopIdStr = "endStop" + String(endStopIdx);
            String endStopJSON = RdJson::getString(endStopIdStr.c_str(), "{}", axisJSON);
            if (endStopJSON.length() == 0 || endStopJSON.equals("{}"))
                continue;

            // Create endStop from JSON
            _endStops[axisIdx][endStopIdx] = new EndStop(axisIdx, endStopIdx, endStopJSON.c_str());
        }

        return true;
    }

    bool configureMotors(const char *robotConfigJSON)
    {
        // Get motor enable info
        String stepEnablePinName = RdJson::getString("stepEnablePin", "-1", robotConfigJSON);
        _stepEnLev = RdJson::getLong("stepEnLev", 1, robotConfigJSON);
        _stepEnablePin = ConfigPinMap::getPinFromName(stepEnablePinName.c_str());
        _stepDisableSecs = float(RdJson::getDouble("stepDisableSecs", stepDisableSecs_default, robotConfigJSON));
        Log.notice("MotorIO (pin %d, actLvl %d, disableAfter %Fs)\n", _stepEnablePin, _stepEnLev, _stepDisableSecs);

        // Enable pin - initially disable
        pinMode(_stepEnablePin, OUTPUT);
        digitalWrite(_stepEnablePin, !_stepEnLev);
        return true;
    }

    // Set step direction
    void stepDirn(int axisIdx, bool dirn)
    {
#ifdef BOUNDS_CHECK_ISR_FUNCTIONS
        _ASSERT(axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES);
#endif
        // Start dirn
        if (_stepperMotors[axisIdx])
            _stepperMotors[axisIdx]->stepDirn(dirn);
    }

    // Start a step
    void stepStart(int axisIdx)
    {
#ifdef BOUNDS_CHECK_ISR_FUNCTIONS
        _ASSERT(axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES);
#endif
        // Start step
        if (_stepperMotors[axisIdx])
            return _stepperMotors[axisIdx]->stepStart();
    }

    // Check if a step is in progress on any motor, if all such and return true, else false
    bool stepEnd()
    {
        // Check if step in progress
        bool aStepEnded = false;
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            if (_stepperMotors[axisIdx])
                aStepEnded = aStepEnded || _stepperMotors[axisIdx]->stepEnd();
        }
        return aStepEnded;
    }

    void stepSynch(int axisIdx, bool direction)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return;

        enableMotors(true, false);
        if (_stepperMotors[axisIdx])
        {
            _stepperMotors[axisIdx]->stepSync(direction);
        }
        //_axisParams[axisIdx]._stepsFromHome += (direction ? 1 : -1);
        //_axisParams[axisIdx]._lastStepMicros = micros();
    }

    void jump(int axisIdx, long targetPosition)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return;

        if (_servoMotors[axisIdx])
            _servoMotors[axisIdx]->writeMicroseconds(targetPosition);
    }

    // Endstops
    bool isEndStopValid(int axisIdx, int endStopIdx)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return false;
        if (endStopIdx < 0 || endStopIdx >= RobotConsts::MAX_ENDSTOPS_PER_AXIS)
            return false;
        return true;
    }

    bool isAtEndStop(int axisIdx, int endStopIdx)
    {
        // For safety return true in these cases
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return true;
        if (endStopIdx < 0 || endStopIdx >= RobotConsts::MAX_ENDSTOPS_PER_AXIS)
            return true;

        // Test endstop
        if (_endStops[axisIdx][endStopIdx])
            return _endStops[axisIdx][endStopIdx]->isAtEndStop();

        // All other cases return true (as this might be safer)
        return true;
    }

    AxisMinMaxBools getEndStopVals()
    {
        AxisMinMaxBools endstops;
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            for (int endStopIdx = 0; endStopIdx < RobotConsts::MAX_ENDSTOPS_PER_AXIS; endStopIdx++)
            {
                AxisMinMaxBools::AxisMinMaxEnum endStopEnum = AxisMinMaxBools::END_STOP_NONE;
                if (_endStops[axisIdx][endStopIdx])
                {
                    if (_endStops[axisIdx][endStopIdx]->isAtEndStop())
                        endStopEnum = AxisMinMaxBools::END_STOP_HIT;
                    else
                        endStopEnum = AxisMinMaxBools::END_STOP_NOT_HIT;
                }
                endstops.set(axisIdx, endStopIdx, endStopEnum);
            }
        }
        return endstops;
    }

    void enableMotors(bool en, bool timeout)
    {
        // Log.trace("Enable %d, disable level %d, disable after time %F\n",
        //							en, !_stepEnLev, _stepDisableSecs);
        if (en)
        {
            if (_stepEnablePin != -1)
            {
                if (!_motorsAreEnabled)
                    Log.notice("MotionIO: motors enabled, disable after time %F\n", _stepDisableSecs);
                digitalWrite(_stepEnablePin, _stepEnLev);
            }
            _motorsAreEnabled = true;
            _motorEnLastMillis = millis();
            time(&_motorEnLastUnixTime);
        }
        else
        {
            if (_stepEnablePin != -1)
            {
                if (_motorsAreEnabled)
                    Log.notice("MotionIO: motors disabled by %s\n", timeout ? "timeout" : "command");
                digitalWrite(_stepEnablePin, !_stepEnLev);
            }
            _motorsAreEnabled = false;
        }
    }

    unsigned long getLastActiveUnixTime()
    {
        return _motorEnLastUnixTime;
    }

    void motionIsActive()
    {
        enableMotors(true, false);
    }

    void service()
    {
        // Check for motor enable timeout
        if (_motorsAreEnabled && Utils::isTimeout(millis(), _motorEnLastMillis,
                                                  (unsigned long)(_stepDisableSecs * 1000)))
            enableMotors(false, true);
    }

    void getRawMotionHwInfo(RobotConsts::RawMotionHwInfo_t &raw)
    {
        // Fill in the info
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            // Initialise
            raw._axis[axisIdx]._pinStepCurLevel = 0;
            raw._axis[axisIdx]._motorType = RobotConsts::MOTOR_TYPE_NONE;
            raw._axis[axisIdx]._pinStep = -1;
            raw._axis[axisIdx]._pinDirection = -1;
            raw._axis[axisIdx]._pinDirectionReversed = 0;
            raw._axis[axisIdx]._pinEndStopMin = -1;
            raw._axis[axisIdx]._pinEndStopMinactLvl = 0;
            raw._axis[axisIdx]._pinEndStopMax = -1;
            raw._axis[axisIdx]._pinEndStopMaxactLvl = 0;

            // Extract info about stepper motor if any
            if (_stepperMotors[axisIdx])
            {
                raw._axis[axisIdx]._motorType = _stepperMotors[axisIdx]->getMotorType();
                _stepperMotors[axisIdx]->getPins(raw._axis[axisIdx]._pinStep,
                                                 raw._axis[axisIdx]._pinDirection,
                                                 raw._axis[axisIdx]._pinDirectionReversed);
            }
            // Min endstop
            if (_endStops[axisIdx][0])
            {
                _endStops[axisIdx][0]->getPins(raw._axis[axisIdx]._pinEndStopMin,
                                               raw._axis[axisIdx]._pinEndStopMinactLvl);
            }
            // Max endstop
            if (_endStops[axisIdx][1])
            {
                _endStops[axisIdx][1]->getPins(raw._axis[axisIdx]._pinEndStopMax,
                                               raw._axis[axisIdx]._pinEndStopMaxactLvl);
            }
        }
    }
};
