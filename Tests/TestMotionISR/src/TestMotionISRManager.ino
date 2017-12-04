#include "MotionISRManager.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

MotionISRManager motionISRManager;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("TestMotionISR starting 4");
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

bool addSomeSteps(int i)
{
    if (motionISRManager.canAdd())
    {
        motionISRManager.addAxisSteps(0, 80, i > 3, 500 * ((i%4)+1));
        motionISRManager.addAxisSteps(1, 80, i > 3, 500 * ((i%4)+1));
        motionISRManager.addComplete();
        return true;
    }
    return false;
}

int nextAdd = 0;
void loop() {

    delay(50);

    motionISRManager.showDebug();

    bool addOk = addSomeSteps(nextAdd);
    if (addOk)
    {
       Serial.printlnf("Done put: getPos, putPos %d,%d", __isrRingBufferPosn._getPos, __isrRingBufferPosn._putPos);
       nextAdd++;
       if (nextAdd > 7)
        nextAdd = 0;
   }
    else
    {
       Serial.printlnf("Can't add to queue: getPos, putPos %d,%d", __isrRingBufferPosn._getPos, __isrRingBufferPosn._putPos);
   }
}
