#pragma once

#include "AxisValues.h"

class AxisMotion
{
public:
	AxisFloats _axisPositionMM;
	AxisFloats _stepsFromHome;

	void clear()
	{
		_axisPositionMM.set(0, 0, 0);
		_stepsFromHome.set(0, 0, 0);
	}
};
