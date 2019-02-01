#include <Arduino.h>
#include <unity.h>
#include "UnitTestHomingSeq.h"
#include "UnitTestMiniHDLC.h"

void setUp(void) {
// set stuff up here
}

void tearDown(void) {
// clean stuff up here
}

void testHomingSeq(void) {
    UnitTestHomingSeq unitTestHomingSeq;
    unitTestHomingSeq.runTests();
}

void testMiniHDLC(void) {
    UnitTestMiniHDLC unitTestMiniHDLC;
    unitTestMiniHDLC.runTests();
}

void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();    // IMPORTANT LINE!

    // Run the tests
    RUN_TEST(testMiniHDLC);
    RUN_TEST(testHomingSeq);

    UNITY_END(); // stop unit testing

}

void loop() {
}