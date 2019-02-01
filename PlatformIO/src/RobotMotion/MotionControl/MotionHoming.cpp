// RBotFirmware
// Rob Dobson 2016-18

#include "MotionHelper.h"
#include "MotionHoming.h"

static const char* MODULE_PREFIX = "MotionHoming: ";

MotionHoming::MotionHoming(MotionHelper *pMotionHelper)
{
    _pMotionHelper = pMotionHelper;
    _homingInProgress = false;
    _homingStrPos = 0;
    _commandInProgress = false;
    _isHomedOk = false;
    _maxHomingSecs = maxHomingSecs_default;
    _homeReqMillis = 0;
    _homingCurCommandIndex = homing_baseCommandIndex;
    _feedrateStepsPerSecForHoming = -1;
    _doCentring = false;
    _centringInProgress = false;
    _centringPhase = 0;
}

void MotionHoming::configure(const char *configJSON)
{
    // Sequence of commands for homing
    bool isValid = false;
    _homingSequence = RdJson::getString("homing/homingSeq", "", configJSON, isValid);
    if (!isValid)
        _homingSequence = "";
    // Max time homing
    _maxHomingSecs = RdJson::getLong("homing/maxHomingSecs", maxHomingSecs_default, configJSON);
    // No homing currently
    _homingStrPos = 0;
    _commandInProgress = false;
    _centringInProgress = false;
    Log.notice("%sconfig sequence %s\n", MODULE_PREFIX, _homingSequence.c_str());
}

bool MotionHoming::isHomingInProgress()
{
    return _homingInProgress;
}

bool MotionHoming::isHomedOk()
{
    return _isHomedOk;
}

void MotionHoming::homingStart(RobotCommandArgs &args)
{
    _axesToHome = args;
    _homingStrPos = 0;
    _homingInProgress = true;
    _commandInProgress = false;
    _isHomedOk = false;
    _centringInProgress = false;
    _doCentring = false;
    _homeReqMillis = millis();
    _feedrateStepsPerSecForHoming = -1;
    RobotCommandArgs curStatus;
    _pMotionHelper->getCurStatus(curStatus);
    _homingStartSteps = curStatus.getPointSteps();
    Log.notice("%sstart, seq = %s\n", MODULE_PREFIX, _homingSequence.c_str());
}

void MotionHoming::service(AxesParams &axesParams)
{
    // Check if active
    if (!_homingInProgress)
    {
        return;
    }

    // Progress homing if possible
    if (_commandInProgress)
    {
        // Check if a command has completed execution
        if (getLastCompletedNumberedCmdIdx() != _homingCurCommandIndex)
            return;
        debugShowSteps("Command completed");
        _commandInProgress = false;
    }

    // Check for timeout
    if (millis() > _homeReqMillis + (_maxHomingSecs * 1000))
    {
        debugShowSteps("Timed Out");
        _isHomedOk = false;
        _homingInProgress = false;
        _commandInProgress = false;
        return;
    }

    // Check if centring in progress
    if (_centringInProgress)
    {
        if (nextCentringOperation())
            return;
        _centringInProgress = false;
    }

    // Need to start a command or finish
    _curCommand.clear();
    String debugCmdStr;
    bool cmdValid = extractAndExecNextCmd(axesParams, debugCmdStr);
    if (!cmdValid)
    {
        // Done
        _isHomedOk = false;
        _homingInProgress = false;
        _commandInProgress = false;
    }
}

bool MotionHoming::getInteger(unsigned int &homingStrPos, int &retInt)
{
    // Check for distance
    String distStr = "";
    while (true)
    {
        if (isdigit(_homingSequence.charAt(homingStrPos)) || _homingSequence.charAt(homingStrPos) == '.' || 
                _homingSequence.charAt(homingStrPos) == '+' || _homingSequence.charAt(homingStrPos) == '-')
        {
            distStr += _homingSequence.charAt(homingStrPos);
            homingStrPos++;
        }
        else
        {
            break;
        }
    }
    if (distStr.length() > 0)
    {
        retInt = distStr.toInt();
        return true;
    }
    return false;
}

int MotionHoming::getFeedrate(unsigned int &homingStrPos, AxesParams &axesParams, int axisIdx, int defaultValue)
{
    // Initially set to max rate
    int feedrateStepsPerSec = defaultValue;
    switch(_homingSequence.charAt(homingStrPos))
    {
        case 'R':
        case 'r':
        {
            homingStrPos++;
            int newFeedrate = 0;
            if (getInteger(homingStrPos, newFeedrate))
            {
                if (newFeedrate > 0)
                    feedrateStepsPerSec = newFeedrate * axesParams.getStepsPerRot(axisIdx) / 60;
            }
            break;
        }
        case 'S':
        case 's':
        {
            homingStrPos++;
            int newFeedrate = 0;
            if (getInteger(homingStrPos, newFeedrate))
            {
                if (newFeedrate > 0)
                    feedrateStepsPerSec = newFeedrate;
            }
            break;
        }
    }
    return feedrateStepsPerSec;
}

