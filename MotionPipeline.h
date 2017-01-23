// RBotFirmware
// Rob Dobson 2016

#pragma once

#include <queue>

class MotionPipelineElem
{
public:
    double _xPos;
    double _yPos;

public:
    MotionPipelineElem()
    {
        _xPos = 0;
        _yPos = 0;
    }

    MotionPipelineElem(double& x, double& y)
    {
        _xPos = x;
        _yPos = y;
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
    bool add(double& x, double& y)
    {
        // Check if queue is full
        if (_pipeline.size() >= _pipelineMaxLen)
        {
            return false;
        }

        // Queue up the item
        MotionPipelineElem elem(x,y);
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
