// RBotFirmware
// Rob Dobson 2016-19

#include "TrinamicsController.h"
#include "RdJson.h"
#include "ConfigPinMap.h"
#include "Utils.h"
#include "TMC5072_registers.h"

static const char* MODULE_PREFIX = "TrinamicsController: ";

TrinamicsController* TrinamicsController::_pThisObj = NULL;

TrinamicsController::TrinamicsController(AxesParams& axesParams, MotionPipeline& motionPipeline) :
        _axesParams(axesParams), _motionPipeline(motionPipeline)
{
    _pThisObj = this;
    _pVSPI = NULL;
    _isEnabled = false;
    _isRampGenerator = false;
    _trinamicsTimerStarted = false;
    _miso = _mosi = _sck = -1;
    _cs1 = _cs2 = _cs3 =  -1;
    _mux1 = _mux2 = _mux3 = -1;
    _muxCS1 = _muxCS2 = _muxCS3 = -1;
    _lastDoneNumberedCmdIdx = RobotConsts::NUMBERED_COMMAND_NONE;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
        _axisIdxToChipDriverIdx[i] = i;
    for (int i = 0; i < MAX_TMC5072*MAX_TMC_DRIVERS_PER_CHIP; i++)
        _chipDriverIdxToAxisIdx[i] = i;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
        _axisTargetSteps[i] = 0;
        _axisMinDistForNextBlockStart[i] = MIN_STEP_DIST_FOR_NEXT_BLOCK_START_DEFAULT;
    }
    resetTotalStepPosition();
}

TrinamicsController::~TrinamicsController()
{
    deinit();
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
    _cs1 = _cs2 = _cs3 =  -1;
    _mux1 = _mux2 = _mux3 = -1;
    _muxCS1 = _muxCS2 = _muxCS3 = -1;

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
    _isRampGenerator = false;
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
            if (!_pVSPI)
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
                tmcWrite(i, TMC2130_REG_IHOLD_IRUN, 0x00001010UL);

                // Set native 256 microsteps, MRES=0, TBL=1=24, TOFF=8
                tmcWrite(i, TMC2130_REG_CHOPCONF, 0x00008008UL);
            }
        }
    }

    // Check for ramp-generator chip
    if ((mcChip == "TMC5072") && _isEnabled)
    {
        // Initialise chips
        tmc5072Init();

        // Start timer for TMC5072 motion handling
        const esp_timer_create_args_t _timerArgs = {
                    &TrinamicsController::_staticTimerCb,
                    NULL,
                    esp_timer_dispatch_t::ESP_TIMER_TASK,
                    "Trinamic"
        };

        esp_timer_create(&_timerArgs, &_trinamicsTimerHandle);

        esp_timer_start_periodic(_trinamicsTimerHandle, TRINAMIC_TIMER_PERIOD_US);

        Log.trace("%sTMC5072 Timer Started\n", MODULE_PREFIX);
        _trinamicsTimerStarted = true;
        _isRampGenerator = true;
    }
}

uint32_t TrinamicsController::getUint32WithBaseFromConfig(const char* dataPath, uint32_t defaultValue,
                            const char* pSourceStr)
{
    String confStr = RdJson::getString(dataPath, "", pSourceStr);
    if (confStr.length() == 0)
        return defaultValue;
    if ((confStr.startsWith("0x")) || (confStr.startsWith("0X")))
        return strtoul(confStr.substring(2).c_str(), NULL, 16);
    else if ((confStr.startsWith("0b")) || (confStr.startsWith("0B")))
        return strtoul(confStr.substring(2).c_str(), NULL, 2);
    return strtoul(confStr.c_str(), NULL, 10);    
}

void TrinamicsController::configureAxis(int axisIdx, const char *axisJSON)
{
    if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
        return;

    // Get axis information
    _axisSettings[axisIdx].reversed = (RdJson::getLong("dirnRev", 0, axisJSON) != 0);

    // Driver settings
    _axisSettings[axisIdx].iRunPower = RdJson::getLong("IRUN", TMC_IRUN_DEFAULT, axisJSON);
    _axisSettings[axisIdx].iHoldPower = RdJson::getLong("IHOLD", TMC_IHOLD_DEFAULT, axisJSON);
    _axisSettings[axisIdx].iHoldDelay = RdJson::getLong("IHOLDDELAY", TMC_IHOLDDELAY_DEFAULT, axisJSON);
    _axisSettings[axisIdx].chopConf = getUint32WithBaseFromConfig("CHOPCONF", TMC_CHOPCONF_DEFAULT, axisJSON);

    // Get axis mapping
    int chipDriverIdx = RdJson::getLong("chipDriverIdx", -1, axisJSON);
    if ((chipDriverIdx != -1) && (chipDriverIdx >= 0) && (chipDriverIdx < MAX_TMC5072*MAX_TMC_DRIVERS_PER_CHIP))
    {
        _axisIdxToChipDriverIdx[axisIdx] = chipDriverIdx;
        _chipDriverIdxToAxisIdx[chipDriverIdx] = axisIdx;
    }

    // Debug
    Log.notice("%sAxis%d reversed %s chipDriverIdx %d\n", MODULE_PREFIX, axisIdx, _axisSettings[axisIdx].reversed ? "Y" : "N",
                    _axisIdxToChipDriverIdx[axisIdx]);

    // Reconfigure axis
    tmc5072Init();
}

