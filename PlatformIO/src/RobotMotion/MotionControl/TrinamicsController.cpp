// RBotFirmware
// Rob Dobson 2016-19

#include "TrinamicsController.h"
#include "RdJson.h"
#include "ConfigPinMap.h"
#include "Utils.h"
#include "TMC5072_registers.h"

static const char* MODULE_PREFIX = "TrinamicsController: ";

TrinamicsController* TrinamicsController::_pThisObj = NULL;

TrinamicsController::TrinamicsController()
{
    _isEnabled = false;
    _trinamicsTimerStarted = false;
    _pThisObj = this;
    _pVSPI = NULL;
    _miso = _mosi = _sck = -1;
    _cs1 = _cs2 = _cs3 =  -1;
    _mux1 = _mux2 = _mux3 = -1;
    _muxCS1 = _muxCS2 = _muxCS3 = -1;
}

TrinamicsController::~TrinamicsController()
{
    deinit();
}

void TrinamicsController::configure(const char *configJSON)
{
    Log.verbose("%sconfigure %s\n", MODULE_PREFIX, configJSON);

    // Check for trinamics controller config JSON
    String motionController = RdJson::getString("motionController", "NONE", configJSON);

    // Get chip
    String mcChip = RdJson::getString("chip", "NONE", motionController.c_str());
    Log.trace("%sconfigure motionController %s chip %s\n", MODULE_PREFIX, motionController.c_str(), mcChip.c_str());

    if ((mcChip == "TMC5072") || (mcChip == "TMC2130"))
    {
        // SPI settings
        String pinName = RdJson::getString("MOSI", "", motionController.c_str());
        int spiMOSIPin = ConfigPinMap::getPinFromName(pinName.c_str());
        pinName = RdJson::getString("MISO", "", motionController.c_str());
        int spiMISOPin = ConfigPinMap::getPinFromName(pinName.c_str());
        pinName = RdJson::getString("CLK", "", motionController.c_str());
        int spiCLKPin = ConfigPinMap::getPinFromName(pinName.c_str());

        // Check valid
        if (spiMOSIPin == -1 || spiMISOPin == -1 || spiCLKPin == -1)
        {
            Log.warning("%ssetup SPI pins invalid\n", MODULE_PREFIX);
        }
        else
        {
            _miso = spiMISOPin;
            _mosi = spiMOSIPin;
            _sck = spiCLKPin;
            _isEnabled = true;

            // Start SPI
            _pVSPI = new SPIClass(VSPI);
            _pVSPI->begin(_sck, _miso, _mosi); //SCLK, MISO, MOSI, SS - not specified
        }
    }

    // Handle CS (may be multiplexed)
    if (_isEnabled)
    {
        // Non-multiplexed cs pins
        _cs1 = getPinAndConfigure(motionController.c_str(), "CS1", OUTPUT, HIGH);
        _cs2 = getPinAndConfigure(motionController.c_str(), "CS2", OUTPUT, HIGH);
        _cs3 = getPinAndConfigure(motionController.c_str(), "CS3", OUTPUT, HIGH);

        // Multiplexer settings
        _mux1 = getPinAndConfigure(motionController.c_str(), "MUX1", OUTPUT, LOW);
        _mux2 = getPinAndConfigure(motionController.c_str(), "MUX2", OUTPUT, LOW);
        _mux3 = getPinAndConfigure(motionController.c_str(), "MUX3", OUTPUT, HIGH);
        String confName = RdJson::getString("MUX_CS_1", "", motionController.c_str());
        _muxCS1 = ConfigPinMap::getPinFromName(confName.c_str());
        confName = RdJson::getString("MUX_CS_2", "", motionController.c_str());
        _muxCS2 = ConfigPinMap::getPinFromName(confName.c_str());
        confName = RdJson::getString("MUX_CS_3", "", motionController.c_str());
        _muxCS3 = ConfigPinMap::getPinFromName(confName.c_str());

        // Configure TMC2130s
        if (mcChip == "TMC2130")
        {
            for (int i = 0; i < MAX_TMC2130; i++)
            {
                // Set IHOLD=0x10, IRUN=0x10
                tmcWrite(i, WRITE_FLAG | REG_IHOLD_IRUN, 0x00001010UL);

                // Set native 256 microsteps, MRES=0, TBL=1=24, TOFF=8
                tmcWrite(i, WRITE_FLAG | REG_CHOPCONF, 0x00008008UL);
            }
        }
    }

    // Timer for TMC5072 continuous handling
    if ((mcChip == "TMC5072") && _isEnabled)
    {
        // Initialise chips
        uint64_t retVal1 = tmc5072Init(0);
        uint64_t retVal2 = tmc5072Init(1);

        // Start timer
        const esp_timer_create_args_t _timerArgs = {
                    &TrinamicsController::_staticTimerCb,
                    NULL,
                    esp_timer_dispatch_t::ESP_TIMER_TASK,
                    "Trinamic"
        };

        esp_timer_create(&_timerArgs, &_trinamicsTimerHandle);

        esp_timer_start_periodic(_trinamicsTimerHandle, 500000);

        Log.trace("%sTMC5072 Chip1Init %x,%x Chip2Init %x,%x Timer Started\n", MODULE_PREFIX,
                    (uint32_t)(retVal1 >> 32), (uint32_t)retVal1,
                    (uint32_t)(retVal2 >> 32), (uint32_t)retVal2);
        _trinamicsTimerStarted = true;
    }
}

