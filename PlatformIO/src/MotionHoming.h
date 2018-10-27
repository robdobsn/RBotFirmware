// RBotFirmware
// Rob Dobson 2016-18

#pragma once

class MotionHelper;

#define DBG_HOMING_LVL notice

class MotionHoming
{
  public:
    static constexpr int maxHomingSecs_default = 1000;
    static constexpr int homing_baseCommandIndex = 10000;

    bool _isHomedOk;
    String _homingSequence;
    bool _homingInProgress;
    RobotCommandArgs _axesToHome;
    unsigned int _homingStrPos;
    bool _commandInProgress;
    RobotCommandArgs _curCommand;
    int _maxHomingSecs;
    unsigned long _homeReqMillis;
    MotionHelper *_pMotionHelper;
    int _homingCurCommandIndex;

    MotionHoming(MotionHelper *pMotionHelper)
    {
        _pMotionHelper = pMotionHelper;
        _homingInProgress = false;
        _homingStrPos = 0;
        _commandInProgress = false;
        _isHomedOk = false;
        _maxHomingSecs = maxHomingSecs_default;
        _homeReqMillis = 0;
        _homingCurCommandIndex = homing_baseCommandIndex;
    }

    void configure(const char *configJSON)
    {
        // Sequence of commands for homing
        bool isValid = false;
        _homingSequence = RdJson::getString("homingSeq", "", configJSON, isValid);
        if (!isValid)
            _homingSequence = "";
        // Max time homing
        _maxHomingSecs = RdJson::getLong("maxHomingSecs", maxHomingSecs_default, configJSON);
        // No homing currently
        _homingStrPos = 0;
        _commandInProgress = false;
        Log.notice("MotionHoming: config sequence %s\n", _homingSequence.c_str());
    }

    bool isHomingInProgress()
    {
        return _homingInProgress;
    }

    void homingStart(RobotCommandArgs &args)
    {
        _axesToHome = args;
        _homingStrPos = 0;
        _homingInProgress = true;
        _commandInProgress = false;
        _isHomedOk = false;
        _homeReqMillis = millis();
        Log.notice("MotionHoming: start, seq = %s\n", _homingSequence.c_str());
    }

    void service(AxesParams &axesParams)
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
            Log.warning("MotionHoming: Timed Out\n");
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

    bool extractAndExecNextCmd(AxesParams &axesParams)
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
                Log.DBG_HOMING_LVL("MotionHoming: command in prog %d, len %d", _homingStrPos, _homingSequence.length());
                Log.notice("MotionHoming: Homed ok\n");
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
                Log.DBG_HOMING_LVL("MotionHoming: exec command %s\n", _curCommand.toJSON().c_str());
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
                {
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
                    // Endstops
                    // Log.notice("MotionHoming: testingCh %c, speed %F, dist %F\n", _homingSequence.charAt(_homingStrPos),
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
                        Log.DBG_HOMING_LVL("MotionHoming: Axis%d in sequence but not required to home\n", axisIdx);
                        continue;
                    }
                    // Check endStop
                    if (setEndstopTest)
                    {
                        Log.DBG_HOMING_LVL("MotionHoming: Axis%d steps %d, rate %F, stop at endstop%d=%s\n", 
                                axisIdx, distToMove, _curCommand.getFeedrate(),
                                endStopIdx, (checkActive ? "Hit" : "NotHit"));
                        _curCommand.setTestEndStop(axisIdx, endStopIdx, checkActive ? AxisMinMaxBools::END_STOP_HIT : AxisMinMaxBools::END_STOP_NOT_HIT);
                    }
                    else
                    {
                        Log.DBG_HOMING_LVL("MotionHoming: Axis%d steps %d, rate %F, no enstop check\n", 
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
                    Log.notice("MotionHoming: Setting at home for axis %d\n", axisIdx);
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

    void moveTo(RobotCommandArgs &args);
    int getLastCompletedNumberedCmdIdx();
    void setAtHomePos(int axisIdx);
};
