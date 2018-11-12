// Auto Dimming LED Strip
// Matt Grammes 2018

#pragma once

#include "Arduino.h"
#include "Utils.h"
#include "ConfigBase.h"
#include "ConfigPinMap.h"
#include <analogWrite.h>

class LedStrip
{
  public:
    LedStrip()
    {
        _isSetup = false;
        _ledPin = -1;
        _sensorPin = -1;
    }

    void setup(ConfigBase& hwConfig, const char* ledStripName, ConfigBase *pSysConfig)
    {
        _pConfigBase = pSysConfig;
        String ledStripConfigStr = _pConfigBase->getConfigData();
        if (ledStripConfigStr.length() == 0 || ledStripConfigStr.equals("{}")) {
            Log.trace("LedStrip: Initializing default config: %s\n", _ledStripDefaultJSON);
            _pConfigBase->setConfigData(_ledStripDefaultJSON);
            _pConfigBase->writeConfig();
        }

        ConfigBase ledConfig(hwConfig.getString(ledStripName, "").c_str());
        String pinStr = ledConfig.getString("ledPin", "");
        int ledPin = -1;
        if (pinStr.length() != 0)
            ledPin = ConfigPinMap::getPinFromName(pinStr.c_str());
        String sensorStr = ledConfig.getString("sensorPin", "");
        int sensorPin = -1;
        if (sensorStr.length() != 0)
            sensorPin = ConfigPinMap::getPinFromName(sensorStr.c_str());
        Log.notice("LedStrip: LED pin %d Sensor pin %d\n", ledPin, sensorPin);
        if (ledPin == -1)
            return;

        // Check if this is a change
        if (_isSetup)
        {
            if (ledPin != _ledPin)
            {
                pinMode(_ledPin, INPUT);
            }
            else
            {
                // No change so nothing to do
                Log.notice("LedStrip: No change\n");
                return;
            }
        }


        // Setup the pin
        _ledPin = ledPin;
        _sensorPin = sensorPin;
        pinMode(_ledPin, OUTPUT);
        if (_sensorPin != -1) {
            pinMode(_sensorPin, INPUT);
            for (int i = 0; i < NUM_SENSOR_VALUES; i++) {
                sensorValues[i] = 0;
            }
        }

        // Start off at dimmest
        _ledValue = 0x0;
        analogWrite(_ledPin, _ledValue);

        _isSetup = true;
    }

    void setAutoDim(boolean autoDim) {
        _autoDim = autoDim;
    }

    void setLedValue(boolean ledValue) {
        _ledValue = ledValue;
    }

    void service()
    {
        // Check if active
        if (!_isSetup)
            return;

        _ledOn = _pConfigBase->getLong("ledOn", 0) == 1;
        //_ledValue = _pConfigBase->getLong("ledValue", 0xFF);
        _autoDim = _pConfigBase->getLong("autoDim", 0) == 1;

        byte ledValue = _pConfigBase->getLong("ledValue", 0xFF);

        // If the switch is off, turn off the led
        if (!_ledOn) {
            //Log.trace("LED is switched off\n");
            ledValue = 0x0;
        } else {
            //Log.trace("Using set led value: 0x%x\n", _ledValue);
            // Check if we need to read and evaluate the light sensor
            if (_autoDim) {
                if (_sensorPin != -1) {
                    sensorValues[sensorReadingCount++ % NUM_SENSOR_VALUES] = analogRead(_sensorPin);
                    uint16_t sensorAvg = getAverageSensorReading();
                    Log.trace("Ambient Light Avg Value: %d\n", sensorAvg);

                    // Convert ambient light to led value
                    int ledBrightnessInt = 0x88 + (sensorAvg / 4);
                    if (ledBrightnessInt > 255) {
                        ledBrightnessInt = 255;
                    }
                    ledValue = ledBrightnessInt;
                    // TODO write new value to config
                }            
            }
        }

        if (ledValue != _ledValue) {
            _ledValue = ledValue;
            Log.trace("Writing LED Value: 0x%x\n", _ledValue);
            analogWrite(_ledPin, _ledValue);
        }
    }

  private:
    static const int NUM_SENSOR_VALUES = 16;
    bool _isSetup;
    bool _ledOn;
    int _ledPin;
    int _sensorPin;
    byte _ledValue = -1;
    bool _autoDim = false;
    int sensorReadingCount = 0;

    ConfigBase* _pConfigBase;

    const char * _ledStripDefaultJSON = {
    "{"
    "\"ledOn\":1,"
    "\"ledValue\":0,"
    "\"autoDim\":0"
    "}"};


    // Store a number of sensor readings for smoother transitions
    uint16_t sensorValues[NUM_SENSOR_VALUES];

    // Get the average sensor reading
    uint16_t getAverageSensorReading() {
        uint16_t sum = 0;
        for (int i = 0; i < NUM_SENSOR_VALUES; i++) {
            sum += sensorValues[i];
        }
        return sum / NUM_SENSOR_VALUES;
    }
    
};