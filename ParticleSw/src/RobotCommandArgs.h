// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "AxisValues.h"

enum RobotMoveTypeArg
{
  RobotMoveTypeArg_None,
  RobotMoveTypeArg_Absolute,
  RobotMoveTypeArg_Relative
};

class RobotCommandArgs
{
public:
  AxisFloats pt;
  bool extrudeValid;
  float extrudeVal;
  bool feedrateValid;
  float feedrateVal;
  AxisMinMaxBools endstopCheck;
  bool moveClockwise;
  bool moveRapid;
  RobotMoveTypeArg moveType;

public:
  RobotCommandArgs()
  {
    clear();
  }
  void clear()
  {
    extrudeValid          = feedrateValid = false;
    extrudeVal            = feedrateVal = 0.0;
    moveClockwise         = moveRapid = false;
    moveType              = RobotMoveTypeArg_None;
  }
  RobotCommandArgs(const RobotCommandArgs& other)
  {
    copy(other);
  }
  void operator=(const RobotCommandArgs& other)
  {
    copy(other);
  }
  void setAxisValue(int axisIdx, float value, bool isValid)
  {
    if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
    {
      pt.setVal(axisIdx, value);
      pt.setValid(axisIdx, isValid);
    }
  }

  void setTestAllEndStops()
  {
    endstopCheck.all();
    Log.info("Test all endstops");
  }

  void setTestNoEndStops()
  {
    endstopCheck.none();
  }

  void setTestEndStopsDefault()
  {
    endstopCheck.none();
  }

  String toJSON()
  {
    String jsonStr;
    jsonStr  = "{\"pos\":" + pt.toJSON();
    jsonStr += ", \"mvTyp\":";
    if (moveType == RobotMoveTypeArg_Relative)
      jsonStr += "\"rel\"";
    else
      jsonStr += "\"abs\"";
    jsonStr += ",\"endStopChk\":\"";
    String endStopStr = String::format("%08lx", endstopCheck.uintVal());
    jsonStr += endStopStr + "\"";
    jsonStr += "}";
    return jsonStr;
  }

private:
  void copy(const RobotCommandArgs& copyFrom)
  {
    clear();
    pt                    = copyFrom.pt;
    extrudeValid          = copyFrom.extrudeValid;
    extrudeVal            = copyFrom.extrudeVal;
    feedrateValid         = copyFrom.feedrateValid;
    feedrateVal           = copyFrom.feedrateVal;
    endstopCheck          = copyFrom.endstopCheck;
    moveClockwise         = copyFrom.moveClockwise;
    moveRapid             = copyFrom.moveRapid;
    moveType              = copyFrom.moveType;
  }
};
