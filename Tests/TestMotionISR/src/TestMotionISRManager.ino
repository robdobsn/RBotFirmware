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

void loop() {

    delay(10000);

    motionISRManager.showDebug();

    // Add to ring buffer
    if (__isrRingBufferPosn.canPut())
    {
       motionISRManager.addSteps(0, 30000, 0, 100);
       motionISRManager.addSteps(1, 60000, 0, 50);
       __isrRingBufferPosn.hasPut();
       Serial.printlnf("Put 10000 %d %d", __isrRingBufferPosn._putPos, __isrRingBufferPosn._getPos);
    }
    else
    {
       Serial.println("Can't put");
    }
}
