// RBotFirmware
// Rob Dobson 2017

#pragma once

#include "math.h"

static const int MAX_AXES = 3;

class PointND
{
public:
    double _pt[MAX_AXES];

public:
    PointND()
    {
        for (int i = 0; i < MAX_AXES; i++)
            _pt[i] = 0;
    }
    PointND(const PointND& pt)
    {
        for (int i = 0; i < MAX_AXES; i++)
            _pt[i] = pt._pt[i];
    }
    PointND(double x, double y)
    {
        _pt[0] = x;
        _pt[1] = y;
        _pt[2] = 0;
    }
    PointND(double x, double y, double z)
    {
        _pt[0] = x;
        _pt[1] = y;
        _pt[2] = z;
    }
    double getVal(int axisIdx)
    {
        if (axisIdx >= 0 && axisIdx < MAX_AXES)
            return _pt[axisIdx];
        return 0;
    }
    void setVal(int axisIdx, double val)
    {
        if (axisIdx >= 0 && axisIdx < MAX_AXES)
            _pt[axisIdx] = val;
    }
    void set(double val0, double val1, double val2 = 0)
    {
        _pt[0] = val0;
        _pt[1] = val1;
        _pt[2] = val2;
    }
    double X()
    {
        return _pt[0];
    }
    double Y()
    {
        return _pt[1];
    }
    double Z()
    {
        return _pt[2];
    }
    void operator=(const PointND& pt)
    {
        for (int i = 0; i < MAX_AXES; i++)
            _pt[i] = pt._pt[i];
    }
    PointND operator-(const PointND& pt)
    {
        PointND result;
        for (int i = 0; i < MAX_AXES; i++)
            result._pt[i] = _pt[i] - pt._pt[i];
        return result;
    }
    PointND operator-(double val)
    {
        PointND result;
        for (int i = 0; i < MAX_AXES; i++)
            result._pt[i] = _pt[i] - val;
        return result;
    }
    PointND operator+(const PointND& pt)
    {
        PointND result;
        for (int i = 0; i < MAX_AXES; i++)
            result._pt[i] = _pt[i] + pt._pt[i];
        return result;
    }
    PointND operator+(double val)
    {
        PointND result;
        for (int i = 0; i < MAX_AXES; i++)
            result._pt[i] = _pt[i] + val;
        return result;
    }
    PointND operator/(const PointND& pt)
    {
        PointND result;
        for (int i = 0; i < MAX_AXES; i++)
        {
            if (pt._pt[i] != 0)
                result._pt[i] = _pt[i] / pt._pt[i];
        }
        return result;
    }
    PointND operator/(double val)
    {
        PointND result;
        for (int i = 0; i < MAX_AXES; i++)
        {
            if (val != 0)
                result._pt[i] = _pt[i] / val;
        }
        return result;
    }
    PointND operator*(const PointND& pt)
    {
        PointND result;
        for (int i = 0; i < MAX_AXES; i++)
        {
            result._pt[i] = _pt[i] * pt._pt[i];
        }
        return result;
    }
    PointND operator*(double val)
    {
        PointND result;
        for (int i = 0; i < MAX_AXES; i++)
        {
            result._pt[i] = _pt[i] * val;
        }
        return result;
    }
    double distanceTo(const PointND& pt, bool includeDist[] = NULL)
    {
        double distSum = 0;
        for (int i = 0; i < MAX_AXES; i++)
        {
            if ((includeDist == NULL) || includeDist[i])
            {
                double sq = _pt[i] - pt._pt[i];
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
    String toJSON()
    {
        String jsonStr;
        if (MAX_AXES >= 2)
        {
            jsonStr = "\"X\":" + String::format("%0.5f", _pt[0]) +
                ",\"Y\":" + String::format("%0.5f", _pt[1]);
        }
        if (MAX_AXES >= 3)
        {
            jsonStr += ",\"Z\":" + String::format("%0.5f", _pt[2]);
        }
        return "{" + jsonStr + "}";
    }
};

class PointNDValid
{
public:
    bool _valid[MAX_AXES];

public:
    PointNDValid()
    {
        for (int i = 0; i < MAX_AXES; i++)
            _valid[i] = false;
    }
    PointNDValid(const PointNDValid& valid)
    {
        for (int i = 0; i < MAX_AXES; i++)
            _valid[i] = valid._valid[i];
    }
    void operator=(const PointNDValid& valid)
    {
        for (int i = 0; i < MAX_AXES; i++)
            _valid[i] = valid._valid[i];
    }
    PointNDValid(bool xValid, bool yValid, bool zValid)
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
