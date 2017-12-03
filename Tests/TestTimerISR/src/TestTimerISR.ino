/*
 * Project TestTimerISR
 * Description:
 * Author:
 * Date:
 */

 #include "SparkIntervalTimer.h"

SYSTEM_MODE(AUTOMATIC);
//SYSTEM_THREAD(ENABLED);

IntervalTimer myTimer;

volatile unsigned long isrCount = 0;
volatile unsigned long lastUs = 0;
volatile unsigned long maxUs = 0;
volatile unsigned long minUs = 1000000;

void isr(void)
{
//    isrCount++;
    unsigned long cur = micros();
    if (lastUs != 0)
    {
        unsigned long elapsed = cur - lastUs;
        if (maxUs < elapsed)
            maxUs = elapsed;
        if (minUs > elapsed)
            minUs = elapsed;
    }
    lastUs = cur;
}

void setup() {
    Serial.begin(115200);
    myTimer.begin(isr, 1000, uSec);
}

void loop() {
    delay(10000);
    Serial.printlnf("%lu %lu", minUs, maxUs);
    minUs = 100000000;
    maxUs = 0;
}
