#include "SparkIntervalTimer.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

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

typedef struct ISRAxisMotionVars
{
    unsigned long stepUs[MAX_STEP_GROUPS];
    unsigned long stepNum[MAX_STEP_GROUPS];
    unsigned long usAccum;
    unsigned long stepCount;
    // Debug
    bool lastStepUsValid;
    unsigned long lastStepUs;
    unsigned long maxStepUs;
    unsigned long minStepUs;
};

static const int ISR_MAX_AXES = 3;
volatile ISRAxisMotionVars axisVars[ISR_MAX_AXES];

//unsigned long axisAStepUs[MAX_STEP_GROUPS];
//unsigned long axisAStepNum[MAX_STEP_GROUPS];

//volatile unsigned long axisAUsAccum = 0;
//volatile unsigned long axisAStepCount = 0;

volatile unsigned long isrCount = 0;
volatile unsigned long lastUs = micros();

volatile unsigned long lastStepUsValid = false;
volatile unsigned long lastStepUs = 0;
volatile unsigned long maxStepUs = 0;
volatile unsigned long minStepUs = 1000000;

volatile bool testLedVal = false;

void myisr(void)
{
    // Get current uS elapsed
    unsigned long curUs = micros();
    unsigned long elapsed = curUs - lastUs;
    if (lastUs > curUs)
        elapsed = 0xffffffff-lastUs+curUs;
    // Check if queue is empty
    if (!ringBufferPosn.canGet())
        return;
    // Get pointer to this axis
    volatile ISRAxisMotionVars* pAxisVars = &(axisVars[0]);
    pAxisVars->usAccum += elapsed;
    unsigned long stepUs = pAxisVars->stepUs[ringBufferPosn._getPos];
    if (pAxisVars->usAccum > stepUs)
    {
        if (pAxisVars->usAccum > stepUs + stepUs)
            pAxisVars->usAccum = 0;
        else
            pAxisVars->usAccum -= stepUs;
        // Step
        testLedVal = !testLedVal;
        digitalWrite(testLed, testLedVal);
        pAxisVars->stepCount++;
        if (pAxisVars->stepCount >= pAxisVars->stepNum[ringBufferPosn._getPos])
        {
            pAxisVars->stepCount = 0;
            ringBufferPosn.hasGot();
            pAxisVars->usAccum = 0;
            lastStepUsValid = false;
        }
        else
        {
            // Check timing
            if (lastStepUsValid)
            {
                unsigned long betweenStepsUs = curUs - lastStepUs;
                if (maxStepUs < betweenStepsUs)
                    maxStepUs = betweenStepsUs;
                if (minStepUs > betweenStepsUs)
                    minStepUs = betweenStepsUs;
            }
            // Record last time
            lastStepUs = curUs;
            lastStepUsValid = true;
        }
    }
    lastUs = curUs;
}

void setup() {
   Serial.begin(115200);
   myTimer.begin(myisr, 100, uSec);
   Serial.println("TestMotionISRManager starting");
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
       axisVars[0].stepUs[ringBufferPosn._putPos] = 500000;
       axisVars[0].stepNum[ringBufferPosn._putPos] = 6;
       axisVars[1].stepUs[ringBufferPosn._putPos] = 250000;
       axisVars[1].stepNum[ringBufferPosn._putPos] = 12;
       axisVars[2].stepUs[ringBufferPosn._putPos] = 100000;
       axisVars[2].stepNum[ringBufferPosn._putPos] = 30;
       ringBufferPosn.hasPut();
       Serial.printlnf("Put 10000 %d %d", ringBufferPosn._putPos, ringBufferPosn._getPos);
   }
   else
   {
       Serial.println("Can't put");
   }
}
