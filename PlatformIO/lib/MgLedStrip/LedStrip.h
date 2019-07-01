// Auto Dimming LED Strip
// Matt Grammes 2018

#pragma once

#include "Utils.h"
#include "ConfigNVS.h"
#include "ConfigPinMap.h"

class LedStrip
{
public:
    LedStrip(ConfigBase &ledNvValues);
    void setup(ConfigBase* pConfig, const char* ledStripName);
    void service();
    void updateLedFromConfig(const char* pLedJson);
    const char* getConfigStrPtr();
    void setSleepMode(int sleep);

private:
    void configChanged();
    void updateNv();
    uint16_t getAverageSensorReading();

private:
    String _name;
    ConfigBase* _pHwConfig;
    static const int NUM_SENSOR_VALUES = 100;
    bool _isSetup;
    bool _isSleeping;
    int _ledPin;
    int _sensorPin;

    bool _ledOn;
    byte _ledValue = -1;
    bool _autoDim = false;
    bool ledConfigChanged = false;
    
    int sensorReadingCount = 0;

    // Store the settings for LED in NV Storage
    ConfigBase& _ledNvValues;

    // Store a number of sensor readings for smoother transitions
    uint16_t sensorValues[NUM_SENSOR_VALUES];

    // LEDC library controls
    static const int LED_STRIP_PWM_FREQ = 7000;
    static const int LED_STRIP_LEDC_CHANNEL = 0;
    static const int LED_STRIP_LEDC_RESOLUTION = 8;
};