// void TrinamicsController::clear()
// {
//     _lastDoneNumberedCmdIdx = RobotConsts::NUMBERED_COMMAND_NONE;
// }

void TrinamicsController::tmc5072Init()
{
    // Iterate axes (may have been re-mapped)
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        // Chopper configuration
        // CHOPCONF: TMC5072 datasheet pp 42
        uint32_t chopConfValue = _axisSettings[axisIdx].chopConf;
        tmc5072SendCmd(axisIdx, TMC5072_CHOPCONF, chopConfValue);

        // Motor currents IHOLD_IRUN: 0x00DDRRHH
        // DD = IHOLDDELAY Controls the number of clock cycles for motor power down after a motion as soon as TZEROWAIT
        //      has expired. The smooth transition avoids a motor jerk upon power down.
        //      0: instant power down
        //      1..15: Delay per current reduction step in multiple of 2^18 clocks
        // RR = IRUN Motor run current (0=1/32…31=32/32)
        //      Hint: Choose sense resistors in a way, that normal IRUN is 16 to 31 for best microstep performance
        // HH = IHOLD: Standstill current (0=1/32…31=32/32)
        //      In combination with stealthChop mode, setting IHOLD=0 allows to choose freewheeling or coil
        //      short circuit for motor stand still.

        uint32_t holdIRunValue = _axisSettings[axisIdx].iHoldDelay << 16;
        holdIRunValue |= _axisSettings[axisIdx].iRunPower << 8;
        holdIRunValue |= _axisSettings[axisIdx].iHoldPower;
        //  & 0x01f) ||
        //                          ((uint32_t)(_axisSettings[axisIdx].iRunPower & 0x01f) << 8) ||
        //                          ((uint32_t)(_axisSettings[axisIdx].iHoldDelay & 0x01f) << 16);
        tmc5072SendCmd(axisIdx, TMC5072_IHOLD_IRUN, holdIRunValue);

        Log.trace("%sTMC5072 Init axisIdx %d CHOPCONF %x IHOLDRUN %x iHoldPower %d iRunPower %d iHoldDelay %d\n", MODULE_PREFIX,
                    axisIdx, chopConfValue, holdIRunValue,
                    _axisSettings[axisIdx].iHoldPower,
                    _axisSettings[axisIdx].iRunPower,
                    _axisSettings[axisIdx].iHoldDelay);
    }

    for (int chipIdx = 0; chipIdx < MAX_TMC5072; chipIdx++)
    {
        // GCONF: 0x100 = driver 1 reversed, 0x200 = driver 2 reversed
        uint32_t gConfValue = 0;
        int axisIdx = _chipDriverIdxToAxisIdx[chipIdx*MAX_TMC_DRIVERS_PER_CHIP];
        if ((axisIdx >= 0) && (axisIdx < RobotConsts::MAX_AXES))
            gConfValue |= (_axisSettings[axisIdx].reversed ? 0x100 : 0);
        axisIdx = _chipDriverIdxToAxisIdx[chipIdx*MAX_TMC_DRIVERS_PER_CHIP+1];
        if ((axisIdx >= 0) && (axisIdx < RobotConsts::MAX_AXES))
            gConfValue |= (_axisSettings[axisIdx].reversed ? 0x200 : 0);
        uint64_t retVal = tmcWrite(chipIdx, TMC5072_GCONF, gConfValue);

        // Reset positions
        tmcWrite(chipIdx, TMC5072_RAMPMODE_1,TMC5072_MODE_POSITION);
        tmcWrite(chipIdx, TMC5072_XTARGET_1, 0);
        tmcWrite(chipIdx, TMC5072_XACTUAL_1, 0);
        tmcWrite(chipIdx, TMC5072_RAMPMODE_2, TMC5072_MODE_POSITION);
        tmcWrite(chipIdx, TMC5072_XTARGET_2, 0);
        tmcWrite(chipIdx, TMC5072_XACTUAL_2, 0);

        //Standard values for speed and acceleration
        tmcWrite(chipIdx, TMC5072_VSTART_1, 1);
        tmcWrite(chipIdx, TMC5072_A1_1, 5000);
        tmcWrite(chipIdx, TMC5072_V1_1, 0);
        tmcWrite(chipIdx, TMC5072_AMAX_1, 5000);   
        tmcWrite(chipIdx, TMC5072_VMAX_1, 10000);
        tmcWrite(chipIdx, TMC5072_DMAX_1, 5000);
        tmcWrite(chipIdx, TMC5072_D1_1, 5000);
        tmcWrite(chipIdx, TMC5072_VSTOP_1, 10);

        tmcWrite(chipIdx, TMC5072_VSTART_2, 1);
        tmcWrite(chipIdx, TMC5072_A1_2, 5000);
        tmcWrite(chipIdx, TMC5072_V1_2, 0);
        tmcWrite(chipIdx, TMC5072_AMAX_2, 5000);
        tmcWrite(chipIdx, TMC5072_VMAX_2, 100000);
        tmcWrite(chipIdx, TMC5072_DMAX_2, 5000);
        tmcWrite(chipIdx, TMC5072_D1_2, 5000);
        tmcWrite(chipIdx, TMC5072_VSTOP_2, 10);
        Log.trace("%sTMC5072 Chip%dInit retVal %x,%x GCONF %x\n", MODULE_PREFIX,
            chipIdx,
            (uint32_t)(retVal >> 32), (uint32_t)retVal,
            gConfValue);
    }
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

