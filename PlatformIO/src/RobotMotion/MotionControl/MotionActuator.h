// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

#ifdef ESP32
#define USE_ESP32_TIMER_ISR 1
#endif

#include <ArduinoLog.h>
#include "MotionInstrumentation.h"
#include "MotionBlock.h"
#include "MotionIO.h"

class MotionPipeline;

class MotionActuator
{
private:
    // If this is true nothing will move
    static volatile bool _isPaused;

    // Steps moved in total and increment based on direction
    static volatile int32_t _totalStepsMoved[RobotConsts::MAX_AXES];
    static volatile int32_t _totalStepsInc[RobotConsts::MAX_AXES];

    // Pipeline of blocks to be processed
    static MotionPipeline* _pMotionPipeline;

    // Access to motors and endstops
    static MotionIO* _pMotionIO;

    // Raw access to motors and endstops
    static RobotConsts::RawMotionHwInfo_t _rawMotionHwInfo;

    // This is to ensure that the robot never goes to 0 tick rate - which would leave it
    // immobile forever
    static constexpr uint32_t MIN_STEP_RATE_PER_SEC = 10;
    static constexpr uint32_t MIN_STEP_RATE_PER_TTICKS = uint32_t((MIN_STEP_RATE_PER_SEC * 1.0 * MotionBlock::TTICKS_VALUE) / MotionBlock::TICKS_PER_SEC);

#ifdef INSTRUMENT_MOTION_ACTUATOR_ENABLE
    // Test code
    static MotionInstrumentation *_pMotionInstrumentation;
#endif

#ifdef USE_ESP32_TIMER_ISR
    // ISR based interval timer
    static hw_timer_t *_isrMotionTimer;
    static constexpr uint32_t CLOCK_RATE_MHZ = 80;
    static constexpr uint32_t DIRECT_STEP_ISR_TIMER_PERIOD_US = uint32_t(MotionBlock::TICK_INTERVAL_NS / 1000l);
#endif
    static bool _isrTimerStarted;

private:
    // Execution info for the currently executing block
    static bool _isEnabled;
    // End-stop reached
    static bool _endStopReached;
    // Last completed numbered command
    static int _lastDoneNumberedCmdIdx;
    // Steps
    static uint32_t _stepsTotalAbs[RobotConsts::MAX_AXES];
    static uint32_t _curStepCount[RobotConsts::MAX_AXES];
    // Current step rate (in steps per K ticks)
    static uint32_t _curStepRatePerTTicks;
    // Accumulators for stepping and acceleration increments
    static uint32_t _curAccumulatorStep;
    static uint32_t _curAccumulatorNS;
    static uint32_t _curAccumulatorRelative[RobotConsts::MAX_AXES];

    static int _endStopCheckNum;
    struct EndStopChecks
    {
        int pin;
        bool val;
    };
    static EndStopChecks _endStopChecks[RobotConsts::MAX_AXES];

public:
    MotionActuator(MotionIO* pMotionIO, MotionPipeline* pMotionPipeline);
    static void setRawMotionHwInfo(RobotConsts::RawMotionHwInfo_t &rawMotionHwInfo);
    static void setInstrumentationMode(const char *testModeStr);
    static void deinit();
    static void configure();
    static void stop();
    static void clear();
    static void pause(bool pauseIt);
    static void resetTotalStepPosition();
    static void getTotalStepPosition(AxisInt32s& actuatorPos);
    static void setTotalStepPosition(int axisIdx, int32_t stepPos);
    static void clearEndstopReached();
    static bool isEndStopReached();
    static int getLastCompletedNumberedCmdIdx();
    static void process();
    static String getDebugStr();
    static void showDebug();

private:
    static void _isrStepperMotion(void);
    static bool handleStepEnd();
    static void setupNewBlock(MotionBlock *pBlock);
    static void updateMSAccumulator(MotionBlock *pBlock);
    static bool handleStepMotion(MotionBlock *pBlock);
    static void endMotion(MotionBlock *pBlock);
};
