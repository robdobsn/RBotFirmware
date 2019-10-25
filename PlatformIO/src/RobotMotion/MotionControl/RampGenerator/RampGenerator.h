// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

#ifdef ESP32
#define USE_ESP32_TIMER_ISR 1
#endif

#include <ArduinoLog.h>
#include "MotionInstrumentation.h"
#include "../MotionBlock.h"
#include "RampGenIO.h"

class MotionPipeline;

class RampGenerator
{
private:
    // This singleton
    static RampGenerator* _pThis;

    // If this is true nothing will move
    volatile bool _isPaused;

    // Steps moved in total and increment based on direction
    volatile int32_t _axisTotalSteps[RobotConsts::MAX_AXES];
    volatile int32_t _totalStepsInc[RobotConsts::MAX_AXES];

    // Pipeline of blocks to be processed
    MotionPipeline* _pMotionPipeline;

    // Motors and endstops
    RampGenIO _rampGenIO;

    // Raw access to motors and endstops
    RobotConsts::RawMotionHwInfo_t _rawMotionHwInfo;

    // This is to ensure that the robot never goes to 0 tick rate - which would leave it
    // immobile forever
    static constexpr uint32_t MIN_STEP_RATE_PER_SEC = 10;
    static constexpr uint32_t MIN_STEP_RATE_PER_TTICKS = uint32_t((MIN_STEP_RATE_PER_SEC * 1.0 * MotionBlock::TTICKS_VALUE) / MotionBlock::TICKS_PER_SEC);

#ifdef INSTRUMENT_MOTION_ACTUATOR_ENABLE
    // Test code
    MotionInstrumentation *_pMotionInstrumentation;
#endif

#ifdef USE_ESP32_TIMER_ISR
    // ISR based interval timer
    hw_timer_t *_isrMotionTimer;
    static constexpr uint32_t CLOCK_RATE_MHZ = 80;
    static constexpr uint32_t DIRECT_STEP_ISR_TIMER_PERIOD_US = uint32_t(MotionBlock::TICK_INTERVAL_NS / 1000l);
#endif
    bool _isrTimerStarted;

private:
    // Execution info for the currently executing block
    bool _isEnabled;
    // Ramp generation enabled
    bool _rampGenEnabled;
    // End-stop reached
    bool _endStopReached;
    // Last completed numbered command
    int _lastDoneNumberedCmdIdx;
    // Steps
    uint32_t _stepsTotalAbs[RobotConsts::MAX_AXES];
    uint32_t _curStepCount[RobotConsts::MAX_AXES];
    // Current step rate (in steps per K ticks)
    uint32_t _curStepRatePerTTicks;
    // Accumulators for stepping and acceleration increments
    uint32_t _curAccumulatorStep;
    uint32_t _curAccumulatorNS;
    uint32_t _curAccumulatorRelative[RobotConsts::MAX_AXES];

    int _endStopCheckNum;
    struct EndStopChecks
    {
        int pin;
        bool val;
    };
    EndStopChecks _endStopChecks[RobotConsts::MAX_AXES];

public:
    RampGenerator(MotionPipeline* pMotionPipeline);
    // static void setRawMotionHwInfo(RobotConsts::RawMotionHwInfo_t &rawMotionHwInfo);
    void setInstrumentationMode(const char *testModeStr);
    void deinit();
    void configure(bool rampGenEnabled);
    bool configureAxis(int axisIdx, const char *axisJSON)
    {
        return _rampGenIO.configureAxis(axisIdx, axisJSON);
    }
    void stop();
    // static void clear();
    void pause(bool pauseIt);
    void resetTotalStepPosition();
    void getTotalStepPosition(AxisInt32s& actuatorPos);
    void setTotalStepPosition(int axisIdx, int32_t stepPos);
    void clearEndstopReached();
    void getEndStopStatus(AxisMinMaxBools& axisEndStopVals)
    {
        _rampGenIO.getEndStopStatus(axisEndStopVals);
    }
    bool isEndStopReached();
    int getLastCompletedNumberedCmdIdx();
    void process();
    String getDebugStr();
    void showDebug();

private:
    static void _staticISRStepperMotion();
    void isrStepperMotion();
    bool handleStepEnd();
    void setupNewBlock(MotionBlock *pBlock);
    void updateMSAccumulator(MotionBlock *pBlock);
    bool handleStepMotion(MotionBlock *pBlock);
    void endMotion(MotionBlock *pBlock);
};