void MotionHoming::setCentring(unsigned int &homingStrPos, int axisIdx)
{
    if ((_homingSequence.charAt(homingStrPos) == 'Q') || (_homingSequence.charAt(homingStrPos) == 'q'))
    {
        _doCentring = true;
        homingStrPos++;
    }
}

void MotionHoming::setEndstops(unsigned int &homingStrPos, int axisIdx)
{
    // Endstops
    // Log.notice("%stestingCh %c, speed %F, dist %F\n", MODULE_PREFIX, _homingSequence.charAt(_homingStrPos),
    //             _curCommand.getFeedrate(), _curCommand.getValMM(axisIdx));
    int endStopIdx = 0;
    bool checkActive = false;
    bool setEndstopTest = false;
    if ((_homingSequence.charAt(homingStrPos) == 'X') || (_homingSequence.charAt(homingStrPos) == 'x') ||
        (_homingSequence.charAt(homingStrPos) == 'N') || (_homingSequence.charAt(homingStrPos) == 'n'))
    {
        endStopIdx = ((_homingSequence.charAt(homingStrPos) == 'N') || (_homingSequence.charAt(homingStrPos) == 'n')) ? 0 : 1;
        checkActive = (_homingSequence.charAt(homingStrPos) == 'X') || (_homingSequence.charAt(homingStrPos) == 'N');
        homingStrPos++;
        setEndstopTest = true;
    }
    // Check axis should be homed
    if (setEndstopTest && (!_axesToHome.isValid(axisIdx)))
    {
        Log.DBG_HOMING_LVL("%sAxis%d in sequence but not required to home\n", MODULE_PREFIX, axisIdx);
        return;
    }
    // Check endStop
    if (setEndstopTest)
        _curCommand.setTestEndStop(axisIdx, endStopIdx, checkActive ? AxisMinMaxBools::END_STOP_HIT : AxisMinMaxBools::END_STOP_NOT_HIT);
}

void MotionHoming::startCentringOperation()
{
    // Start the centring process
    _centringInProgress = true;
    _centringPhase = 0;
    RobotCommandArgs curStatus;
    _pMotionHelper->getCurStatus(curStatus);
    _centringSteps[_centringPhase++] = curStatus.getPointSteps();
    // Process first part of centring
    processHomingCommand(_curCommand);
}

bool MotionHoming::nextCentringOperation()
{
    Log.DBG_HOMING_LVL("%s cur phase %i\n", MODULE_PREFIX, _centringPhase); 

    // Record current position
    if (_centringPhase >= NUM_CENTRING_PHASES)
        return false;
    RobotCommandArgs curStatus;
    _pMotionHelper->getCurStatus(curStatus);
    _centringSteps[_centringPhase] = curStatus.getPointSteps();
    // Check which phase of centring we're in
    if (_centringPhase == 1)
    {
        // Reverse direction of travel
        _centringReversedCommand = _curCommand;
        _centringReversedCommand.reverseStepDirection();
        // Reverse endstop checks
        _centringReversedCommand.reverseEndstopChecks();
        // Execute
        processHomingCommand(_centringReversedCommand);
    }
    else if (_centringPhase == 2)
    {
        // Reverse sense of endstop checks again
        _centringReversedCommand.reverseEndstopChecks();
        // Execute
        processHomingCommand(_centringReversedCommand);
    }
    else if (_centringPhase == 3)
    {
        // Reverse sense of endstop checks for original command
        _curCommand.reverseEndstopChecks();
        // Execute
        processHomingCommand(_curCommand);
    }
    else if (_centringPhase == 4)
    {
        // Remove endstop checks for original command
        _curCommand.setTestNoEndStops();
        // Calculate mid point to step to
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            if (_curCommand.isValid(axisIdx))
                _curCommand.setAxisSteps(axisIdx, 
                            (_centringSteps[2].getVal(axisIdx) + _centringSteps[3].getVal(axisIdx)) / 2 - _centringSteps[4].getVal(axisIdx),
                            true);
        }
        // Execute
        processHomingCommand(_curCommand);
    }
    else if (_centringPhase == 5)
    {
        // Now look at results
        for (int i = 0; i < NUM_CENTRING_PHASES; i++)
        {
            Log.DBG_HOMING_LVL("%sphase %i %d %d %d\n", MODULE_PREFIX, 
                        i, _centringSteps[i].getVal(0), _centringSteps[i].getVal(1), _centringSteps[i].getVal(2));
        }
        return false;
    }
    
    // Bump phase
    _centringPhase++;
    return true;
}

void MotionHoming::processHomingCommand(RobotCommandArgs& commandArgs)
{
    // Allow out of bounds movement while homing
    commandArgs.setAllowOutOfBounds(true);
    // Don't split the move
    commandArgs.setDontSplitMove(true);
    // Set the command index so we know when complete
    _homingCurCommandIndex++;
    commandArgs.setNumberedCommandIndex(_homingCurCommandIndex);
    // Command complete so exec
    _commandInProgress = true;
    moveTo(commandArgs);
    Log.DBG_HOMING_LVL("%sexec command %s\n", MODULE_PREFIX, commandArgs.toJSON().c_str());
}

