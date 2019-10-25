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

class RampGenIO
{
private:
    // Stepper motors
    StepperMotor* _stepperMotors[RobotConsts::MAX_AXES];
    // Servo motors
    Servo* _servoMotors[RobotConsts::MAX_AXES];
    // End stops
    EndStop* _endStops[RobotConsts::MAX_AXES][RobotConsts::MAX_ENDSTOPS_PER_AXIS];

public:
    RampGenIO();
    ~RampGenIO();

    // Service
    void service();

    // Motors
    void enableMotors(bool en, bool timeout);

    // // Set step direction
    // void stepDirn(int axisIdx, bool dirn);

    // // Start a step
    // void stepStart(int axisIdx);    

    // Deinitialise
    void deinit();

    // Raw access
    void getRawMotionHwInfo(RobotConsts::RawMotionHwInfo_t &raw);

    // Configure
    bool configureAxis(int axisIdx, const char *axisJSON);

    // Endstop status
    void getEndStopStatus(AxisMinMaxBools& axisEndStopVals);

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