uint64_t TrinamicsController::tmcWrite(int chipIdx, uint8_t cmd, uint32_t data, bool addWriteFlag)
{
    if (!_pVSPI)
        return 0;
        
    // Start SPI transaction
    _pVSPI->beginTransaction(SPISettings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE3));

    // Select chip
    chipSel(chipIdx, true);

    // Transfer data
    uint64_t retVal = _pVSPI->transfer(cmd | (addWriteFlag ? TMC5072_WRITE : 0));
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

uint8_t TrinamicsController::tmcReadLastAndSetCmd(int chipIdx, uint8_t cmd, uint32_t& dataOut)
{
    dataOut = 0;
    if (!_pVSPI)
        return 0;

    // Start SPI transaction
    _pVSPI->beginTransaction(SPISettings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE3));

    // Select chip
    chipSel(chipIdx, true);

    // Transfer data
    uint8_t retVal = _pVSPI->transfer(cmd);
    dataOut = _pVSPI->transfer(0x00) & 0xFF;
    dataOut <<= 8;
    dataOut |= _pVSPI->transfer(0x00) & 0xFF;
    dataOut <<= 8;
    dataOut |= _pVSPI->transfer(0x00) & 0xFF;
    dataOut <<= 8;
    dataOut |= _pVSPI->transfer(0x00) & 0xFF;

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
    //     uint8_t retVal = tmcRead(0, REG_GSTAT, data);
    //     Log.trace("%sGSTAT %02x Status %02x %s%s%s%s\n", MODULE_PREFIX, data, retVal, 
    //                 (retVal & 0x01) ? " reset" : "",
    //                 (retVal & 0x02) ? " error" : "",
    //                 (retVal & 0x04) ? " sg2" : "",
    //                 (retVal & 0x08) ? " standstill" : "");

    //     // Show REG_DRVSTATUS
    //     retVal = tmcRead(0, REG_DRVSTATUS, data);
    //     Log.trace("%sDRVSTATUS %02x Status %02x %s%s%s%s\n", MODULE_PREFIX, data, retVal, 
    //                 (retVal & 0x01) ? " reset" : "",
    //                 (retVal & 0x02) ? " error" : "",
    //                 (retVal & 0x04) ? " sg2" : "",
    //                 (retVal & 0x08) ? " standstill" : "");

    //     _debugTimerLast = millis();
    // }
}

