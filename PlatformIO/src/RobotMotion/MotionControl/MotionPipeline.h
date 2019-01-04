// RBotFirmware
// Rob Dobson 2016-2018

#pragma once

#include "MotionRingBuffer.h"
#include "MotionBlock.h"
#include <vector>

class MotionPipeline
{
  private:
    MotionRingBufferPosn _pipelinePosn;
    std::vector<MotionBlock> _pipeline;

  public:
    MotionPipeline() : _pipelinePosn(0)
    {
    }

    void init(int pipelineSize)
    {
        _pipeline.resize(pipelineSize);
        _pipelinePosn.init(pipelineSize);
    }

    // Clear the pipeline
    void clear()
    {
        _pipelinePosn.clear();
    }

    unsigned int count()
    {
        return _pipelinePosn.count();
    }

    // Check if ready to accept data
    bool canAccept()
    {
        return _pipelinePosn.canPut();
    }

    // Add to pipeline
    bool add(MotionBlock &block)
    {
        // Check if full
        if (!_pipelinePosn.canPut())
            return false;

        // Add the item
        _pipeline[_pipelinePosn._putPos] = block;
        _pipelinePosn.hasPut();
        return true;
    }

    // Can get from queue (i.e. not empty)
    bool IRAM_ATTR canGet()
    {
        return _pipelinePosn.canGet();
    }

    // Get from queue
    bool IRAM_ATTR get(MotionBlock &block)
    {
        // Check if queue is empty
        if (!_pipelinePosn.canGet())
            return false;

        // read the item and remove
        block = _pipeline[_pipelinePosn._getPos];
        _pipelinePosn.hasGot();
        return true;
    }

    // Remove last element from queue
    bool IRAM_ATTR remove()
    {
        // Check if queue is empty
        if (!_pipelinePosn.canGet())
            return false;

        // remove item
        _pipelinePosn.hasGot();
        return true;
    }

    // Peek the block which would be got (if there is one)
    MotionBlock* IRAM_ATTR peekGet()
    {
        // Check if queue is empty
        if (!_pipelinePosn.canGet())
            return NULL;
        // get pointer to the last item (don't remove)
        return &(_pipeline[_pipelinePosn._getPos]);
    }

    // Peek from the put position
    // 0 is the last element put in the queue
    // 1 is the one put in before that
    // returns NULL when nothing to peek
    MotionBlock *peekNthFromPut(unsigned int N)
    {
        // Get index
        int nthPos = _pipelinePosn.getNthFromPut(N);
        if (nthPos < 0)
            return NULL;
        return &(_pipeline[nthPos]);
    }

    // Peek from the get position
    // 0 is the element next got from the queue
    // 1 is the one got after that
    // returns NULL when nothing to peek
    MotionBlock *peekNthFromGet(unsigned int N)
    {
        // Get index
        int nthPos = _pipelinePosn.getNthFromGet(N);
        if (nthPos < 0)
            return NULL;
        return &(_pipeline[nthPos]);
    }

    // Debug
    void debugShowBlocks(AxesParams &axesParams)
    {
        int elIdx = 0;
        bool headShown = false;
        for (int i = count() - 1; i >= 0; i--)
        {
            MotionBlock *pBlock = peekNthFromPut(i);
            if (pBlock)
            {
                if (!headShown)
                {
                    pBlock->debugShowBlkHead();
                    headShown = true;
                }
                pBlock->debugShowBlock(elIdx++, axesParams);
            }
        }
    }

    // Debug
    void debugShowTopBlock(AxesParams &axesParams)
    {
        int cnt = count();
        if (cnt == 0)
            return;
        MotionBlock *pBlock = peekNthFromPut(cnt-1);
        if (pBlock)
            pBlock->debugShowBlock(0, axesParams);
    }
};
