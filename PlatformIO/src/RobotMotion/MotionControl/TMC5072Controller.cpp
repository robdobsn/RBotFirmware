// RBotFirmware
// Rob Dobson 2016-19

#include "TMC5072Controller.h"
#include "RdJson.h"
#include "ConfigPinMap.h"
#include <functional>

static const char* MODULE_PREFIX = "TMC5072Controller: ";

TMC5072Controller* TMC5072Controller::_pThisObj = NULL;

TMC5072Controller::TMC5072Controller()
{
    _isEnabled = false;
    _trinamicsTimerStarted = false;
    _pThisObj = this;
}

TMC5072Controller::~TMC5072Controller()
{
    deinit();
}

void TMC5072Controller::configure(const char *configJSON)
{
    Log.verbose("%sconfigure %s\n", MODULE_PREFIX, configJSON);

    // Check for trinamics controller config JSON
    String motionController = RdJson::getString("motionController", "NONE", configJSON);

    // Get chip
    String mcChip = RdJson::getString("chip", "NONE", motionController.c_str());
    Log.trace("%sconfigure motionController %s chip %s\n", MODULE_PREFIX, motionController.c_str(), mcChip.c_str());

    if (mcChip == "TMC5072")
    {
        // SPI settings
        String pinName = RdJson::getString("MOSI", "", motionController.c_str());
        int spiMOSIPin = ConfigPinMap::getPinFromName(pinName.c_str());
        pinName = RdJson::getString("MISO", "", motionController.c_str());
        int spiMISOPin = ConfigPinMap::getPinFromName(pinName.c_str());
        pinName = RdJson::getString("CLK", "", motionController.c_str());
        int spiCLKPin = ConfigPinMap::getPinFromName(pinName.c_str());
        pinName = RdJson::getString("CS1", "", motionController.c_str());
        int spiCS1Pin = ConfigPinMap::getPinFromName(pinName.c_str());
        pinName = RdJson::getString("CS2", "", motionController.c_str());
        int spiCS2Pin = ConfigPinMap::getPinFromName(pinName.c_str());
        
        // Check valid
        if (spiMOSIPin == -1 || spiMISOPin == -1 || spiCLKPin == -1 || spiCS1Pin == -1 || spiCS2Pin == -1)
        {
            Log.warning("%ssetup SPI pins invalid\n", MODULE_PREFIX);
        }
        else
        {
            _miso = spiMISOPin;
            _mosi = spiMOSIPin;
            _sck = spiCLKPin;
            _cs1 = spiCS1Pin;
            _cs2 = spiCS2Pin;
            _isEnabled = true;

            // Start timer
            // int id = 1;
            // _trinamicsTimerHandle = xTimerCreate("MyTimer", pdMS_TO_TICKS(TRINAMIC_TIMER_PERIOD_MS), pdTRUE, ( void * )id, 
            //             std::bind(&TMC5072Controller::_timerCallback, this, std::placeholders::_1));
            
            // esp_timer_create_args_t timer_args = {
            //         .callback = &timer_func,
            //         .arg = &args,
            //         .name = "self_deleter"
            // };
            const esp_timer_create_args_t _timerArgs = {
                        &TMC5072Controller::_staticTimerCb,
                        NULL,
                        esp_timer_dispatch_t::ESP_TIMER_TASK,
                        "Trinamic"
            };

            esp_timer_create(&_timerArgs, &_trinamicsTimerHandle);

            esp_timer_start_periodic(_trinamicsTimerHandle, 500000);

            Log.trace("%sStarting timer for trinamics\n", MODULE_PREFIX);
            _trinamicsTimerStarted = true;
        }
    }
}

void TMC5072Controller::deinit()
{
    if (_trinamicsTimerStarted)
    {
        Log.trace("%sDe-init\n", MODULE_PREFIX);
        esp_timer_stop(_trinamicsTimerHandle);
        // xTimerStop(_trinamicsTimerHandle, 0);
        _trinamicsTimerStarted = false;
    }
    _isEnabled = false;
}

void TMC5072Controller::process()
{

}

void TMC5072Controller::_timerCallback(void* arg)
{
    Log.notice("tring\n");
}
