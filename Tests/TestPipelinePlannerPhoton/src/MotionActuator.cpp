// RBotFirmware
// Rob Dobson 2018

#include "MotionActuator.h"

#ifdef USE_SPARK_INTERVAL_TIMER_ISR

// Static interval timer
IntervalTimer MotionActuator::_isrMotionTimer;

// Static refrerence to a single MotionActuator instance
MotionActuator* MotionActuator::_pMotionActuatorInstance = NULL;

#ifdef DEBUG_BLINK_D7_ON_ISR
uint32_t MotionActuator::_testCount = 0;
#endif

#ifdef DEBUG_TIME_ISR_OVERALL
uint32_t MotionActuator::__isrDbgTickMin = 100000000;
uint32_t MotionActuator::__isrDbgTickMax = 0;
#endif

void MotionActuator::_isrStepperMotion(void)
{
#ifdef DEBUG_TIME_ISR_OVERALL
    __disable_irq();
    uint32_t startTicks = System.ticks();
#endif
#ifdef DEBUG_BLINK_D7_ON_ISR
    int blinkRate = 10000;
    if (_pMotionActuatorInstance != NULL)
        blinkRate = 2500;
	_testCount++;
	if (_testCount > blinkRate)
	{
		_testCount = 0;
		digitalWrite(D7, !digitalRead(D7));
	}
#endif

    // Process block
    if (_pMotionActuatorInstance)
    {
        _pMotionActuatorInstance->procTick();
    }

#ifdef DEBUG_TIME_ISR_OVERALL
    // Time the ISR over entire execution
    uint32_t endTicks = System.ticks();
    __enable_irq();
    uint32_t elapsedTicks = endTicks - startTicks;
    if (startTicks < endTicks)
        elapsedTicks = 0xffffffff - startTicks + endTicks;
    if (__isrDbgTickMin > elapsedTicks)
        __isrDbgTickMin = elapsedTicks;
    if (__isrDbgTickMax < elapsedTicks)
        __isrDbgTickMax = elapsedTicks;
#endif
}

#endif
