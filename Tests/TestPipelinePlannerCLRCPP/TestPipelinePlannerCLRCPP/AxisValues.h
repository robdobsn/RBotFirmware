// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "math.h"

class AxisFloats
{
public:
	static constexpr int MAX_AXES = 3;
	float _pt[MAX_AXES];
	uint8_t _validityFlags;

public:
	AxisFloats()
	{
		for (int i = 0; i < MAX_AXES; i++)
			_pt[i] = 0;
		_validityFlags = 0;
	}
	AxisFloats(const AxisFloats& other)
	{
		for (int i = 0; i < MAX_AXES; i++)
			_pt[i] = other._pt[i];
		_validityFlags = other._validityFlags;
	}
	AxisFloats(float x, float y)
	{
		_pt[0] = x;
		_pt[1] = y;
		_pt[2] = 0;
		_validityFlags = 0xff;
	}
	AxisFloats(float x, float y, float z)
	{
		_pt[0] = x;
		_pt[1] = y;
		_pt[2] = z;
		_validityFlags = 0xff;
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
		{
			int axisMask = 0x01 << axisIdx;
			_pt[axisIdx] = val;
			_validityFlags |= axisMask;
		}
	}
	void set(float val0, float val1, float val2 = 0)
	{
		_pt[0] = val0;
		_pt[1] = val1;
		_pt[2] = val2;
		_validityFlags = 0xff;
	}
	void setValid(int axisIdx, bool isValid)
	{
		if (axisIdx >= 0 && axisIdx < MAX_AXES)
		{
			int axisMask = 0x01 << axisIdx;
			if (isValid)
				_validityFlags |= axisMask;
			else
				_validityFlags &= ~axisMask;
		}
	}
	bool isValid(int axisIdx)
	{
		if (axisIdx >= 0 && axisIdx < MAX_AXES)
		{
			int axisMask = 0x01 << axisIdx;
			return (_validityFlags & axisMask) != 0;
		}
		return false;
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
	void operator=(const AxisFloats& other)
	{
		for (int i = 0; i < MAX_AXES; i++)
			_pt[i] = other._pt[i];
		_validityFlags = other._validityFlags;
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
	static constexpr int MAX_AXES = 3;
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
	bool operator[](int axisIdx)
	{
		if (axisIdx >= 0 && axisIdx < MAX_AXES)
			return _valid[axisIdx];
		return false;
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
	static constexpr int MAX_AXES = 3;
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
