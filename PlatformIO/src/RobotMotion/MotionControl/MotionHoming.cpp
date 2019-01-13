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
    Log.notice("%sconfig sequence %s\n", MODULE_PREFIX, _homingSequence.c_str());
}

bool MotionHoming::isHomingInProgress()
{
    return _homingInProgress;
}

void MotionHoming::homingStart(RobotCommandArgs &args)
{
    _axesToHome = args;
    _homingStrPos = 0;
    _homingInProgress = true;
    _commandInProgress = false;
    _isHomedOk = false;
    _homeReqMillis = millis();
    _feedrateStepsPerSecForHoming = -1;
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
    }

    // Check for timeout
    if (millis() > _homeReqMillis + (_maxHomingSecs * 1000))
    {
        Log.warning("%sTimed Out\n", MODULE_PREFIX);
        _isHomedOk = false;
        _homingInProgress = false;
        _commandInProgress = false;
        return;
    }

    // Need to start a command or finish
    _curCommand.clear();
    String debugCmdStr;
    bool cmdValid = extractAndExecNextCmd(axesParams, debugCmdStr);
    if (!cmdValid)
    {
        // Done
        _homingInProgress = false;
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
                // Allow out of bounds movement while homing
                _curCommand.setAllowOutOfBounds(true);
                // Don't split the move
                _curCommand.setDontSplitMove(true);
                // Set the command index so we know when complete
                _homingCurCommandIndex++;
                _curCommand.setNumberedCommandIndex(_homingCurCommandIndex);
                // Command complete so exec
                _commandInProgress = true;
                moveTo(_curCommand);
                Log.DBG_HOMING_LVL("%sexec command %s\n", MODULE_PREFIX, _curCommand.toJSON().c_str());
                _homingStrPos++;
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