void TrinamicsController::updateStatus(int chipIdx)
{
    // Check if the chip is used based on whether the cs pin is defined
    int csPin = _cs1;
    if (chipIdx == 1)
        csPin = _cs2;
    if (csPin < 0)
        return;

    // Get status
    uint32_t rampStat1 = 0;
    uint32_t rampStat2 = 0;
    uint32_t steps1 = 0;
    uint32_t steps2 = 0;
    uint8_t tmcStatus = tmcReadLastAndSetCmd(chipIdx, TMC5072_RAMPSTAT_1, rampStat1);
    tmcReadLastAndSetCmd(chipIdx, TMC5072_RAMPSTAT_2, rampStat1);
    tmcReadLastAndSetCmd(chipIdx, TMC5072_XACTUAL_1, rampStat2);
    tmcReadLastAndSetCmd(chipIdx, TMC5072_XACTUAL_2, steps1);
    tmcReadLastAndSetCmd(chipIdx, TMC5072_XACTUAL_2, steps2);
    _tmc5072Status[chipIdx].set(tmcStatus, rampStat1, rampStat2, steps1, steps2);

    if (Utils::isTimeout(millis(), _debugTimerLast, 5000))
    {
        if (chipIdx == 0)
        {
            Log.trace("%sStatus chip%d Steps1 %d Steps2 %d Driver1 %s Driver2 %s%s\n", MODULE_PREFIX, 
                    chipIdx+1, 
                    steps1, steps2,
                    _tmc5072Status[chipIdx].getDriverStr(0).c_str(),
                    _tmc5072Status[chipIdx].getDriverStr(1).c_str(),
                    _tmc5072Status[chipIdx].getStatusStr().c_str());
            _debugTimerLast = millis();
        }
    }

    // Update total steps moved
    int axisIdx = _chipDriverIdxToAxisIdx[chipIdx*MAX_TMC_DRIVERS_PER_CHIP];
    if ((axisIdx >= 0) && (axisIdx < RobotConsts::MAX_AXES))
        _axisTotalSteps[axisIdx] = steps1;
    axisIdx = _chipDriverIdxToAxisIdx[chipIdx*MAX_TMC_DRIVERS_PER_CHIP+1];
    if ((axisIdx >= 0) && (axisIdx < RobotConsts::MAX_AXES))
        _axisTotalSteps[axisIdx] = steps2;

    // Check if we need to clear flags by reading GSTAT
    if (_tmc5072Status[chipIdx].isGstatClearNeeded())
    {
        uint32_t discard = 0;
        tmcReadLastAndSetCmd(chipIdx, TMC5072_GSTAT, discard);
    }

}

void TrinamicsController::tmc5072SendCmd(int axisIdx, uint8_t baseCmd, uint32_t data)
{
    int chipDriverIdx = _axisIdxToChipDriverIdx[axisIdx];
    int chipIdx = chipDriverIdx / MAX_TMC_DRIVERS_PER_CHIP;
    int driverIdx = chipDriverIdx % MAX_TMC_DRIVERS_PER_CHIP;
    if ((chipIdx < 0) || (chipIdx > MAX_TMC5072))
        return;
    uint8_t cmd = baseCmd;
    if (driverIdx == 0)
        cmd += TMC5072_MOTOR0;
    else
        cmd += TMC5072_MOTOR1;
    tmcWrite(chipIdx, cmd, data);

    // Debug
    // if (axisIdx == 0)
    //     Log.trace("C%d CMD %x %x\n", chipIdx, cmd, data);
}

bool TrinamicsController::isCloseToDestination()
{
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        // Log.trace("TEST AXIS %d %d %d %d\n", axisIdx, 
        //         _axisTargetSteps[axisIdx], _axisTotalSteps[axisIdx], _axisMinDistForNextBlockStart[axisIdx]);
        if (std::abs(_axisTargetSteps[axisIdx] - _axisTotalSteps[axisIdx]) > _axisMinDistForNextBlockStart[axisIdx])
        {
                return false;
        }
    }
    return true;
}

