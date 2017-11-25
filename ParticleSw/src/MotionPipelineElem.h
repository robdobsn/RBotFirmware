// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "PointND.h"

class MotionPipelineElem
{
public:
    PointND _pt1MM;
    PointND _pt2MM;
    double _accMMpss;
    double _speedMMps;

public:
    MotionPipelineElem()
    {
        _accMMpss = 0;
        _speedMMps = 0;
    }

    MotionPipelineElem(PointND& pt1, PointND& pt2)
    {
        _pt1MM = pt1;
        _pt2MM = pt2;
        _accMMpss = 0;
        _speedMMps = 0;
    }

    double delta()
    {
        return _pt1MM.distanceTo(_pt2MM);
    }
};
