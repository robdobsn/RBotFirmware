// RBotFirmware
// Rob Dobson 2016-18

#include "RampGenIO.h"
#include <ESP32Servo.h>
#include "AxisValues.h"
#include "StepperMotor.h"
#include "EndStop.h"
#include "Utils.h"

static const char* MODULE_PREFIX = "RampGenIO: ";

RampGenIO::RampGenIO()
{
    // Clear axis specific values
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
        _stepperMotors[i] = NULL;
        _servoMotors[i] = NULL;
        for (int j = 0; j < RobotConsts::MAX_ENDSTOPS_PER_AXIS; j++)
            _endStops[i][j] = NULL;
    }
}

RampGenIO::~RampGenIO()
{
    deinit();
}

void RampGenIO::deinit()
{
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

bool RampGenIO::configureAxis(int axisIdx, const char *axisJSON)
{
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
        return false;

    // Check the kind of motor to use
    bool isValid = false;
    String stepPinName = RdJson::getString("stepPin", "-1", axisJSON, isValid);
    if (isValid)
    {
        // Create the stepper motor for the axis
        int stepPin = ConfigPinMap::getPinFromName(stepPinName.c_str());
        String dirnPinName = RdJson::getString("dirnPin", "-1", axisJSON);
        int dirnPin = ConfigPinMap::getPinFromName(dirnPinName.c_str());
        int muxPin1 = -1, muxPin2 = -1, muxPin3 = -1, muxDirnIdx = 0;
        if (dirnPin == -1)
        {
            // Check for multiplexed pins
            String muxName = RdJson::getString("muxPin1", "-1", axisJSON);
            muxPin1 = ConfigPinMap::getPinFromName(muxName.c_str());
            muxName = RdJson::getString("muxPin2", "-1", axisJSON);
            muxPin2 = ConfigPinMap::getPinFromName(muxName.c_str());
            muxName = RdJson::getString("muxPin3", "-1", axisJSON);
            muxPin3 = ConfigPinMap::getPinFromName(muxName.c_str());
            muxName = RdJson::getString("muxDirnIdx", "-1", axisJSON);
            muxDirnIdx = ConfigPinMap::getPinFromName(muxName.c_str());
        }
        bool directionReversed = (RdJson::getLong("dirnRev", 0, axisJSON) != 0);

        // Debug
        if (dirnPin >= 0)
            Log.notice("%sAxis%d (step pin %d, dirn pin %d)\n", MODULE_PREFIX, axisIdx, stepPin, dirnPin);
        else
            Log.notice("%sAxis%d (step pin %d, dirn pin %d, mux1 %d, mux2 %d, mux3 %d, muxDirnIdx %d)\n", 
                        MODULE_PREFIX, axisIdx, stepPin, dirnPin, muxPin1, muxPin2, muxPin3, muxDirnIdx);

        // Setup stepper
        if ((stepPin >= 0) && ((dirnPin >= 0) || (muxPin1 >= 0)))
            _stepperMotors[axisIdx] = new StepperMotor(RobotConsts::MOTOR_TYPE_DRIVER, stepPin, dirnPin, 
                                muxPin1, muxPin2, muxPin3, muxDirnIdx, directionReversed);
    }
    else
    {
        // Create a servo motor for the axis
        String servoPinName = RdJson::getString("servoPin", "-1", axisJSON);
        long servoPin = ConfigPinMap::getPinFromName(servoPinName.c_str());
        Log.notice("%sAxis%d (servo pin %d)\n", MODULE_PREFIX, axisIdx, servoPin);
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

// // Set step direction
// void RampGenIO::stepDirn(int axisIdx, bool dirn)
// {
// #ifdef BOUNDS_CHECK_ISR_FUNCTIONS
//     _ASSERT(axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES);
// #endif
//     // Start dirn
//     if (_stepperMotors[axisIdx])
//         _stepperMotors[axisIdx]->stepDirn(dirn);
// }

// // Start a step
// void RampGenIO::stepStart(int axisIdx)
// {
// #ifdef BOUNDS_CHECK_ISR_FUNCTIONS
//     _ASSERT(axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES);
// #endif
//     // Start step
//     if (_stepperMotors[axisIdx])
//         return _stepperMotors[axisIdx]->stepStart();
// }

// // Check if a step is in progress on any motor, if all such and return true, else false
// bool RampGenIO::stepEnd()
// {
//     // Check if step in progress
//     bool aStepEnded = false;
//     for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
//     {
//         if (_stepperMotors[axisIdx])
//             aStepEnded = aStepEnded || _stepperMotors[axisIdx]->stepEnd();
//     }
//     return aStepEnded;
// }

// void RampGenIO::stepSynch(int axisIdx, bool direction)
// {
//     if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
//         return;

//     enableMotors(true, false);
//     if (_stepperMotors[axisIdx])
//     {
//         _stepperMotors[axisIdx]->stepSync(direction);
//     }
//     //_axisParams[axisIdx]._stepsFromHome += (direction ? 1 : -1);
//     //_axisParams[axisIdx]._lastStepMicros = micros();
// }

// void RampGenIO::jump(int axisIdx, long targetPosition)
// {
//     if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
//         return;

//     if (_servoMotors[axisIdx])
//         _servoMotors[axisIdx]->writeMicroseconds(targetPosition);
// }

// // Endstops
// bool RampGenIO::isEndStopValid(int axisIdx, int endStopIdx)
// {
//     if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
//         return false;
//     if (endStopIdx < 0 || endStopIdx >= RobotConsts::MAX_ENDSTOPS_PER_AXIS)
//         return false;
//     return true;
// }

// bool RampGenIO::isAtEndStop(int axisIdx, int endStopIdx)
// {
//     // For safety return true in these cases
//     if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
//         return true;
//     if (endStopIdx < 0 || endStopIdx >= RobotConsts::MAX_ENDSTOPS_PER_AXIS)
//         return true;

//     // Test endstop
//     if (_endStops[axisIdx][endStopIdx])
//         return _endStops[axisIdx][endStopIdx]->isAtEndStop();

//     // All other cases return true (as this might be safer)
//     return true;
// }

void RampGenIO::getEndStopStatus(AxisMinMaxBools& axisEndStopVals)
{
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
            axisEndStopVals.set(axisIdx, endStopIdx, endStopEnum);
        }
    }
}

void RampGenIO::service()
{
}

void RampGenIO::getRawMotionHwInfo(RobotConsts::RawMotionHwInfo_t &raw)
{
    // Fill in the info
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        // Initialise
        raw._axis[axisIdx]._motorType = RobotConsts::MOTOR_TYPE_NONE;
        raw._axis[axisIdx]._pinEndStopMin = -1;
        raw._axis[axisIdx]._pinEndStopMinactLvl = 0;
        raw._axis[axisIdx]._pinEndStopMax = -1;
        raw._axis[axisIdx]._pinEndStopMaxactLvl = 0;

        // Extract info about stepper motor if any
        if (_stepperMotors[axisIdx])
        {
            raw._axis[axisIdx]._motorType = _stepperMotors[axisIdx]->getMotorType();
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

// Set axis direction
void IRAM_ATTR RampGenIO::setDirection(int axisIdx, bool direction)
{
    StepperMotor* pStepper = _stepperMotors[axisIdx];
    if (pStepper)
        pStepper->setDirection(direction);
}

void IRAM_ATTR RampGenIO::stepStart(int axisIdx)
{
    StepperMotor* pStepper = _stepperMotors[axisIdx];
    if (pStepper)
        pStepper->stepStart();
}

bool IRAM_ATTR RampGenIO::stepEnd(int axisIdx)
{
    StepperMotor* pStepper = _stepperMotors[axisIdx];
    if (pStepper)
        return pStepper->stepEnd();
    return false;
}