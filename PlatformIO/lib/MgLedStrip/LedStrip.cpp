// Auto Dimming LED Strip
// Matt Grammes 2018

#include "LedStrip.h"
#include "Arduino.h"
#include "ConfigNVS.h"

static const char* MODULE_PREFIX = "LedStrip: ";

LedStrip::LedStrip(ConfigBase &ledNvValues) : _ledNvValues(ledNvValues)
{
    _isSetup = false;
    _isSleeping = false;
    _ledPin = -1;
    _sensorPin = -1;
}

void LedStrip::setup(ConfigBase* pConfig, const char* ledStripName)
{
    _name = ledStripName;
    // Save config and register callback on config changed
    if (_pHwConfig == NULL)
    {
        _pHwConfig = pConfig;
        _pHwConfig->registerChangeCallback(std::bind(&LedStrip::configChanged, this));
    }


    // Get LED config
    ConfigBase ledConfig(pConfig->getString(ledStripName, "").c_str());
    Log.trace("%ssetup name %s configStr %s\n", MODULE_PREFIX, _name.c_str(),
                    ledConfig.getConfigCStrPtr());


    // LED Strip Negative PWM Pin
    String pinStr = ledConfig.getString("ledPin", "");
    int ledPin = -1;
    if (pinStr.length() != 0)
        ledPin = ConfigPinMap::getPinFromName(pinStr.c_str());

    // Ambient Light Sensor Pin
    String sensorStr = ledConfig.getString("sensorPin", "");
    int sensorPin = -1;
    if (sensorStr.length() != 0)
        sensorPin = ConfigPinMap::getPinFromName(sensorStr.c_str());

    Log.notice("%sLED pin %d Sensor pin %d\n", MODULE_PREFIX, ledPin, sensorPin);
    // Sensor pin isn't necessary for operation.
    if (ledPin == -1)
        return;

    // Setup led pin
    if (_isSetup && (ledPin != _ledPin))
    {
        ledcDetachPin(_ledPin);
    }
    else
    {
        _ledPin = ledPin;
        ledcSetup(LED_STRIP_LEDC_CHANNEL, LED_STRIP_PWM_FREQ, LED_STRIP_LEDC_RESOLUTION);
        ledcAttachPin(_ledPin, LED_STRIP_LEDC_CHANNEL);
    }

    // Setup the sensor
    _sensorPin = sensorPin;
    if (_sensorPin != -1) {
        pinMode(_sensorPin, INPUT);
        for (int i = 0; i < NUM_SENSOR_VALUES; i++) {
            sensorValues[i] = 0;
        }
    }

    // If there is no LED data stored, set to default
    String ledStripConfigStr = _ledNvValues.getConfigString();
    if (ledStripConfigStr.length() == 0 || ledStripConfigStr.equals("{}")) {
        Log.trace("%sNo LED Data Found in NV Storge, Defaulting\n", MODULE_PREFIX);
        // Default to LED On, Half Brightness
        _ledOn = true;
        _ledValue = 0x7f;
        _autoDim = false;
        updateNv();
    } else {
        _ledOn = _ledNvValues.getLong("ledOn", 0) == 1;
        _ledValue = _ledNvValues.getLong("ledValue", 0xFF);
        _autoDim = _ledNvValues.getLong("autoDim", 0) == 1;
        Log.trace("%sLED Setup from JSON: %s On: %d, Value: %d, Auto Dim: %d\n", MODULE_PREFIX, 
                    ledStripConfigStr.c_str(), _ledOn, _ledValue, _autoDim);
    }

    _isSetup = true;
    // Trigger initial write
    ledConfigChanged = true;
    Log.trace("%sLED Configured: On: %d, Value: %d, AutoDim: %d\n", MODULE_PREFIX, _ledOn, _ledValue, _autoDim);
}

void LedStrip::updateLedFromConfig(const char * pLedJson) {

    boolean changed = false;
    boolean ledOn = RdJson::getLong("ledOn", 0, pLedJson) == 1;
    if (ledOn != _ledOn) {
        _ledOn = ledOn;
        changed = true;
    }
    byte ledValue = RdJson::getLong("ledValue", 0, pLedJson);
    if (ledValue != _ledValue) {
        _ledValue = ledValue;
        changed = true;
    }
    boolean autoDim = RdJson::getLong("autoDim", 0, pLedJson) == 1;
    if (autoDim != _autoDim) {
        // TODO Never Enable Auto Dim
        //_autoDim = autoDim;
        _autoDim = false;
        changed = true;
    }

    if (changed)
        updateNv();
}

const char* LedStrip::getConfigStrPtr() {
    return _ledNvValues.getConfigCStrPtr();
}

void LedStrip::service()
{
    // Check if active
    if (!_isSetup)
        return;

    // If the switch is off or sleeping, turn off the led
    if (!_ledOn || _isSleeping)
    {
        _ledValue = 0x0;
    }
    else
    {
        // TODO Auto Dim isn't working as expected - this should never go enabled right now
        // Check if we need to read and evaluate the light sensor
        if (_autoDim)
        {
            if (_sensorPin != -1) {
                sensorValues[sensorReadingCount++ % NUM_SENSOR_VALUES] = analogRead(_sensorPin);
                uint16_t sensorAvg = LedStrip::getAverageSensorReading();
                // if (count % 100 == 0) {
                // Log.trace("Ambient Light Avg Value: %d, reading count %d\n", sensorAvg, sensorReadingCount % NUM_SENSOR_VALUES);
                
                // Convert ambient light (int) to led value (byte)
                int ledBrightnessInt = sensorAvg / 4;

                // This case shouldn't be hit
                if (ledBrightnessInt > 255) {
                    ledBrightnessInt = 255;
                    Log.error("%sAverage Sensor Value over max!\n", MODULE_PREFIX);
                }
                byte ledValue = ledBrightnessInt;
                if (_ledValue != ledValue) {
                    _ledValue = ledValue;
                    updateNv();
                }
            }
        }
    }

    if (ledConfigChanged) {
        ledConfigChanged = false;
        Log.trace("Writing LED Value: 0x%x\n", _ledValue);
        ledcWrite(LED_STRIP_LEDC_CHANNEL, _ledValue);
    }
}

void LedStrip::configChanged()
{
    // Reset config
    Log.trace("%sconfigChanged\n", MODULE_PREFIX);
    setup(_pHwConfig, _name.c_str());
}

void LedStrip::updateNv()
{
    String jsonStr;
    jsonStr += "{";
    jsonStr += "\"ledOn\":";
    jsonStr += _ledOn ? "1" : "0";
    jsonStr += ",";
    jsonStr += "\"ledValue\":";
    jsonStr += _ledValue;
    jsonStr += ",";
    jsonStr += "\"autoDim\":";
    jsonStr += _autoDim ? "1" : "0";
    jsonStr += "}";
    _ledNvValues.setConfigData(jsonStr.c_str());
    _ledNvValues.writeConfig();
    Log.trace("%supdateNv() : wrote %s\n", MODULE_PREFIX, _ledNvValues.getConfigCStrPtr());
    ledConfigChanged = true;
}

// Get the average sensor reading
uint16_t LedStrip::getAverageSensorReading() {
    uint16_t sum = 0;
    for (int i = 0; i < NUM_SENSOR_VALUES; i++) {
        sum += sensorValues[i];
    }
    return sum / NUM_SENSOR_VALUES;
}

// Set sleep mode
void LedStrip::setSleepMode(int sleep)
{
    _isSleeping = sleep;
}
