// RBotFirmware
// Rob Dobson 2016

#pragma once

#include <queue>
#include "MotionPipelineElem.h"

class MotionPipeline
{
private:
    std::queue<MotionPipelineElem> _pipeline;
    static const unsigned int _pipelineMaxLenDefault = 300;
    unsigned int _pipelineMaxLen = _pipelineMaxLenDefault;
    unsigned int _maxBlocksInAMove = 1;

public:
    // Get slots available
    unsigned int slotsEmpty()
    {
        return _pipelineMaxLen - _pipeline.size();
    }

    // Set parameters to determine pipeline space required to be left free
    void setParameters(int maxBlocksInAMove)
    {
        if (maxBlocksInAMove < 1)
            maxBlocksInAMove = 1;
        _maxBlocksInAMove = maxBlocksInAMove;
    }

    // Check if ready to accept data
    bool canAccept()
    {
        // Check if block len is reasonable
        if (_maxBlocksInAMove < _pipelineMaxLen / 2)
        {
            // Log.trace("MotionPipeline canAccept %d slotsEmpty %d _maxBlocksInAMove %d",
            //     slotsEmpty() >= _maxBlocksInAMove, slotsEmpty(), _maxBlocksInAMove);
            // If so ensure enough space for maximal block length
            return slotsEmpty() >= _maxBlocksInAMove;
        }
        // Log.trace("MotionPipeline canAccept %d slotsEmpty %d _maxBlocksInAMove %d, _pipelineMaxLen %d",
        //     slotsEmpty() >= _pipelineMaxLen / 3, slotsEmpty(), _maxBlocksInAMove, _pipelineMaxLen);
        // If not return ok if 33%+ of pipeline is free
        return slotsEmpty() >= _pipelineMaxLen / 3;
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

    // Clear the pipeline
    void clear()
    {
        // Using technique from here https://stackoverflow.com/questions/709146/how-do-i-clear-the-stdqueue-efficiently
        std::queue<MotionPipelineElem> emptyQ;
        std::swap( _pipeline, emptyQ );
    }
};
