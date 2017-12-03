#include "SparkIntervalTimer.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

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

const int MAX_STEP_GROUPS = 20;

RingBufferPosn ringBufferPosn(MAX_STEP_GROUPS);

class ISRAxisMotionVars
{
public:
    unsigned long stepUs[MAX_STEP_GROUPS];
    unsigned long stepNum[MAX_STEP_GROUPS];
    unsigned long stepDirn[MAX_STEP_GROUPS];
    unsigned long usAccum;
    unsigned long stepCount;
    bool _isActive;
    int _stepPin;
    bool _stepPinValue;
    int _dirnPin;
    bool _dirnPinValue;
    // Debug
    bool dbgLastStepUsValid;
    unsigned long dbgLastStepUs;
    unsigned long dbgMaxStepUs;
    unsigned long dbgMinStepUs;

    ISRAxisMotionVars()
    {
        usAccum = 0;
        stepCount = 0;
        _isActive = false;
        _stepPin = -1;
        _stepPinValue = false;
        _dirnPin = -1;
        _dirnPinValue = false;
        clearDbg();
    }

    void clearDbg()
    {
        dbgLastStepUsValid = false;
        dbgMaxStepUs = 0;
        dbgMinStepUs = 10000000;
    }

    void setPins(int stepPin, int dirnPin)
    {
        _stepPin = stepPin;
        _dirnPin = dirnPin;
        digitalWrite(_stepPin, false);
        _stepPinValue = false;
        digitalWrite(_dirnPin, false);
        _dirnPinValue = false;
    }
};

static const int ISR_MAX_AXES = 3;
ISRAxisMotionVars axisVars[ISR_MAX_AXES];

void myisr(void)
{
    static unsigned long lastUs = micros();
    // Get current uS elapsed
    unsigned long curUs = micros();
    unsigned long elapsed = curUs - lastUs;
    if (lastUs > curUs)
        elapsed = 0xffffffff-lastUs+curUs;
    // Check if queue is empty
    if (!ringBufferPosn.canGet())
        return;

    bool allAxesDone = true;
    // Go through axes
    for (int axisIdx = 0; axisIdx < ISR_MAX_AXES; axisIdx++)
    {
        // Get pointer to this axis
        volatile ISRAxisMotionVars* pAxisVars = &(axisVars[axisIdx]);
        if (!pAxisVars->_isActive)
        {
            allAxesDone = false;
            pAxisVars->usAccum += elapsed;
            unsigned long stepUs = pAxisVars->stepUs[ringBufferPosn._getPos];
            if (pAxisVars->usAccum > stepUs)
            {
                if (pAxisVars->usAccum > stepUs + stepUs)
                    pAxisVars->usAccum = 0;
                else
                    pAxisVars->usAccum -= stepUs;
                // Direction
                if (pAxisVars->_dirnPinValue != pAxisVars->stepDirn[ringBufferPosn._getPos])
                {
                    pAxisVars->_dirnPinValue = pAxisVars->stepDirn[ringBufferPosn._getPos];
                    digitalWrite(pAxisVars->_dirnPin, pAxisVars->_dirnPinValue);
                }
                // Step
                pAxisVars->_stepPinValue = !pAxisVars->_stepPinValue;
                digitalWrite(pAxisVars->_stepPin, pAxisVars->_stepPinValue);
                pAxisVars->stepCount++;
                if (pAxisVars->stepCount >= pAxisVars->stepNum[ringBufferPosn._getPos])
                {
                    pAxisVars->_isActive = true;
                    pAxisVars->stepCount = 0;
                    pAxisVars->usAccum = 0;
                    pAxisVars->dbgLastStepUsValid = false;
                }
                else
                {
                    // Check timing
                    if (pAxisVars->dbgLastStepUsValid)
                    {
                        unsigned long betweenStepsUs = curUs - pAxisVars->dbgLastStepUs;
                        if (pAxisVars->dbgMaxStepUs < betweenStepsUs)
                            pAxisVars->dbgMaxStepUs = betweenStepsUs;
                        if (pAxisVars->dbgMinStepUs > betweenStepsUs)
                            pAxisVars->dbgMinStepUs = betweenStepsUs;
                    }
                    // Record last time
                    pAxisVars->dbgLastStepUs = curUs;
                    pAxisVars->dbgLastStepUsValid = true;
                }
            }
        }
    }
    if (allAxesDone)
        ringBufferPosn.hasGot();
    lastUs = curUs;
}

void setup() {
   Serial.begin(115200);
   delay(2000);
   myTimer.begin(myisr, 10, uSec);
   Serial.println("TestMotionISRManager starting");
   pinMode(A2, OUTPUT);
   digitalWrite(A2, 0);
   pinMode(D2, OUTPUT);
   pinMode(D3, OUTPUT);
   pinMode(D4, OUTPUT);
   pinMode(D5, OUTPUT);
   axisVars[0].setPins(D2, D3);
   axisVars[1].setPins(D4, D5);
   delay(2000);
   digitalWrite(A2, 1);
}

void addSteps(int axisIdx, int stepNum, bool stepDirection, unsigned long uSBetweenSteps)
{
    axisVars[axisIdx].stepUs[ringBufferPosn._putPos] = uSBetweenSteps;
    axisVars[axisIdx].stepNum[ringBufferPosn._putPos] = stepNum;
    axisVars[axisIdx].stepDirn[ringBufferPosn._putPos] = stepDirection;
    axisVars[axisIdx].usAccum = 0;
    axisVars[axisIdx].stepCount = 0;
    axisVars[axisIdx]._isActive = (stepNum == 0);
    axisVars[axisIdx].dbgLastStepUsValid = false;
    axisVars[axisIdx].dbgMinStepUs = 10000000;
    axisVars[axisIdx].dbgMaxStepUs = 0;
}

void loop() {
    delay(10000);
    Serial.printlnf("%lu,%lu %lu,%lu %lu,%lu %d %d",
                axisVars[0].dbgMinStepUs, axisVars[0].dbgMaxStepUs,
                axisVars[1].dbgMinStepUs, axisVars[1].dbgMaxStepUs,
                axisVars[2].dbgMinStepUs, axisVars[2].dbgMaxStepUs,
                ringBufferPosn._putPos, ringBufferPosn._getPos);

    for (int i = 0; i < ISR_MAX_AXES; i++)
    {
        axisVars[i].dbgMinStepUs = 10000000;
        axisVars[i].dbgMaxStepUs = 0;
    }

    // for (int i = 0; i < 1000; i++)
    // {
    //     digitalWrite(D2, 0);
    //     digitalWrite(D4, 0);
    //     delay(1);
    //     digitalWrite(D2, 1);
    //     digitalWrite(D4, 1);
    //     delay(1);
    // }
    // Add to ring buffer
    if (ringBufferPosn.canPut())
    {
       addSteps(0, 30000, 0, 100);
       addSteps(1, 60000, 0, 50);
    //    addSteps(2, 30, 0, 100000);
       ringBufferPosn.hasPut();
       Serial.printlnf("Put 10000 %d %d", ringBufferPosn._putPos, ringBufferPosn._getPos);
    }
    else
    {
       Serial.println("Can't put");
    }
}
