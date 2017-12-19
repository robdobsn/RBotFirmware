// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "math.h"

static const int MAX_AXES = 3;

class AxisFloats
{
public:
	float _pt[MAX_AXES];

public:
	AxisFloats()
	{
		for (int i = 0; i < MAX_AXES; i++)
			_pt[i] = 0;
	}
	AxisFloats(const AxisFloats& pt)
	{
		for (int i = 0; i < MAX_AXES; i++)
			_pt[i] = pt._pt[i];
	}
	AxisFloats(float x, float y)
	{
		_pt[0] = x;
		_pt[1] = y;
		_pt[2] = 0;
	}
	AxisFloats(float x, float y, float z)
	{
		_pt[0] = x;
		_pt[1] = y;
		_pt[2] = z;
	}
	float getVal(int axisIdx)
	{
		if (axisIdx >= 0 && axisIdx < MAX_AXES)
			return _pt[axisIdx];
		return 0;
	}
	void setVal(int axisIdx, float val)
	{
		if (axisIdx >= 0 && axisIdx < MAX_AXES)
			_pt[axisIdx] = val;
	}
	void set(float val0, float val1, float val2 = 0)
	{
		_pt[0] = val0;
		_pt[1] = val1;
		_pt[2] = val2;
	}
	float X()
	{
		return _pt[0];
	}
	float Y()
	{
		return _pt[1];
	}
	float Z()
	{
		return _pt[2];
	}
	void operator=(const AxisFloats& pt)
	{
		for (int i = 0; i < MAX_AXES; i++)
			_pt[i] = pt._pt[i];
	}
	AxisFloats operator-(const AxisFloats& pt)
	{
		AxisFloats result;
		for (int i = 0; i < MAX_AXES; i++)
			result._pt[i] = _pt[i] - pt._pt[i];
		return result;
	}
	AxisFloats operator-(float val)
	{
		AxisFloats result;
		for (int i = 0; i < MAX_AXES; i++)
			result._pt[i] = _pt[i] - val;
		return result;
	}
	AxisFloats operator+(const AxisFloats& pt)
	{
		AxisFloats result;
		for (int i = 0; i < MAX_AXES; i++)
			result._pt[i] = _pt[i] + pt._pt[i];
		return result;
	}
	AxisFloats operator+(float val)
	{
		AxisFloats result;
		for (int i = 0; i < MAX_AXES; i++)
			result._pt[i] = _pt[i] + val;
		return result;
	}
	AxisFloats operator/(const AxisFloats& pt)
	{
		AxisFloats result;
		for (int i = 0; i < MAX_AXES; i++)
		{
			if (pt._pt[i] != 0)
				result._pt[i] = _pt[i] / pt._pt[i];
		}
		return result;
	}
	AxisFloats operator/(float val)
	{
		AxisFloats result;
		for (int i = 0; i < MAX_AXES; i++)
		{
			if (val != 0)
				result._pt[i] = _pt[i] / val;
		}
		return result;
	}
	AxisFloats operator*(const AxisFloats& pt)
	{
		AxisFloats result;
		for (int i = 0; i < MAX_AXES; i++)
		{
			result._pt[i] = _pt[i] * pt._pt[i];
		}
		return result;
	}
	AxisFloats operator*(float val)
	{
		AxisFloats result;
		for (int i = 0; i < MAX_AXES; i++)
		{
			result._pt[i] = _pt[i] * val;
		}
		return result;
	}
	float distanceTo(const AxisFloats& pt, bool includeDist[] = NULL)
	{
		float distSum = 0;
		for (int i = 0; i < MAX_AXES; i++)
		{
			if ((includeDist == NULL) || includeDist[i])
			{
				float sq = _pt[i] - pt._pt[i];
				sq = sq*sq;
				distSum += sq;
			}
		}
		return sqrt(distSum);
	}
	void logDebugStr(const char* prefixStr)
	{
		Log.trace("%s X %0.2f Y %0.2f Z %0.2f", prefixStr, _pt[0], _pt[1], _pt[2]);
	}
};

class AxisBools
{
public:
	bool _valid[MAX_AXES];

public:
	AxisBools()
	{
		for (int i = 0; i < MAX_AXES; i++)
			_valid[i] = false;
	}
	AxisBools(const AxisBools& valid)
	{
		for (int i = 0; i < MAX_AXES; i++)
			_valid[i] = valid._valid[i];
	}
	void operator=(const AxisBools& valid)
	{
		for (int i = 0; i < MAX_AXES; i++)
			_valid[i] = valid._valid[i];
	}
	AxisBools(bool xValid, bool yValid, bool zValid)
	{
		_valid[0] = xValid;
		_valid[1] = yValid;
		_valid[2] = zValid;
	}
	bool X()
	{
		return _valid[0];
	}
	bool Y()
	{
		return _valid[1];
	}
	bool Z()
	{
		return _valid[2];
	}
	bool isValid(int axisIdx)
	{
		if (axisIdx >= 0 && axisIdx < MAX_AXES)
			return _valid[axisIdx];
		return false;
	}
	void setVal(int axisIdx, bool val)
	{
		if (axisIdx >= 0 && axisIdx < MAX_AXES)
			_valid[axisIdx] = val;
	}
};

class AxisU32s
{
public:
	uint32_t vals[MAX_AXES];

public:
	AxisU32s()
	{
		for (int i = 0; i < MAX_AXES; i++)
			vals[i] = 0;
	}
	AxisU32s(const AxisU32s& u32s)
	{
		for (int i = 0; i < MAX_AXES; i++)
			vals[i] = u32s.vals[i];
	}
	void operator=(const AxisU32s& u32s)
	{
		for (int i = 0; i < MAX_AXES; i++)
			vals[i] = u32s.vals[i];
	}
	AxisU32s(uint32_t xVal, uint32_t yVal, uint32_t zVal)
	{
		vals[0] = xVal;
		vals[1] = yVal;
		vals[2] = zVal;
	}
	uint32_t X()
	{
		return vals[0];
	}
	uint32_t Y()
	{
		return vals[1];
	}
	uint32_t Z()
	{
		return vals[2];
	}
	uint32_t getVal(int axisIdx)
	{
		if (axisIdx >= 0 && axisIdx < MAX_AXES)
			return vals[axisIdx];
		return 0;
	}
	void setVal(int axisIdx, uint32_t val)
	{
		if (axisIdx >= 0 && axisIdx < MAX_AXES)
			vals[axisIdx] = val;
	}
};
