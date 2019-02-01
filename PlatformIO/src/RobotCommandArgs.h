// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "AxisValues.h"

enum RobotMoveTypeArg
{
    RobotMoveTypeArg_None,
    RobotMoveTypeArg_Absolute,
    RobotMoveTypeArg_Relative
};

class RobotCommandArgs
{
private:
    // Flags
    bool _ptUnitsSteps : 1;
    bool _dontSplitMove : 1;
    bool _extrudeValid : 1;
    bool _feedrateValid : 1;
    bool _moveClockwise : 1;
    bool _moveRapid : 1;
    bool _allowOutOfBounds : 1;
    bool _pause : 1;
    bool _moreMovesComing : 1;
    bool _isHoming: 1;
    bool _hasHomed: 1;
    // Command control
    int _queuedCommands;
    int _numberedCommandIndex;
    // Coords etc
    AxisFloats _ptInMM;
    AxisFloats _ptInCoordUnits;
    AxisInt32s _ptInSteps;
    float _extrudeValue;
    float _feedrateValue;
    RobotMoveTypeArg _moveType;
    AxisMinMaxBools _endstops;

public:
    RobotCommandArgs()
    {
        clear();
    }
    void clear()
    {
        // Flags
        _ptUnitsSteps = false;
        _dontSplitMove = false;
        _extrudeValid = false;
        _feedrateValid = false;
        _moveClockwise = false;
        _moveRapid = false;
        _allowOutOfBounds = false;
        _pause = false;
        _moreMovesComing = false;
        _isHoming = false;
        _hasHomed = false;
        // Command control
        _queuedCommands = 0;
        _numberedCommandIndex = RobotConsts::NUMBERED_COMMAND_NONE;
        // Coords, etc
        _ptInMM.clear();
        _ptInCoordUnits.clear();
        _ptInSteps.clear();
        _extrudeValue = 0.0;
        _feedrateValue = 0.0;
        _moveType = RobotMoveTypeArg_None;
        _endstops.none();
    }

    RobotCommandArgs& operator=(const RobotCommandArgs& copyFrom)
    {
        copy(copyFrom);
        return *this;
    }

    bool operator==(const RobotCommandArgs& other)
    {
        bool isEqual =
            // Flags
            (_ptUnitsSteps == other._ptUnitsSteps) &&
            (_dontSplitMove == other._dontSplitMove) &&
            (_extrudeValid == other._extrudeValid) &&
            (_feedrateValid == other._feedrateValid) &&
            (_moveClockwise == other._moveClockwise) &&
            (_moveRapid == other._moveRapid) &&
            (_allowOutOfBounds == other._allowOutOfBounds) &&
            (_pause == other._pause) &&
            (_moreMovesComing == other._moreMovesComing) &&
            // Command control
            (_queuedCommands == other._queuedCommands) &&
            (_numberedCommandIndex == other._numberedCommandIndex) &&
            // Endstops etc
            (_extrudeValue == other._extrudeValue) &&
            (_feedrateValue == other._feedrateValue) &&
            (_moveType == other._moveType) &&
            (_endstops == other._endstops);
        if (!isEqual)
            return false;
        // Coords, etc
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            if (_ptInMM.isValid(axisIdx) != _ptInMM.isValid(axisIdx))
                return false;
            if (_ptInMM.isValid(axisIdx) &&
                ((_ptInMM != other._ptInMM) ||
                (_ptInCoordUnits != other._ptInCoordUnits) ||
                (_ptInSteps != other._ptInSteps)))
                return false;
        }
        return true;
    }

    bool operator!=(const RobotCommandArgs& other)
    {
        return !(*this == other);
    }

private:
    void copy(const RobotCommandArgs &copyFrom)
    {
        clear();
        // Flags
        _ptUnitsSteps = copyFrom._ptUnitsSteps;
        _dontSplitMove = copyFrom._dontSplitMove;
        _extrudeValid = copyFrom._extrudeValid;
        _feedrateValid = copyFrom._feedrateValid;
        _moveClockwise = copyFrom._moveClockwise;
        _moveRapid = copyFrom._moveRapid;
        _allowOutOfBounds = copyFrom._allowOutOfBounds;
        _pause = copyFrom._pause;
        _moreMovesComing = copyFrom._moreMovesComing;
        // Command control
        _queuedCommands = copyFrom._queuedCommands;
        _numberedCommandIndex = copyFrom._numberedCommandIndex;
        // Coords, etc
        _ptInMM = copyFrom._ptInMM;
        _ptInCoordUnits = copyFrom._ptInCoordUnits;
        _ptInSteps = copyFrom._ptInSteps;
        _extrudeValue = copyFrom._extrudeValue;
        _feedrateValue = copyFrom._feedrateValue;
        _moveType = copyFrom._moveType;
        _endstops = copyFrom._endstops;
    }

