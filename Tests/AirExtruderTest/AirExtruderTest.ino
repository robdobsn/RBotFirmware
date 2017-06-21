
#include "application.h"

const int FET0 = A0;
const int FET1 = A1;

void setup()
{
    Serial.begin(115200);
    delay(5000);
    Serial.printlnf("Air extruder test (built %s %s)", __DATE__, __TIME__);
    Serial.printlnf("System version: %s", (const char*)System.version());

    pinMode(FET0, OUTPUT);
    pinMode(FET1, OUTPUT);
}

int val = 0;
void loop()
{
    digitalWrite(FET0, val % 2);
    digitalWrite(FET1, val / 2);
    val++;
    delay(2000);
}