void TrinamicsController::_timerCallback(void* arg)
{

    // TODO DEBUG ONLY
    // digitalWrite(21, 0);
    // if (_debugTimerLast)
    // {
    //     tmcWrite(0, TMC5072_XTARGET_1, 0x0007D000);  //XTARGET=512000 | 10 revolutions with micro step = 256
    //     tmcWrite(0, TMC5072_XTARGET_2, 0xFFF83000);  //XTARGET=-512000 | 10 revolutions with micro step = 256
    // }
    // else
    // {
    //     tmcWrite(0, TMC5072_XTARGET_1, 0x00000000); //XTARGET=0
    //     tmcWrite(0, TMC5072_XTARGET_2, 0x00000000); //XTARGET=0
    // }

    // _debugTimerLast = !_debugTimerLast;

    // Log.notice("tring\n");


    // Log.notice("tring\n");

    // return;

    // Read status    
    bool isMoving = false;
    for (int i = 0; i < MAX_TMC5072; i++)
    {
        updateStatus(i);
        isMoving |= _tmc5072Status[i].anyAxisMoving();
    }

    // Peek a MotionPipelineElem from the queue
    MotionBlock *pBlock = _motionPipeline.peekGet();
    if (!pBlock)
    {
        // Log.trace("Nothing in pipe\n");
        return;
    }

    // Check if the element can be executed
    if (!pBlock->_canExecute)
    {
        // Log.trace("Can't execute\n");
        return;
    }

    // See if the block was already executing and set isExecuting if not
    bool blockIsNew = !pBlock->_isExecuting;
    pBlock->_isExecuting = true;

    // Existing block
    bool debugBlockCompleteCode = 0;
    if (!blockIsNew)
    {
        bool blockFinished = false;
        if (!isMoving)
        {
            // Log.trace("Removing\n");
            _motionPipeline.remove();
            blockFinished = true;
            debugBlockCompleteCode = 1;
            // Check if this is a numbered block - if so record its completion
            if (pBlock->getNumberedCommandIndex() != RobotConsts::NUMBERED_COMMAND_NONE)
                _lastDoneNumberedCmdIdx = pBlock->getNumberedCommandIndex();
        }
        else if (pBlock->getNumberedCommandIndex() == RobotConsts::NUMBERED_COMMAND_NONE)
        {
            // Check if close to destination
            if (isCloseToDestination())
            {
                // Remove block and 
                _motionPipeline.remove();
                blockFinished = true;
                debugBlockCompleteCode = 2;
            }
        }

        // Check if we finished a block and, if so, see if there's a new one waiting
        if (blockFinished)
        {
            // Peek a MotionPipelineElem from the queue
            // Check if the element can be executed
            MotionBlock *pBlock = _motionPipeline.peekGet();
            if (pBlock && pBlock->_canExecute)
            {
                // Should be new!
                blockIsNew = !pBlock->_isExecuting;
                pBlock->_isExecuting = true;
            }
        }
    }

    // New block
    if (blockIsNew)
    {
        // Handle the motion by requesting the controller to make the move
        int32_t maxAxisSteps = pBlock->_stepsTotalMaybeNeg[pBlock->_axisIdxWithMaxSteps];
        float entrySpeedFactor = fabs(pBlock->_entrySpeedMMps / pBlock->_feedrate);
        float exitSpeedFactor = fabs(pBlock->_exitSpeedMMps / pBlock->_feedrate);
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            // Set VMAX based on the distance each axis will travel
            uint32_t axisVMax = 100000 * abs(1000*pBlock->_stepsTotalMaybeNeg[axisIdx]/maxAxisSteps) / 1000;
            uint32_t axisVStart = axisVMax * entrySpeedFactor;
            tmc5072SendCmd(axisIdx, TMC5072_VSTART, axisVStart);
            tmc5072SendCmd(axisIdx, TMC5072_VMAX, axisVMax);
            uint32_t axisVStop = axisVMax * exitSpeedFactor;
            if (axisVStop < 10)
                axisVStop = 10;
            
            // Debug
            // if (axisIdx == 0)
            //     Log.trace("%F %F %F %F %F %d %d %d\n", pBlock->_entrySpeedMMps, pBlock->_exitSpeedMMps, pBlock->_feedrate, entrySpeedFactor, exitSpeedFactor,
            //         axisVMax, axisVStart, axisVStop);
            tmc5072SendCmd(axisIdx, TMC5072_VSTOP, axisVMax);
        }
        
        // Send commnands for each axis movement
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            _axisTargetSteps[axisIdx] = _axisTotalSteps[axisIdx] + pBlock->_stepsTotalMaybeNeg[axisIdx];
            tmc5072SendCmd(axisIdx, TMC5072_XTARGET, _axisTargetSteps[axisIdx]);

            // Log.trace("TARGET %d, %d, %d Total %d %d %d\n", 
            //     _axisTargetSteps[0], _axisTargetSteps[1], _axisTargetSteps[2], 
            //     _axisTotalSteps[0], _axisTotalSteps[1], _axisTotalSteps[2]);
        }
    }

    // Debug
    if (debugBlockCompleteCode != 0)
        Log.trace("BLOCK COMPLETE CODE %d Target %d, %d, %d Total %d %d %d\n", 
                debugBlockCompleteCode,
                _axisTargetSteps[0], _axisTargetSteps[1], _axisTargetSteps[2], 
                _axisTotalSteps[0], _axisTotalSteps[1], _axisTotalSteps[2]);
}