public:
    RobotCommandArgs(const RobotCommandArgs &other)
    {
        copy(other);
    }
    void setAxisValMM(int axisIdx, float value, bool isValid)
    {
        if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
        {
            _ptInMM.setVal(axisIdx, value);
            _ptInMM.setValid(axisIdx, isValid);
            _ptUnitsSteps = false;
        }
    }
    void setAxisSteps(int axisIdx, int32_t value, bool isValid)
    {
        if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
        {
            _ptInSteps.setVal(axisIdx, value);
            // Piggy-back on MM validity flags
            _ptInMM.setValid(axisIdx, isValid);
            _ptUnitsSteps = true;
        }
    }
    void reverseStepDirection()
    {
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            if (isValid(axisIdx))
                _ptInSteps.setVal(axisIdx, -_ptInSteps.getVal(axisIdx));
        }
    }
    bool isValid(int axisIdx)
    {
        return _ptInMM.isValid(axisIdx);
    }
    bool anyValid()
    {
        return _ptInMM.anyValid();
    }
    bool isStepwise()
    {
        return _ptUnitsSteps;
    }
    // Indicate that all axes need to be homed
    void setAllAxesNeedHoming()
    {
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
            setAxisValMM(axisIdx, 0, true);
    }
    inline float getValNoCkMM(int axisIdx)
    {
        return _ptInMM.getValNoCk(axisIdx);
    }
    float getValMM(int axisIdx)
    {
        return _ptInMM.getVal(axisIdx);
    }
    float getValCoordUnits(int axisIdx)
    {
        return _ptInCoordUnits.getVal(axisIdx);
    }
    AxisFloats &getPointMM()
    {
        return _ptInMM;
    }
    void setPointMM(AxisFloats &ptInMM)
    {
        _ptInMM = ptInMM;
    }
    void setPointSteps(AxisInt32s &ptInSteps)
    {
        _ptInSteps = ptInSteps;
    }
    AxisInt32s &getPointSteps()
    {
        return _ptInSteps;
    }
    void setMoveType(RobotMoveTypeArg moveType)
    {
        _moveType = moveType;
    }
    RobotMoveTypeArg getMoveType()
    {
        return _moveType;
    }
    void setMoveRapid(bool moveRapid)
    {
        _moveRapid = moveRapid;
    }
    void setMoreMovesComing(bool moreMovesComing)
    {
        _moreMovesComing = moreMovesComing;
    }
    bool getMoreMovesComing()
    {
        return _moreMovesComing;
    }
    void setFeedrate(float feedrate)
    {
        _feedrateValue = feedrate;
        _feedrateValid = true;
    }
    bool isFeedrateValid()
    {
        return _feedrateValid;
    }
    float getFeedrate()
    {
        return _feedrateValue;
    }
    void setExtrude(float extrude)
    {
        _extrudeValue = extrude;
        _extrudeValid = true;
    }
    bool isExtrudeValid()
    {
        return _extrudeValid;
    }
    float getExtrude()
    {
        return _extrudeValue;
    }
    void setEndStops(AxisMinMaxBools endstops)
    {
        _endstops = endstops;
    }
    void setTestAllEndStops()
    {
        _endstops.all();
        Log.notice("Test all endstops\n");
    }

    void setTestNoEndStops()
    {
        _endstops.none();
    }

    void setTestEndStopsDefault()
    {
        _endstops.none();
    }

    void setTestEndStop(int axisIdx, int endStopIdx, AxisMinMaxBools::AxisMinMaxEnum checkType)
    {
        _endstops.set(axisIdx, endStopIdx, checkType);
    }

    inline AxisMinMaxBools &getEndstopCheck()
    {
        return _endstops;
    }

    void reverseEndstopChecks()
    {
        _endstops.reverse();
    }
    void setAllowOutOfBounds(bool allowOutOfBounds = true)
    {
        _allowOutOfBounds = allowOutOfBounds;
    }
    bool getAllowOutOfBounds()
    {
        return _allowOutOfBounds;
    }
    void setDontSplitMove(bool dontSplitMove = true)
    {
        _dontSplitMove = dontSplitMove;
    }
    bool getDontSplitMove()
    {
        return _dontSplitMove;
    }
    void setNumberedCommandIndex(int cmdIdx)
    {
        _numberedCommandIndex = cmdIdx;
    }
    int getNumberedCommandIndex()
    {
        return _numberedCommandIndex;
    }
    void setNumQueued(int numQueued)
    {
        _queuedCommands = numQueued;
    }
    void setPause(bool pause)
    {
        _pause = pause;
    }
    void setIsHoming(bool isHoming)
    {
        _isHoming = isHoming;
    }
    void setHasHomed(bool hasHomed)
    {
        _hasHomed = hasHomed;
    }
    String toJSON(bool includeBraces = true)
    {
        String jsonStr;
        if (includeBraces)
            jsonStr = "{";
        jsonStr += "\"XYZ\":" + _ptInMM.toJSON();
        jsonStr += ",\"ABC\":" + _ptInSteps.toJSON();
        if (_feedrateValid)
        {
            String feedrateStr = String(_feedrateValue, 2);
            jsonStr += ",\"F\":" + feedrateStr;
        }
        if (_extrudeValid)
        {
            String extrudeStr = String(_extrudeValue, 2);
            jsonStr += ",\"E\":" + extrudeStr;
        }
        jsonStr += ",\"mv\":";
        if (_moveType == RobotMoveTypeArg_Relative)
            jsonStr += "\"rel\"";
        else
            jsonStr += "\"abs\"";
        jsonStr += ",\"end\":" + _endstops.toJSON();
        jsonStr += ",\"OoB\":" + String(_allowOutOfBounds ? "\"Y\"" : "\"N\"");
        String numberedCmdStr = String(_numberedCommandIndex);
        jsonStr += ",\"num\":" + numberedCmdStr;
        String queuedCommandsStr = String(_queuedCommands);
        jsonStr += ",\"Qd\":" + queuedCommandsStr;
        jsonStr += String(",\"Hmd\":") + (_hasHomed ? "1" : "0");
        if (_isHoming)
            jsonStr += ",\"Homing\":1";
        jsonStr += String(",\"pause\":") + (_pause ? "1" : "0");
        if (includeBraces)
            jsonStr += "}";
        return jsonStr;
    }
};
