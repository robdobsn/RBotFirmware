// RBotFirmware
// Rob Dobson 2016-18

#pragma once

#include "AxisValues.h"

class AxisPosition
{
  public:
    AxisFloats _axisPositionMM;
    AxisInt32s _stepsFromHome;

    void clear()
    {
        _axisPositionMM.set(0, 0, 0);
        _stepsFromHome.set(0, 0, 0);
    }
};
