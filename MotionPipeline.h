// RBotFirmware
// Rob Dobson 2016

#pragma once

#include <queue>
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

class MotionPipeline
{
private:
    std::queue<MotionPipelineElem> _pipeline;
    static const int _pipelineMaxLenDefault = 300;
    int _pipelineMaxLen = _pipelineMaxLenDefault;

public:
    // Get slots available
    int slotsEmpty()
    {
        return _pipelineMaxLen - _pipeline.size();
    }

    // Add to pipeline
    bool add(MotionPipelineElem& elem)
    {
        // Check if queue is full
        if (_pipeline.size() >= _pipelineMaxLen)
        {
            return false;
        }

        // Queue up the item
        _pipeline.push(elem);
        return true;
    }

    // Add to pipeline
    bool add(PointND& pt1, PointND& pt2)
    {
        // Check if queue is full
        if (_pipeline.size() >= _pipelineMaxLen)
        {
            return false;
        }

        // Queue up the item
        MotionPipelineElem elem(pt1, pt2);
        _pipeline.push(elem);
        return true;
    }

    // Get from queue
    bool get(MotionPipelineElem& elem)
    {
        // Check if queue is empty
        if (_pipeline.empty())
        {
            return false;
        }

        // read the item and remove
        elem = _pipeline.front();
        _pipeline.pop();
        return true;
    }

    // Peek the last block (if there is one)
    bool peekLast(MotionPipelineElem& elem)
    {
        // Check if queue is empty
        if (_pipeline.empty())
        {
            return false;
        }
        // read the last item
        elem = _pipeline.back();
        return true;
    }


    // Get number of items in queue
    int count()
    {
        return _pipeline.size();
    }
};
