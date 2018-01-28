// RBotFirmware
// Rob Dobson 2016-18

#pragma once

#include "math.h"
#include "RobotConsts.h"

class AxisFloats
{
public:
  float _pt[RobotConsts::MAX_AXES];
  uint8_t _validityFlags;

public:
  AxisFloats()
  {
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
      _pt[i] = 0;
    _validityFlags = 0;
  }
  AxisFloats(const AxisFloats& other)
  {
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
      _pt[i] = other._pt[i];
    _validityFlags = other._validityFlags;
  }
  AxisFloats(float x, float y)
  {
    _pt[0]         = x;
    _pt[1]         = y;
    _pt[2]         = 0;
    _validityFlags = 0x03;
  }
  AxisFloats(float x, float y, float z)
  {
    _pt[0]         = x;
    _pt[1]         = y;
    _pt[2]         = z;
    _validityFlags = 0x07;
  }
  AxisFloats(float x, float y, float z, bool xValid, bool yValid, bool zValid)
  {
    _pt[0]          = x;
    _pt[1]          = y;
    _pt[2]          = z;
    _validityFlags  = xValid ? 0x01 : 0;
    _validityFlags |= yValid ? 0x02 : 0;
    _validityFlags |= zValid ? 0x04 : 0;
  }
  float getVal(int axisIdx)
  {
    if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
      return _pt[axisIdx];
    return 0;
  }
  void setVal(int axisIdx, float val)
  {
    if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
    {
      int axisMask = 0x01 << axisIdx;
      _pt[axisIdx]    = val;
      _validityFlags |= axisMask;
    }
  }
  void set(float val0, float val1, float val2 = 0)
  {
    _pt[0]         = val0;
    _pt[1]         = val1;
    _pt[2]         = val2;
    _validityFlags = 0x07;
  }
  void setValid(int axisIdx, bool isValid)
  {
    if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
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
    if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
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
  void X(float val)
  {
    _pt[0]          = val;
    _validityFlags |= 0x01;
  }
  float Y()
  {
    return _pt[1];
  }
  void Y(float val)
  {
    _pt[1]          = val;
    _validityFlags |= 0x02;
  }
  float Z()
  {
    return _pt[2];
  }
  void Z(float val)
  {
    _pt[2]          = val;
    _validityFlags |= 0x04;
  }
  void operator=(const AxisFloats& other)
  {
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
      _pt[i] = other._pt[i];
    _validityFlags = other._validityFlags;
  }
  AxisFloats operator-(const AxisFloats& pt)
  {
    AxisFloats result;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
      result._pt[i] = _pt[i] - pt._pt[i];
    return result;
  }
  AxisFloats operator-(float val)
  {
    AxisFloats result;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
      result._pt[i] = _pt[i] - val;
    return result;
  }
  AxisFloats operator+(const AxisFloats& pt)
  {
    AxisFloats result;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
      result._pt[i] = _pt[i] + pt._pt[i];
    return result;
  }
  AxisFloats operator+(float val)
  {
    AxisFloats result;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
      result._pt[i] = _pt[i] + val;
    return result;
  }
  AxisFloats operator/(const AxisFloats& pt)
  {
    AxisFloats result;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
      if (pt._pt[i] != 0)
        result._pt[i] = _pt[i] / pt._pt[i];
    }
    return result;
  }
  AxisFloats operator/(float val)
  {
    AxisFloats result;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
      if (val != 0)
        result._pt[i] = _pt[i] / val;
    }
    return result;
  }
  AxisFloats operator*(const AxisFloats& pt)
  {
    AxisFloats result;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
      result._pt[i] = _pt[i] * pt._pt[i];
    }
    return result;
  }
  AxisFloats operator*(float val)
  {
    AxisFloats result;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
      result._pt[i] = _pt[i] * val;
    }
    return result;
  }
  float distanceTo(const AxisFloats& pt, bool includeDist[] = NULL)
  {
    float distSum = 0;
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
    {
      if ((includeDist == NULL) || includeDist[i])
      {
        float sq = _pt[i] - pt._pt[i];
        sq       = sq * sq;
        distSum += sq;
      }
    }
    return sqrt(distSum);
  }
  void logDebugStr(const char* prefixStr)
  {
    Log.trace("%s X %0.2f Y %0.2f Z %0.2f", prefixStr, _pt[0], _pt[1], _pt[2]);
  }
  String toJSON()
  {
    String jsonStr;
    if (RobotConsts::MAX_AXES >= 2)
    {
      jsonStr = "\"X\":" + String::format("%0.5f", _pt[0]) +
                ",\"Y\":" + String::format("%0.5f", _pt[1]);
    }
    if (RobotConsts::MAX_AXES >= 3)
    {
      jsonStr += ",\"Z\":" + String::format("%0.5f", _pt[2]);
    }
    return "{" + jsonStr + "}";
  }
};

