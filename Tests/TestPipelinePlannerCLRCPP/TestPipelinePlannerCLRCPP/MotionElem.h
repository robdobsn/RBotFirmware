#pragma once

#include "AxisValues.h"

class MotionElem
{
public:
	// From
	AxisFloats _pt1MM;
	// To
	AxisFloats _pt2MM;

	MotionElem(AxisFloats& pt1, AxisFloats& pt2)
	{
		// Set points
		_pt1MM = pt1;
		_pt2MM = pt2;
	}

	double delta()
	{
		return _pt1MM.distanceTo(_pt2MM);
	}
};
