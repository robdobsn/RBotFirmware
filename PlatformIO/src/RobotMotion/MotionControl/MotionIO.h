// RBotFirmware
// Rob Dobson 2016-18

#pragma once

#include <time.h>
#include "RobotConsts.h"

#ifndef SPARK
//#define BOUNDS_CHECK_ISR_FUNCTIONS    1
#endif

class StepperMotor;
class EndStop;
class Servo;
class AxisMinMaxBools;

class MotionIO
{
public:
    static constexpr float stepDisableSecs_default = 60.0f;

private:
    // Stepper motors
    StepperMotor* _stepperMotors[RobotConsts::MAX_AXES];
    // Servo motors
    Servo* _servoMotors[RobotConsts::MAX_AXES];
    // Step enable
    int _stepEnablePin;
    bool _stepEnLev = true;
    // Motor enable
    float _stepDisableSecs;
    bool _motorsAreEnabled;
    unsigned long _motorEnLastMillis;
    time_t _motorEnLastUnixTime;
    // End stops
    EndStop* _endStops[RobotConsts::MAX_AXES][RobotConsts::MAX_ENDSTOPS_PER_AXIS];

public:
    MotionIO();
    ~MotionIO();

    // Service
    void service();

    // Motors
    void enableMotors(bool en, bool timeout);

    // // Set step direction
    // void stepDirn(int axisIdx, bool dirn);

    // // Start a step
    // void stepStart(int axisIdx);    

    // Activity
    unsigned long getLastActiveUnixTime();
    void motionIsActive();

    // Deinitialise
    void deinit();

    // Raw access
    void getRawMotionHwInfo(RobotConsts::RawMotionHwInfo_t &raw);

    // Configure
    bool configureMotors(const char *robotConfigJSON);
    bool configureAxis(const char *axisJSON, int axisIdx);

    // Endstop info
    void getEndStopVals(AxisMinMaxBools& axisEndStopVals);

    // Motor control
    void setDirection(int axisIdx, bool direction);
    void stepStart(int axisIdx);
    bool stepEnd(int axisIdx);

// private:

//     // Check if a step is in progress on any motor, if all such and return true, else false
//     bool stepEnd();

//     // Step (blocking)
//     void stepSynch(int axisIdx, bool direction);

//     // Jump (servo)
//     void jump(int axisIdx, long targetPosition);

//     // Endstops
//     bool isEndStopValid(int axisIdx, int endStopIdx);
//     bool isAtEndStop(int axisIdx, int endStopIdx);
};