class AxisBools
{
public:
  struct
  {
    bool b0 : 1;
    bool b1 : 1;
    bool b2 : 1;
    bool b3 : 1;
    bool b4 : 1;
    bool b5 : 1;
    bool b6 : 1;
  };

public:
  AxisBools()
  {
    b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0;
  }
  AxisBools(const AxisBools& other)
  {
    b0 = other.b0;
    b1 = other.b1;
    b2 = other.b2;
    b3 = other.b3;
    b4 = other.b4;
    b5 = other.b5;
    b6 = other.b6;
  }
  void operator=(const AxisBools& other)
  {
    b0 = other.b0;
    b1 = other.b1;
    b2 = other.b2;
    b3 = other.b3;
    b4 = other.b4;
    b5 = other.b5;
    b6 = other.b6;
  }
  bool isValid(int axisIdx)
  {
    switch (axisIdx)
    {
    case 0: return b0;
    case 1: return b1;
    case 2: return b2;
    case 3: return b3;
    case 4: return b4;
    case 5: return b5;
    case 6: return b6;
    }
    return false;
  }
  AxisBools(bool xValid, bool yValid, bool zValid)
  {
    b0 = xValid;
    b1 = yValid;
    b2 = zValid;
  }
  bool X()
  {
    return b0;
  }
  bool Y()
  {
    return b1;
  }
  bool Z()
  {
    return b2;
  }
  bool operator[](int axisIdx)
  {
    return isValid(axisIdx);
  }
  void setVal(int axisIdx, bool val)
  {
    switch (axisIdx)
    {
    case 0: b0 = val; break;
    case 1: b1 = val; break;
    case 2: b2 = val; break;
    case 3: b3 = val; break;
    case 4: b4 = val; break;
    case 5: b5 = val; break;
    case 6: b6 = val; break;
    }
  }
};

class AxisInt32s
{
public:
  int32_t vals[RobotConsts::MAX_AXES];

public:
  AxisInt32s()
  {
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
      vals[i] = 0;
  }
  AxisInt32s(const AxisInt32s& u32s)
  {
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
      vals[i] = u32s.vals[i];
  }
  void operator=(const AxisInt32s& u32s)
  {
    for (int i = 0; i < RobotConsts::MAX_AXES; i++)
      vals[i] = u32s.vals[i];
  }
  AxisInt32s(int32_t xVal, int32_t yVal, int32_t zVal)
  {
    vals[0] = xVal;
    vals[1] = yVal;
    vals[2] = zVal;
  }
  void set(int32_t val0, int32_t val1, int32_t val2 = 0)
  {
    vals[0] = val0;
    vals[1] = val1;
    vals[2] = val2;
  }
  int32_t X()
  {
    return vals[0];
  }
  int32_t Y()
  {
    return vals[1];
  }
  int32_t Z()
  {
    return vals[2];
  }
  int32_t getVal(int axisIdx)
  {
    if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
      return vals[axisIdx];
    return 0;
  }
  void setVal(int axisIdx, int32_t val)
  {
    if (axisIdx >= 0 && axisIdx < RobotConsts::MAX_AXES)
      vals[axisIdx] = val;
  }
};
