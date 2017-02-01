// RBotFirmware
// Rob Dobson 2016

#pragma once

#include <queue>

class MotionPipelineElem
{
public:
    double _x1MM;
    double _y1MM;
    double _x2MM;
    double _y2MM;
    double _accMMpss;
    double _speedMMps;

public:
    MotionPipelineElem()
    {
        _x1MM = 0;
        _y1MM = 0;
        _x2MM = 0;
        _y2MM = 0;
        _accMMpss = 0;
        _speedMMps = 0;
    }

    MotionPipelineElem(double& x1, double& y1, double& x2, double& y2)
    {
        _x1MM = x1;
        _y1MM = y1;
        _x2MM = x2;
        _y2MM = y2;
        _accMMpss = 0;
        _speedMMps = 0;
    }
};

class MotionPipeline
{
private:
    std::queue<MotionPipelineElem> _pipeline;
    static const int _pipelineMaxLenDefault = 1000;
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
    bool add(double& x1, double& y1, double& x2, double& y2)
    {
        // Check if queue is full
        if (_pipeline.size() >= _pipelineMaxLen)
        {
            return false;
        }

        // Queue up the item
        MotionPipelineElem elem(x1,y1,x2,y2);
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