void TrinamicsController::deinit()
{
    // Release pins
    if (_cs1 >= 0)
        pinMode(_cs1, INPUT);
    if (_cs2 >= 0)
        pinMode(_cs2, INPUT);
    if (_cs3 >= 0)
        pinMode(_cs3, INPUT);
    if (_mux1 >= 0)
        pinMode(_mux1, INPUT);
    if (_mux2 >= 0)
        pinMode(_mux2, INPUT);
    if (_mux3 >= 0)
        pinMode(_mux3, INPUT);
    if (_muxCS1 >= 0)
        pinMode(_muxCS1, INPUT);
    if (_muxCS2 >= 0)
        pinMode(_muxCS2, INPUT);
    if (_muxCS3 >= 0)
        pinMode(_muxCS3, INPUT);

    // Stop timer
    if (_trinamicsTimerStarted)
    {
        Log.trace("%sDe-init\n", MODULE_PREFIX);
        esp_timer_stop(_trinamicsTimerHandle);
        // xTimerStop(_trinamicsTimerHandle, 0);
        _trinamicsTimerStarted = false;
    }

    // Stop SPI
    if (_pVSPI)
    {
        _pVSPI->end();
    }

    _isEnabled = false;
}

uint64_t TrinamicsController::tmc5072Init(int chipIdx)
{
    Log.trace("%sTMC5072 Init %d cs1 %d\n", MODULE_PREFIX, chipIdx, _cs1);

    // 2 phase motor control
    uint64_t retVal = tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_CHOPCONF_1,0x00010135);      //CHOPCONF: TOFF=5, HSTRT=3, HEND=2, TBL=2, CHM=0 (spreadcycle)
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_IHOLD_IRUN_1,0x00070603);      //IHOLD_IRUN: IHOLD=3, IRUN=10 (max.current), IHOLDDELAY=6

    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_CHOPCONF_2,0x00010135);      //CHOPCONF: TOFF=5, HSTRT=3, HEND=2, TBL=2, CHM=0 (spreadcycle)
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_IHOLD_IRUN_2,0x00070603);      //IHOLD_IRUN: IHOLD=3, IRUN=10 (max.current), IHOLDDELAY=6

    // Reset positions
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_RAMPMODE_1,TMC5072_MODE_POSITION);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_XTARGET_1, 0);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_XACTUAL_1, 0);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_RAMPMODE_2, TMC5072_MODE_POSITION);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_XTARGET_2, 0);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_XACTUAL_2, 0);

    //Standard values for speed and acceleration
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_VSTART_1, 1);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_A1_1, 5000);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_V1_1, 26843);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_AMAX_1, 5000);   
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_VMAX_1, 100000);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_DMAX_1, 5000);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_D1_1, 5000);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_VSTOP_1, 10);

    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_VSTART_2, 1);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_A1_2, 5000);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_V1_2, 26843);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_AMAX_2, 5000);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_VMAX_2, 100000);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_DMAX_2, 5000);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_D1_2, 5000);
    tmcWrite(chipIdx, TMC5072_WRITE | TMC5072_VSTOP_2, 10);
    return retVal;
}

int TrinamicsController::getPinAndConfigure(const char* configJSON, const char* pinSelector, int direction, int initValue)
{
    String pinName = RdJson::getString(pinSelector, "", configJSON);
    int pinIdx = ConfigPinMap::getPinFromName(pinName.c_str());
    if (pinIdx >= 0)
    {
        pinMode(pinIdx, direction);
        digitalWrite(pinIdx, initValue);
    }
    return pinIdx;
}

void TrinamicsController::performSel(int singleCS, int mux1, int mux2, int mux3, int muxCS, bool en)
{
    if (singleCS >= 0)
    {
        digitalWrite(singleCS, !en);
    }
    else
    {
        if ((mux1 >= 0) && (mux2 >= 0) && (mux3 >= 0) && (muxCS >= 0))
        {
            digitalWrite(mux1, en ? (muxCS & 0x01) : 0);
            digitalWrite(mux2, en ? (muxCS & 0x02) : 0);
            digitalWrite(mux3, en ? (muxCS & 0x04) : 1);
        }
    }
}

