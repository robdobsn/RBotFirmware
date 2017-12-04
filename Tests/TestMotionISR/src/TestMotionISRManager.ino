#include "MotionISRManager.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

MotionISRManager motionISRManager;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("TestMotionISR starting2");
    pinMode(A2, OUTPUT);  // stepper enable
    pinMode(D2, OUTPUT);
    pinMode(D3, OUTPUT);
    pinMode(D4, OUTPUT);
    pinMode(D5, OUTPUT);
    motionISRManager.setAxis(0, D2, D3);
    motionISRManager.setAxis(1, D4, D5);
    delay(2000);
    digitalWrite(A2, 1);
    motionISRManager.start();
}

bool addSomeSteps()
{
    if (motionISRManager.canAdd())
    {
        motionISRManager.addAxisSteps(0, 3200, 0, 1000);
        motionISRManager.addAxisSteps(1, 6400, 0, 500);
        motionISRManager.addComplete();
        return true;
    }
    return false;
}

void loop() {

    delay(10000);

    motionISRManager.showDebug();

    bool addOk = addSomeSteps();
    if (addOk)
       Serial.printlnf("Put steps putPos, getPos %d,%d", __isrRingBufferPosn._putPos, __isrRingBufferPosn._getPos);
    else
       Serial.println("Can't add to queue");
}
