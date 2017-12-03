#include "SparkIntervalTimer.h"

SYSTEM_MODE(AUTOMATIC);
//SYSTEM_THREAD(ENABLED);

int testLed = D7;

IntervalTimer myTimer;

class RingBufferPosn
{
public:
    volatile unsigned int _putPos;
    volatile unsigned int _getPos;
    unsigned int _bufLen;

    RingBufferPosn(int maxLen)
    {
        _bufLen = maxLen;
    }

    bool canPut()
    {
        if (_putPos == _getPos)
            return true;
        if (_putPos > _getPos)
            if ((_putPos != _bufLen-1) || (_getPos != 0))
                return true;
        else
            if (_getPos - _putPos > 1)
                return true;
        return false;
    }

    bool canGet()
    {
        return _putPos != _getPos;
    }

    void hasPut()
    {
        _putPos++;
        if (_putPos >= _bufLen)
            _putPos = 0;
    }

    void hasGot()
    {
        _getPos++;
        if (_getPos >= _bufLen)
            _getPos = 0;
    }
};

const int MAX_STEP_GROUPS = 100;

RingBufferPosn ringBufferPosn(MAX_STEP_GROUPS);

unsigned long axisAStepUs[MAX_STEP_GROUPS];
unsigned long axisAStepNum[MAX_STEP_GROUPS];
unsigned long axisBStepUs[MAX_STEP_GROUPS];
unsigned long axisBStepNum[MAX_STEP_GROUPS];

volatile unsigned long axisAUsAccum = 0;
volatile unsigned long axisAStepCount = 0;
volatile unsigned long axisBUsAccum = 0;
volatile unsigned long axisBStepCount = 0;

volatile unsigned long isrCount = 0;
volatile unsigned long lastUs = micros();

volatile unsigned long lastStepUsValid = false;
volatile unsigned long lastStepUs = 0;
volatile unsigned long maxStepUs = 0;
volatile unsigned long minStepUs = 1000000;

volatile bool testLedVal = false;

void myisr(void)
{
//    isrCount++;
    unsigned long cur = micros();
    unsigned long elapsed = cur - lastUs;
    if (!ringBufferPosn.canGet())
    {
        lastStepUsValid = false;
        return;
    }
    axisAUsAccum += elapsed;
    unsigned long stepUs = axisAStepUs[ringBufferPosn._getPos];
    if (axisAUsAccum > stepUs)
    {
        if (axisAUsAccum > stepUs + stepUs)
            axisAUsAccum = 0;
        else
            axisAUsAccum -= stepUs;
        // Step
        digitalWrite(testLed, testLedVal);
        testLedVal = !testLedVal;
        axisAStepCount++;
        if (axisAStepCount >= axisAStepNum[ringBufferPosn._getPos])
        {
            axisAStepCount = 0;
            ringBufferPosn.hasGot();
            axisAUsAccum = 0;
        }
        // Check timing
        if (lastStepUsValid)
        {
            unsigned long betweenStepsUs = cur - lastStepUs;
            if (maxStepUs < betweenStepsUs)
                maxStepUs = betweenStepsUs;
            if (minStepUs > betweenStepsUs)
                minStepUs = betweenStepsUs;
        }
        // Record last time
        lastStepUs = cur;
        lastStepUsValid = true;
    }
    lastUs = cur;
}

void setup() {
   Serial.begin(115200);
   myTimer.begin(myisr, 100, uSec);
   Serial.println("TestRingBufferISR starting");
   pinMode(testLed, OUTPUT);
}

void loop() {
   delay(10000);
   Serial.printlnf("%lu %lu %d %d", minStepUs, maxStepUs, ringBufferPosn._putPos, ringBufferPosn._getPos);
   minStepUs = 100000000;
   maxStepUs = 0;

   // Add to ring buffer
   if (ringBufferPosn.canPut())
   {
       axisAStepUs[ringBufferPosn._putPos] = 100000;
       axisAStepNum[ringBufferPosn._putPos] = 20;
       ringBufferPosn.hasPut();
       Serial.printlnf("Put 10000 %d %d", ringBufferPosn._putPos, ringBufferPosn._getPos);
   }
   else
   {
       Serial.println("Can't put");
   }
}