void TrinamicsController::chipSel(int chipIdx, bool en)
{
    if (chipIdx == 0)
        performSel(_cs1, _mux1, _mux2, _mux3, _muxCS1, en);
    else if (chipIdx == 1)
        performSel(_cs2, _mux1, _mux2, _mux3, _muxCS2, en);
    else if (chipIdx == 2)
        performSel(_cs3, _mux1, _mux2, _mux3, _muxCS3, en);
}

uint64_t TrinamicsController::tmcWrite(int chipIdx, uint8_t cmd, uint32_t data)
{
    if (!_pVSPI)
        return 0;
        
    // Start SPI transaction
    _pVSPI->beginTransaction(SPISettings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE3));

    // Select chip
    chipSel(chipIdx, true);

    // Transfer data
    uint64_t retVal = _pVSPI->transfer(cmd);
    retVal <<= 8;
    retVal |= _pVSPI->transfer((data>>24) & 0xFF) & 0xFF;
    retVal <<= 8;
    retVal |= _pVSPI->transfer((data>>16) & 0xFF) & 0xFF;
    retVal <<= 8;
    retVal |= _pVSPI->transfer((data>>8) & 0xFF) & 0xFF;
    retVal <<= 8;
    retVal |= _pVSPI->transfer((data>>0) & 0xFF) & 0xFF;

    // Deselect chip and end transaction
    chipSel(chipIdx, false);
    _pVSPI->endTransaction();

    return retVal;    
}

uint8_t TrinamicsController::tmcRead(int chipIdx, uint8_t cmd, uint32_t* data)
{
    if (!_pVSPI)
        return 0;

    // Set read address
    tmcWrite(chipIdx, cmd, 0UL); 

    // Start SPI transaction
    _pVSPI->beginTransaction(SPISettings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE3));

    // Select chip
    chipSel(chipIdx, true);

    // Transfer data
    uint8_t retVal = _pVSPI->transfer(cmd);
    *data = _pVSPI->transfer(0x00) & 0xFF;
    *data <<= 8;
    *data |= _pVSPI->transfer(0x00) & 0xFF;
    *data <<= 8;
    *data |= _pVSPI->transfer(0x00) & 0xFF;
    *data <<= 8;
    *data |= _pVSPI->transfer(0x00) & 0xFF;

    // Deselect chip and end transaction
    chipSel(chipIdx, false);
    _pVSPI->endTransaction();
    return retVal;    
}

void TrinamicsController::process()
{
    // if (Utils::isTimeout(millis(), _debugTimerLast, 1000))
    // {
    //     // Show REG_GSTAT
    //     uint32_t data = 0;
    //     uint8_t retVal = tmcRead(0, REG_GSTAT, &data);
    //     Log.trace("%sGSTAT %02x Status %02x %s%s%s%s\n", MODULE_PREFIX, data, retVal, 
    //                 (retVal & 0x01) ? " reset" : "",
    //                 (retVal & 0x02) ? " error" : "",
    //                 (retVal & 0x03) ? " sg2" : "",
    //                 (retVal & 0x08) ? " standstill" : "");

    //     // Show REG_DRVSTATUS
    //     retVal = tmcRead(0, REG_DRVSTATUS, &data);
    //     Log.trace("%sDRVSTATUS %02x Status %02x %s%s%s%s\n", MODULE_PREFIX, data, retVal, 
    //                 (retVal & 0x01) ? " reset" : "",
    //                 (retVal & 0x02) ? " error" : "",
    //                 (retVal & 0x03) ? " sg2" : "",
    //                 (retVal & 0x08) ? " standstill" : "");

    //     _debugTimerLast = millis();
    // }
}

void TrinamicsController::_timerCallback(void* arg)
{
    Log.notice("tring\n");


    // TODO DEBUG ONLY
    digitalWrite(21, 0);
    if (_debugTimerLast)
    {
        tmcWrite(0, TMC5072_WRITE | TMC5072_XTARGET_1, 0x0007D000);  //XTARGET=512000 | 10 revolutions with micro step = 256
        tmcWrite(0, TMC5072_WRITE | TMC5072_XTARGET_2, 0xFFF83000);  //XTARGET=-512000 | 10 revolutions with micro step = 256
    }
    else
    {
        tmcWrite(0, TMC5072_WRITE | TMC5072_XTARGET_1, 0x00000000); //XTARGET=0
        tmcWrite(0, TMC5072_WRITE | TMC5072_XTARGET_2, 0x00000000); //XTARGET=0
    }

    _debugTimerLast = !_debugTimerLast;
}
