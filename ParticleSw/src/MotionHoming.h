// RBotFirmware
// Rob Dobson 2016-18

#pragma once

class MotionHelper;

class MotionHoming
{
public:
  static constexpr int maxHomingSecs_default = 1000;
  static constexpr int homing_baseCommandIndex = 10000;

  bool _isHomedOk;
  String _homingSequence;
  bool _homingInProgress;
  RobotCommandArgs _axesToHome;
  int _homingStrPos;
  bool _commandInProgress;
  RobotCommandArgs _curCommand;
  int _maxHomingSecs;
  unsigned long _homeReqMillis;
  MotionHelper* _pMotionHelper;
  int _homingCurCommandIndex;

  MotionHoming(MotionHelper* pMotionHelper)
  {
    _pMotionHelper = pMotionHelper;
    _homingInProgress  = false;
    _homingStrPos      = 0;
    _commandInProgress = false;
    _isHomedOk         = false;
    _maxHomingSecs     = maxHomingSecs_default;
    _homeReqMillis     = 0;
    _homingCurCommandIndex = homing_baseCommandIndex;
  }

  void configure(const char* configJSON)
  {
    // Sequence of commands for homing
    bool isValid = false;
    _homingSequence = RdJson::getString("homingSeq", "", configJSON, isValid);
    if (!isValid)
      _homingSequence = "";
    // Max time homing
    _maxHomingSecs = RdJson::getLong("maxHomingSecs", maxHomingSecs_default, configJSON);
    // No homing currently
    _homingStrPos      = 0;
    _commandInProgress = false;
    Log.info("MotionHoming: sequence %s", _homingSequence.c_str());
  }

  bool isHomingInProgress()
  {
    return _homingInProgress;
  }

  void homingStart(RobotCommandArgs& args)
  {
    _axesToHome        = args;
    _homingStrPos      = 0;
    _homingInProgress  = true;
    _commandInProgress = false;
    _isHomedOk         = false;
    _homeReqMillis     = millis();
    Log.info("MotionHoming: start, seq = %s", _homingSequence.c_str());
  }

  void service(AxesParams& axesParams)
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
      // Log.info("LastCompleted %d, %d", getLastCompletedNumberedCmdIdx(), _homingCurCommandIndex);
      if (getLastCompletedNumberedCmdIdx() != _homingCurCommandIndex)
        return;
    }

    // Check for timeout
    if (millis() > _homeReqMillis + (_maxHomingSecs * 1000))
    {
      Log.info("MotionHoming: Timed Out");
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

  bool extractAndExecNextCmd(AxesParams& axesParams)
  {
    while (_homingStrPos < _homingSequence.length())
    {
      // Get axis
      int axisIdx = 0;
      int ch      = _homingSequence.charAt(_homingStrPos);
      switch (ch)
      {
        case '$':
        {
          // Check if homing commands complete
          // Log.info("MotionHoming: command in prog %d, %d", _homingStrPos, _homingSequence.length());
          _homingStrPos++;
          Log.info("MotionHoming: Homed ok");
          _isHomedOk = true;
          _homingInProgress = false;
          _commandInProgress = false;
          return true;
        }
        case 'X': case 'x':
        case 'Y': case 'y':
        case 'Z': case 'z':
        {
          // Axis index
          int axisIdx = 0;
          if ((ch == 'Y') || (ch == 'y'))
            axisIdx = 1;
          else if ((ch == 'Z') || (ch == 'z'))
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
              float distToMove = axesParams.getAxisMaxRange(axisIdx);
              if (ch2 == '-')
                distToMove = -distToMove;
              // Check for distance
              String distStr = "";
              while(true)
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
              Log.info("Diststr %s", distStr.c_str());
              if (distStr.length() > 0)
              {
                distToMove = distStr.toFloat();
                if (ch2 == '-')
                  distToMove = -distToMove;
              }
              // Set dist to move
              _curCommand.setAxisValMM(axisIdx, distToMove, true);
              _curCommand.setFeedrate(axesParams.getMaxSpeed(axisIdx) / 10);
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
              // Log.info("MotionHoming: testingCh %c, speed %0.3f, dist %0.3f", _homingSequence.charAt(_homingStrPos),
              //             _curCommand.getFeedrate(), _curCommand.getValMM(axisIdx));
              if ((_homingSequence.charAt(_homingStrPos) == 'S') || (_homingSequence.charAt(_homingStrPos) == 's'))
              {
                _homingStrPos++;
                int endStopIdx = 0;
                if ((_homingSequence.charAt(_homingStrPos) == 'X') || (_homingSequence.charAt(_homingStrPos) == 'x'))
                  endStopIdx = 1;
                _homingStrPos++;
                bool checkActive = true;
                if ((_homingSequence.charAt(_homingStrPos) == '!'))
                {
                  _homingStrPos++;
                  checkActive = false;
                }
                // Log.info("Setting endstop %d, %d, %d",axisIdx, endStopIdx, checkActive ? AxisMinMaxBools::END_TEST_HIT : AxisMinMaxBools::END_TEST_NOT_HIT);
                _curCommand.setTestEndStop(axisIdx, endStopIdx, checkActive ? AxisMinMaxBools::END_TEST_HIT : AxisMinMaxBools::END_TEST_NOT_HIT);
              }
              // Check axis should be homed
              if (!_axesToHome.isValid(axisIdx))
              {
                Log.info("MotionHoming: axis %d in sequence but not required to home", axisIdx);
                continue;
              }
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
              Log.info("MotionHoming: command %s", _curCommand.toJSON().c_str());
              return true;
            }
            case '=':
            {
              _homingStrPos++;
              if ((_homingSequence.charAt(_homingStrPos) == 'H') || (_homingSequence.charAt(_homingStrPos) == 'h'))
              {
                setAtHomePos(axisIdx);
                _homingStrPos++;
              }
              Log.info("MotionHoming: Setting at home for axis %d", axisIdx);
              return true;
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

  void moveTo(RobotCommandArgs& args);
  int getLastCompletedNumberedCmdIdx();
  void setAtHomePos(int axisIdx);
};
