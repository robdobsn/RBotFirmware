#include <Arduino.h>
#include "RobotConsts.h"
#include "StepperMotor.h"

#define MICROSTEPPING_FACTOR 16

const int _stepEnablePin = 4;
const int _stepEnLev = 1;
const int _step1Step = 14;
const int _step1Dirn = 13;
const int _step2Step = 15;
const int _step2Dirn = 21;
const int _endStop1 = 36;
const int _endStop2 = 39;
const int _fet1 = 32;

StepperMotor step1(RobotConsts::MOTOR_TYPE_DRIVER, _step1Step, _step1Dirn);
StepperMotor step2(RobotConsts::MOTOR_TYPE_DRIVER, _step2Step, _step2Dirn);


class PinDeb
{
  public:
    bool _pinLastVal;
    unsigned long _pinLastChgMillis;
    int _pinNum;
    char _sName[100];
    bool _firstShow;
    PinDeb(int pinNum, const char *sName)
    {
        _pinNum = pinNum;
        strcpy(_sName, sName);
        _firstShow = true;
        pinMode(_pinNum, INPUT_PULLDOWN);
    }
    bool hasChanged(bool &curVal)
    {
        bool val = digitalRead(_pinNum) == HIGH;
        curVal = val;
        if (val == _pinLastVal)
            return false;
        if (millis() - _pinLastChgMillis < 100)
            return false;
        _pinLastChgMillis = millis();
        _pinLastVal = val;
        return true;
    }
    void show()
    {
        bool val = false;
        bool chg = hasChanged(val);
        if (chg || _firstShow)
            Serial.printf("%s %s\r\n", _sName, val ? "HIGH" : "LOW");
        _firstShow = false;
    }
};

PinDeb pinSw1Sense(_endStop1, "Sw1");
PinDeb pinSw2Sense(_endStop2, "Sw2");


void setup() {
    Serial.begin(115200);
    pinMode(_stepEnablePin, OUTPUT);
    digitalWrite(_stepEnablePin, !_stepEnLev);
    pinMode(_fet1, OUTPUT);
    digitalWrite(_fet1, LOW);
    Serial.printf("Test Pi/Suo Stepper with ESP32 Backpack\n\n");
    Serial.printf("1 - test stepper 1\n");
    Serial.printf("2 - test stepper 2\n");
    Serial.printf("3 - test FET 1\n");
}

void loop() {
    
    if (Serial.available())
    {
        int ch = Serial.read();
        if ((ch == '1') || (ch == '2'))
        {
            Serial.printf("Performing one revolution forward & 1 back with microstep factor %d\n", MICROSTEPPING_FACTOR);
            digitalWrite(_stepEnablePin, _stepEnLev);
            for (int dirn = 0; dirn < 2; dirn++)
            {
                for (int i = 0; i < 200 * MICROSTEPPING_FACTOR; i++)
                {
                    if (ch == '1')
                        step1.stepSync(dirn);
                    else
                        step2.stepSync(dirn);
                    delayMicroseconds(500);
                    pinSw1Sense.show();
                    pinSw2Sense.show();
                }
            }
            delay(100);
            digitalWrite(_stepEnablePin, !_stepEnLev);
        }
        if (ch == '3')
        {
            Serial.printf("FET1 turning on for 1 second\n");
            digitalWrite(_fet1, HIGH);
            delay(1000);
            digitalWrite(_fet1, LOW);
        }
    }

    pinSw1Sense.show();
    pinSw2Sense.show();
}