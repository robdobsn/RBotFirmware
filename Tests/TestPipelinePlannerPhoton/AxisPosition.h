#pragma once

#include "AxisValues.h"

class AxisPosition
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
