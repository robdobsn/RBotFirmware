// RBotFirmware
// Rob Dobson 2016-18

#include "MotionHelper.h"
#include "MotionHoming.h"

static const char* MODULE_PREFIX = "MotionHoming: ";

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
    bool cmdValid = extractAndExecNextCmd(axesParams);
    if (!cmdValid)
    {
        // Done
        _homingInProgress = false;
    }
}

bool MotionHoming::extractAndExecNextCmd(AxesParams &axesParams)
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
            // Axis index
            int axisIdx = 0;
            if ((ch == 'B') || (ch == 'b'))
                axisIdx = 1;
            else if ((ch == 'C') || (ch == 'c'))
                axisIdx = 2;
            // Get the instruction
            _homingStrPos++;
            int ch2 = _homingSequence.charAt(_homingStrPos);
            switch (ch2)
            {
            case '+':
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                // Move beyond sign if present
                if (!isDigit(ch2))
                    _homingStrPos++;
                _curCommand.setMoveType(RobotMoveTypeArg_Relative);
                // Handle direction
                int32_t distToMove = axesParams.getAxisMaxSteps(axisIdx);
                if (ch2 == '-')
                    distToMove = -distToMove;
                // Check for distance
                String distStr = "";
                while (true)
                {
                    if (isdigit(_homingSequence.charAt(_homingStrPos)) || _homingSequence.charAt(_homingStrPos) == '.')
                    {
                        distStr += _homingSequence.charAt(_homingStrPos);
                        _homingStrPos++;
                    }
                    else
                    {
                        break;
                    }
                }
                if (distStr.length() > 0)
                {
                    distToMove = distStr.toInt();
                    if (ch2 == '-')
                        distToMove = -distToMove;
                }
                // Feedrate
                _curCommand.setFeedrate(axesParams.getMaxSpeed(axisIdx));
                // Check for speed
                if ((_homingSequence.charAt(_homingStrPos) == 'R') || (_homingSequence.charAt(_homingStrPos) == 'r'))
                {
                    _homingStrPos++;
                    if ((_homingSequence.charAt(_homingStrPos) >= '1') && (_homingSequence.charAt(_homingStrPos) <= '9'))
                    {
                        int mult = _homingSequence.charAt(_homingStrPos) - '0' + 1;
                        _curCommand.setFeedrate(axesParams.getMaxSpeed(axisIdx) * mult / 10);
                        _homingStrPos++;
                    }
                }
                // "S" sets the speed directly to a multiple of 5
                else if ((_homingSequence.charAt(_homingStrPos) == 'S') || (_homingSequence.charAt(_homingStrPos) == 's'))
                {
                    _homingStrPos++;
                    if ((_homingSequence.charAt(_homingStrPos) >= '1') && (_homingSequence.charAt(_homingStrPos) <= '9'))
                    {
                        int mult = _homingSequence.charAt(_homingStrPos) - '0' + 1;
                        _curCommand.setFeedrate(mult * 5);
                        _homingStrPos++;
                    }
                }
                // Endstops
                // Log.notice("%stestingCh %c, speed %F, dist %F\n", MODULE_PREFIX, _homingSequence.charAt(_homingStrPos),
                //             _curCommand.getFeedrate(), _curCommand.getValMM(axisIdx));
                int endStopIdx = 0;
                bool checkActive = false;
                bool setEndstopTest = false;
                if ((_homingSequence.charAt(_homingStrPos) == 'X') || (_homingSequence.charAt(_homingStrPos) == 'x') ||
                    (_homingSequence.charAt(_homingStrPos) == 'N') || (_homingSequence.charAt(_homingStrPos) == 'n'))
                {
                    endStopIdx = ((_homingSequence.charAt(_homingStrPos) == 'N') || (_homingSequence.charAt(_homingStrPos) == 'n')) ? 0 : 1;
                    checkActive = (_homingSequence.charAt(_homingStrPos) == 'X') || (_homingSequence.charAt(_homingStrPos) == 'N');
                    _homingStrPos++;
                    setEndstopTest = true;
                }
                // Check axis should be homed
                if (!_axesToHome.isValid(axisIdx))
                {
                    Log.DBG_HOMING_LVL("%sAxis%d in sequence but not required to home\n", MODULE_PREFIX, axisIdx);
                    continue;
                }
                // Check endStop
                if (setEndstopTest)
                {
                    Log.DBG_HOMING_LVL("%sAxis%d steps %d, rate %F, stop at endstop%d=%s\n", MODULE_PREFIX, 
                            axisIdx, distToMove, _curCommand.getFeedrate(),
                            endStopIdx, (checkActive ? "Hit" : "NotHit"));
                    _curCommand.setTestEndStop(axisIdx, endStopIdx, checkActive ? AxisMinMaxBools::END_STOP_HIT : AxisMinMaxBools::END_STOP_NOT_HIT);
                }
                else
                {
                    Log.DBG_HOMING_LVL("%sAxis%d steps %d, rate %F, no enstop check\n", MODULE_PREFIX, 
                            axisIdx, distToMove, _curCommand.getFeedrate());

                }
                // Set dist to move
                _curCommand.setAxisSteps(axisIdx, distToMove, true);
                break;
            }
            case '=':
            {
                _homingStrPos++;
                if ((_homingSequence.charAt(_homingStrPos) == 'H') || (_homingSequence.charAt(_homingStrPos) == 'h'))
                {
                    setAtHomePos(axisIdx);
                    _homingStrPos++;
                }
                Log.notice("%sSetting at home for axis %d\n", MODULE_PREFIX, axisIdx);
                break;
            }
            }
            break;
        }
        case ';':
        {
            // Skip semicolon
            _homingStrPos++;
            break;
        }
        default:
        {
            // Skip other chars
            _homingStrPos++;
            break;
        }
        }
    }
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
