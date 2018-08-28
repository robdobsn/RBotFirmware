// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

#ifdef ESP32
#define USE_ESP32_TIMER_ISR 1
#endif

#include <ArduinoLog.h>
#include "MotionPipeline.h"
#include "MotionIO.h"
#include "TestMotionActuator.h"

class MotionActuator
{
  private:
    // If this is true nothing will move
    volatile bool _isPaused;

    // Pipeline of blocks to be processed
    MotionPipeline &_motionPipeline;

    // Raw access to motors and endstops
    RobotConsts::RawMotionHwInfo_t _rawMotionHwInfo;

    // This is to ensure that the robot never goes to 0 tick rate - which would leave it
    // immobile forever
    static constexpr uint32_t MIN_STEP_RATE_PER_SEC = 1;
    static constexpr uint32_t MIN_STEP_RATE_PER_TTICKS = uint32_t((MIN_STEP_RATE_PER_SEC * 1.0 * MotionBlock::TTICKS_VALUE) / MotionBlock::TICKS_PER_SEC);

#ifdef TEST_MOTION_ACTUATOR_ENABLE
    // Test code
    static TestMotionActuator *_pTestMotionActuator;
#endif

#ifdef USE_ESP32_TIMER_ISR
    // ISR based interval timer
    static hw_timer_t *_isrMotionTimer;
    static MotionActuator *_pISRMotAct;
    static constexpr uint32_t CLOCK_RATE_MHZ = 80;
    static constexpr uint32_t ISR_TIMER_PERIOD_US = uint32_t(MotionBlock::TICK_INTERVAL_NS / 1000l);
#endif

  private:
    // Execution info for the currently executing block
    bool _isEnabled;
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

  public:
    MotionActuator(MotionIO &motionIO, MotionPipeline &motionPipeline) : _motionPipeline(motionPipeline)
    {
        // Init
        clear();

        // If we are using the ISR then create the Spark Interval Timer and start it
#ifdef USE_ESP32_TIMER_ISR
        if (_pISRMotAct == NULL)
        {
            _pISRMotAct = this;
            _isrMotionTimer = timerBegin(0, CLOCK_RATE_MHZ, true);
            timerAttachInterrupt(_isrMotionTimer, _isrStepperMotion, true);
            timerAlarmWrite(_isrMotionTimer, ISR_TIMER_PERIOD_US, true);
            timerAlarmEnable(_isrMotionTimer);
            Log.notice("MotionActuator: Starting ISR timer\n");
        }
#endif
    }

    void setRawMotionHwInfo(RobotConsts::RawMotionHwInfo_t &rawMotionHwInfo)
    {
        _rawMotionHwInfo = rawMotionHwInfo;
    }

    void setTestMode(const char *testModeStr)
    {
#ifdef TEST_MOTION_ACTUATOR_ENABLE
        _pTestMotionActuator = new TestMotionActuator();
        _pTestMotionActuator->setTestMode(testModeStr);
#endif
    }

    void config()
    {
    }

    void clear()
    {
        _isPaused = true;
        _endStopReached = false;
        _lastDoneNumberedCmdIdx = RobotConsts::NUMBERED_COMMAND_NONE;
#ifdef TEST_MOTION_ACTUATOR_ENABLE
        _pTestMotionActuator = NULL;
#endif
    }

    void pause(bool pauseIt)
    {
        _isPaused = pauseIt;
        if (!_isPaused)
        {
            _endStopReached = false;
        }
    }

    void clearEndstopReached()
    {
        _endStopReached = false;
    }

    bool isEndStopReached()
    {
        return _endStopReached;
    }

    int getLastCompletedNumberedCmdIdx()
    {
        return _lastDoneNumberedCmdIdx;
    }
    void process();

    String getDebugStr();
    void showDebug();

  private:
#ifdef USE_ESP32_TIMER_ISR
    static void _isrStepperMotion(void);
#endif
    void procTick();
};
