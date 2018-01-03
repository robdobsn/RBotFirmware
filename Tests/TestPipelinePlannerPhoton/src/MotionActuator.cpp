// RBotFirmware
// Rob Dobson 2018

#include "MotionActuator.h"

#ifdef USE_SPARK_INTERVAL_TIMER_ISR

// Static interval timer
IntervalTimer MotionActuator::_isrMotionTimer;

// Static refrerence to a single MotionActuator instance
MotionActuator* MotionActuator::_pMotionActuatorInstance = NULL;

uint32_t MotionActuator::_testCount = 0;

void MotionActuator::_isrStepperMotion(void)
{
    int blinkRate = 10000;
    if (_pMotionActuatorInstance != NULL)
        blinkRate = 2500;
	_testCount++;
	if (_testCount > blinkRate)
	{
		_testCount = 0;
		digitalWrite(D7, !digitalRead(D7));
	}
}

#endif