bool MotionHoming::extractAndExecNextCmd(AxesParams &axesParams, String& debugCmdStr)
{
    while (_homingStrPos < _homingSequence.length())
    {
        // Get axis
        int ch = _homingSequence.charAt(_homingStrPos);
        switch (ch)
        {
            case '$': // All done ok
            {
                // Check if homing commands complete
                Log.notice("%sHomed ok\n", MODULE_PREFIX);
                _isHomedOk = true;
                _homingInProgress = false;
                _commandInProgress = false;
                _homingStrPos++;
                _curCommand.setHasHomed(true);
                debugCmdStr = "Done";
                return true;
            }
            case '#': // Process command
            {
                // Handle the start of a centring operation
                if (_doCentring)
                    startCentringOperation();
                else
                    processHomingCommand(_curCommand);
                _homingStrPos++;
                _doCentring = false;
                return true;
            }
            case 'A':
            case 'a': // Actuators
            case 'B':
            case 'b':
            case 'C':
            case 'c':
            {
                // We're homing
                _curCommand.setIsHoming(true);
                // Axis index
                int axisIdx = 0;
                if ((ch == 'B') || (ch == 'b'))
                    axisIdx = 1;
                else if ((ch == 'C') || (ch == 'c'))
                    axisIdx = 2;
                // All homing is relative
                _curCommand.setMoveType(RobotMoveTypeArg_Relative);
                // Get the instruction
                _homingStrPos++;
                // See if we have an numeric value next
                int stepsToMove = AxisParams::stepsForAxisHoming_default;
                if (getInteger(_homingStrPos, stepsToMove))
                {
                    // Set dist to move
                    if (_axesToHome.isValid(axisIdx))
                        _curCommand.setAxisSteps(axisIdx, stepsToMove, true);
                    // Set feedrate to either max steps per second for this axis or a setting from the command string
                    int feedrateStepsPerSec = getFeedrate(_homingStrPos, axesParams, axisIdx, 
                                (_feedrateStepsPerSecForHoming == -1) ? axesParams.getMaxStepRatePerSec(axisIdx) : _feedrateStepsPerSecForHoming);
                    _curCommand.setFeedrate(feedrateStepsPerSec);
                    // Check for centring mode
                    setCentring(_homingStrPos, axisIdx);
                    // Set endstop tests
                    setEndstops(_homingStrPos, axisIdx);
                    debugCmdStr = "Move";
                }
                // Maybe this is saying the axis is now home
                else if (_homingSequence[_homingStrPos] == '=')
                {
                    _homingStrPos++;
                    if ((_homingSequence.charAt(_homingStrPos) == 'H') || (_homingSequence.charAt(_homingStrPos) == 'h'))
                    {
                        setAtHomePos(axisIdx);
                        _homingStrPos++;
                        Log.notice("%sSetting at home for axis %d\n", MODULE_PREFIX, axisIdx);
                        debugCmdStr = "Home";
                    }
                    break;
                }
                break;
            }
            case 'F':
            case 'f':
            {
                // Feedrate for whole homing (unless specifically overridden)
                _homingStrPos++;
                int feedrateStepsPerSec = getFeedrate(_homingStrPos, axesParams, 0, axesParams.getMaxStepRatePerSec(0));
                if (feedrateStepsPerSec > 0)
                {
                    Log.trace("%sFeedrate set to %d steps per sec\n", MODULE_PREFIX, feedrateStepsPerSec);
                    _feedrateStepsPerSecForHoming = feedrateStepsPerSec;
                }
            }
            default:
            {
                // Skip other chars (including ;)
                _homingStrPos++;
                break;
            }
        }
    }
    debugCmdStr = "";
    return false;
}

void MotionHoming::moveTo(RobotCommandArgs &args)
{
    _pMotionHelper->moveTo(args);
}

int MotionHoming::getLastCompletedNumberedCmdIdx()
{
    return _pMotionHelper->getLastCompletedNumberedCmdIdx();
}

void MotionHoming::setAtHomePos(int axisIdx)
{
    _pMotionHelper->setCurPositionAsHome(axisIdx);
}

void MotionHoming::debugShowSteps(const char* debugMsg)
{
    RobotCommandArgs curStatus;
    _pMotionHelper->getCurStatus(curStatus);
    AxisInt32s curSteps = curStatus.getPointSteps();
    Log.DBG_HOMING_LVL("%s%s stepFromHomingStart %d %d %d\n", MODULE_PREFIX, 
            debugMsg, 
            curSteps.getVal(0) - _homingStartSteps.getVal(0),
            curSteps.getVal(1) - _homingStartSteps.getVal(1),
            curSteps.getVal(2) - _homingStartSteps.getVal(2));
}
