// RBotFirmware
// Rob Dobson 2016-18

#include "MotionPlanner.h"

void MotionPlanner::configure(float junctionDeviation)
{
    _junctionDeviation = junctionDeviation;
}

// Entry point for adding a motion block
bool MotionPlanner::moveTo(RobotCommandArgs &args,
            AxisFloats &destActuatorCoords,
            AxisPosition &curAxisPositions,
            AxesParams &axesParams, MotionPipeline &motionPipeline)
{
    // Find first primary axis
    int firstPrimaryAxis = -1;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        if (axesParams.isPrimaryAxis(axisIdx))
            firstPrimaryAxis = axisIdx;
    if (firstPrimaryAxis == -1)
        firstPrimaryAxis = 0;

    // Find axis deltas and sum of squares of motion on primary axes
    float deltas[RobotConsts::MAX_AXES];
    bool isAMove = false;
    bool isAPrimaryMove = false;
    int axisWithMaxMoveDist = 0;
    float squareSum = 0;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        deltas[axisIdx] = args.getValNoCkMM(axisIdx) - curAxisPositions._axisPositionMM._pt[axisIdx];
        if (deltas[axisIdx] != 0)
        {
            isAMove = true;
            if (axesParams.isPrimaryAxis(axisIdx))
            {
                squareSum += powf(deltas[axisIdx], 2);
                isAPrimaryMove = true;
            }
        }
        if (fabsf(deltas[axisIdx]) > fabsf(deltas[axisWithMaxMoveDist]))
            axisWithMaxMoveDist = axisIdx;
    }

    // Distance being moved
    float moveDist = sqrtf(squareSum);

    // Ignore if there is no real movement
    if (!isAMove || moveDist < MotionBlock::MINIMUM_MOVE_DIST_MM)
        return false;

    // Create a block for this movement which will end up on the pipeline
    MotionBlock block;

    // Set flag to indicate if more moves coming
    block._blockIsFollowed = args.getMoreMovesComing();

    // set end-stop check requirements
    block.setEndStopsToCheck(args.getEndstopCheck());

    // Set numbered command index if present
    block.setNumberedCommandIndex(args.getNumberedCommandIndex());

    // Max speed (may be overridden downwards by feedrate)
    float validFeedrateMMps = 1e8;
    if (args.isFeedrateValid())
        validFeedrateMMps = args.getFeedrate();

    // Check the feedrate against the first primary axis
    if (validFeedrateMMps > axesParams.getMaxSpeed(firstPrimaryAxis))
        validFeedrateMMps = axesParams.getMaxSpeed(firstPrimaryAxis);

    // Find the unit vectors for the primary axes and check the feedrate
    AxisFloats unitVectors;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        if (axesParams.isPrimaryAxis(axisIdx))
        {
            // Unit vector calculation
            unitVectors._pt[axisIdx] = deltas[axisIdx] / moveDist;
        }
    }

    // Store values in the block
    block._feedrate = validFeedrateMMps;
    block._moveDistPrimaryAxesMM = moveDist;

    // Find if there are any steps
    bool hasSteps = false;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        // Check if any steps to perform
        float stepsFloat = destActuatorCoords._pt[axisIdx] - curAxisPositions._stepsFromHome.vals[axisIdx];
        int32_t steps = int32_t(ceilf(stepsFloat));
        if (steps != 0)
            hasSteps = true;
        // Value (and direction)
        block.setStepsToTarget(axisIdx, steps);
    }

#ifdef DEBUG_MOTIONPLANNER_DETAILED_INFO
    Log.notice("F %F D %F uX %F uY %F, uZ %F maxStAx %d maxDAx %d %s\n", validFeedrateMMps,
            moveDist, 
            unitVectors.getVal(0), unitVectors.getVal(1), unitVectors.getVal(2), 
            block._axisIdxWithMaxSteps, axisWithMaxMoveDist,
            hasSteps ? "has steps" : "NO STEPS");
#endif

    // Check there are some actual steps
    if (!hasSteps)
        return false;

    // Set the dist moved on the axis with max steps
    block._unitVecAxisWithMaxDist = unitVectors.getVal(axisWithMaxMoveDist);

    // If there is a prior block then compute the maximum speed at exit of the second block to keep
    // the junction deviation within bounds - there are more comments in the Smoothieware (and GRBL) code
    float junctionDeviation = _junctionDeviation;
    float vmaxJunction = _minimumPlannerSpeedMMps;

    // Invalidate the data stored for the prev element if the pipeline becomes empty
    if (!motionPipeline.canGet())
        _prevMotionBlockValid = false;

    // Calculate the maximum speed for the junction between two blocks
    if (isAPrimaryMove && _prevMotionBlockValid)
    {
        float prevParamSpeed = isAPrimaryMove ? _prevMotionBlock._maxParamSpeedMMps : 0;
        if (junctionDeviation > 0.0f && prevParamSpeed > 0.0f)
        {
            // Compute cosine of angle between previous and current path. (prev_unit_vec is negative)
            // NOTE: Max junction velocity is computed without sin() or acos() by trig half angle identity.
            float cosTheta = -_prevMotionBlock._unitVectors.X() * unitVectors.X() - _prevMotionBlock._unitVectors.Y() * unitVectors.Y() - _prevMotionBlock._unitVectors.Z() * unitVectors.Z();

            // Skip and use default max junction speed for 0 degree acute junction.
            if (cosTheta < 0.95F)
            {
                vmaxJunction = fminf(prevParamSpeed, block._feedrate);
                // Skip and avoid divide by zero for straight junctions at 180 degrees. Limit to min() of nominal speeds.
                if (cosTheta > -0.95F)
                {
                    // Compute maximum junction velocity based on maximum acceleration and junction deviation
                    // Trig half angle identity, always positive
                    float sinThetaD2 = sqrtf(0.5F * (1.0F - cosTheta));
                    vmaxJunction = fminf(vmaxJunction,
                                            sqrtf(axesParams._masterAxisMaxAccMMps2 * junctionDeviation * sinThetaD2 /
                                                (1.0F - sinThetaD2)));
                }
            }
        }
    }
    block._maxEntrySpeedMMps = vmaxJunction;

#ifdef DEBUG_MOTIONPLANNER_DETAILED_INFO
    Log.notice("PrevMoveInQueue %d, JunctionDeviation %F, VmaxJunction %F\n",
                motionPipeline.canGet(), junctionDeviation, vmaxJunction);
#endif

    // Add the element to the pipeline and remember previous element
    motionPipeline.add(block);
    MotionBlockSequentialData prevBlockInfo;
    prevBlockInfo._maxParamSpeedMMps = block._feedrate;
    prevBlockInfo._unitVectors = unitVectors;
    _prevMotionBlock = prevBlockInfo;
    _prevMotionBlockValid = true;

    // Recalculate the whole queue
    recalculatePipeline(motionPipeline, axesParams);

    // Return the change in actuator position
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        curAxisPositions._stepsFromHome.setVal(axisIdx,
                    curAxisPositions._stepsFromHome.getVal(axisIdx) + block.getStepsToTarget(axisIdx));

    return true;
}

void MotionPlanner::debugDumpQueue(const char *comStr, MotionPipeline &motionPipeline, unsigned int minQLen)
{
#ifdef DEBUG_TEST_DUMP
    if (minQLen != -1 && motionPipeline.count() != minQLen)
        return;
    int curIdx = 0;
    while (MotionBlock *pCurBlock = motionPipeline.peekNthFromGet(curIdx))
    {
        Log.notice("%s #%d En %F Ex %F (maxEntry %F, maxParam %F)\n", comStr, curIdx,
                    pCurBlock->_entrySpeedMMps, pCurBlock->_exitSpeedMMps,
                    pCurBlock->_maxEntrySpeedMMps, pCurBlock->_feedrate);
        // Next
        curIdx++;
    }
#endif
}

void MotionPlanner::recalculatePipeline(MotionPipeline &motionPipeline, AxesParams &axesParams)
{
    // The last block in the pipe (most recently added) will have zero exit speed
    // For each block, walking backwards in the queue :
    //    We know the desired exit speed so calculate the entry speed using v^2 = u^2 + 2*a*s
    //    Set the exit speed for the previous block from this entry speed
    // Then walk forward in the queue starting with the first block that can be changed:
    //    Set the entry speed from the previous block (or to 0 if none)
    //    Calculate the max possible exit speed for the block using the same formula as above
    //    Set the entry speed for the next block using this exit speed
    // Finally prepare the block for stepper motor actuation

#ifdef DEBUG_MOTIONPLANNER_DETAILED_INFO
    Log.notice("^^^^^^^^^^^^^^^^^^^^^^^BEFORE RECALC^^^^^^^^^^^^^^^^^^^^^^^^\n");
    motionPipeline.debugShowBlocks(axesParams);
#endif

    // Iterate the block queue in backwards time order stopping at the first block that has its recalculateFlag false
    int blockIdx = 0;
    int earliestBlockToReprocess = -1;
    float previousBlockExitSpeed = 0;
    float followingBlockEntrySpeed = 0;
    MotionBlock *pBlock = NULL;
    MotionBlock *pFollowingBlock = NULL;
    while (true)
    {
        // Get the block at current index
        pBlock = motionPipeline.peekNthFromPut(blockIdx);
        if (pBlock == NULL)
            break;

        // Stop if we don't need to recalculate beyond here or if this block is already executing
        if (pBlock->_isExecuting)
        {
            // Get the exit speed from this executing block to use as the entry speed when going forwards
            previousBlockExitSpeed = pBlock->_exitSpeedMMps;
            break;
        }

        // If entry speed is already at the maximum entry speed then we can stop here as no further changes are
        // going to be made by going back further
        if (pBlock->_entrySpeedMMps == pBlock->_maxEntrySpeedMMps && blockIdx > 1)
        {
#ifdef DEBUG_MOTIONPLANNER_DETAILED_INFO
            Log.notice("++++++++++++++++++++++++++++++ Optimizing block %d, prevSpeed %F\n", blockIdx, pBlock->_exitSpeedMMps);
#endif
            //Get the exit speed from this block to use as the entry speed when going forwards
            previousBlockExitSpeed = pBlock->_exitSpeedMMps;
            break;
        }

        // If there was a following block (remember we're working backwards) then now set the entry speed
        if (pFollowingBlock)
        {
            // Assume for now that that whole block will be deceleration and calculate the max speed we can enter to be able to slow
            // to the exit speed required
            float maxEntrySpeed = MotionBlock::maxAchievableSpeed(axesParams._masterAxisMaxAccMMps2,
                                                                    pFollowingBlock->_exitSpeedMMps, pFollowingBlock->_moveDistPrimaryAxesMM);
            pFollowingBlock->_entrySpeedMMps = fminf(maxEntrySpeed, pFollowingBlock->_maxEntrySpeedMMps);

            // Remember entry speed (to use as exit speed in the next loop)
            followingBlockEntrySpeed = pFollowingBlock->_entrySpeedMMps;
        }

        // Remember the following block for the next pass
        pFollowingBlock = pBlock;

        // Set the block's exit speed to the entry speed of the block after this one
        pBlock->_exitSpeedMMps = followingBlockEntrySpeed;

        // Remember this as the earliest block to reprocess when going forwards
        earliestBlockToReprocess = blockIdx;

        // Next
        blockIdx++;
    }

    // Now iterate in forward time order
    for (blockIdx = earliestBlockToReprocess; blockIdx >= 0; blockIdx--)
    {
        // Get the block to calculate for
        pBlock = motionPipeline.peekNthFromPut(blockIdx);
        if (!pBlock)
            break;

        // Set the entry speed to the previous block exit speed
        // if (pBlock->_entrySpeedMMps > previousBlockExitSpeed)
        pBlock->_entrySpeedMMps = previousBlockExitSpeed;

        // Calculate maximum speed possible for the block - based on acceleration at the best rate
        float maxExitSpeed = pBlock->maxAchievableSpeed(axesParams._masterAxisMaxAccMMps2,
                                                        pBlock->_entrySpeedMMps, pBlock->_moveDistPrimaryAxesMM);
        pBlock->_exitSpeedMMps = fminf(maxExitSpeed, pBlock->_exitSpeedMMps);

        // Remember for next block
        previousBlockExitSpeed = pBlock->_exitSpeedMMps;
    }

    // Recalculate acceleration and deceleration curves
    for (blockIdx = earliestBlockToReprocess; blockIdx >= 0; blockIdx--)
    {
        // Get the block to calculate for
        pBlock = motionPipeline.peekNthFromPut(blockIdx);
        if (!pBlock)
            break;

        // Prepare this block for stepping
        if (pBlock->prepareForStepping(axesParams, false))
        {
            // Check if the block is part of a split block and has at least one more block following it
            // in which case wait until at least two blocks are in the pipeline before locking down the
            // first so that acceleration can be allowed to happen more smoothly
            if ((!pBlock->_blockIsFollowed) || (motionPipeline.count() > 1))
            {
                // No more changes
                pBlock->_canExecute = true;
            }
        }
    }

#ifdef DEBUG_MOTIONPLANNER_DETAILED_INFO
    Log.notice(".................AFTER RECALC.......................\n");
    motionPipeline.debugShowBlocks(axesParams);
#elif DEBUG_MOTIONPLANNER_INFO
    motionPipeline.debugShowTopBlock(axesParams);
#endif
}

// Entry point for adding a motion block for stepwise motion
bool MotionPlanner::moveToStepwise(RobotCommandArgs &args,
                    AxisPosition &curAxisPositions,
                    AxesParams &axesParams, MotionPipeline &motionPipeline)
{
    // Create a block for this movement which will end up on the pipeline
    MotionBlock block;
    block._entrySpeedMMps = 0;
    block._exitSpeedMMps = 0;

    // Find if there are any steps
    bool hasSteps = false;
    float minFeedrateStepsPerSec = 1e8;
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
    {
        // Check if any steps to perform
        int32_t steps = 0;
        if (args.isValid(axisIdx))
        {
            // See if absolute or relative motion
            if (args.getMoveType() == RobotMoveTypeArg_Relative)
                steps = args.getPointSteps().getVal(axisIdx);
            else
                steps = args.getPointSteps().getVal(axisIdx) - curAxisPositions._stepsFromHome.vals[axisIdx];
        }
        // Set steps to target
        if (steps != 0)
        {
            hasSteps = true;
            if (minFeedrateStepsPerSec > axesParams.getMaxStepRatePerSec(axisIdx))
                minFeedrateStepsPerSec = axesParams.getMaxStepRatePerSec(axisIdx);
        }
        // Value (and direction)
        block.setStepsToTarget(axisIdx, steps);
    }

    // Check there are some actual steps
    if (!hasSteps)
        return false;

    // Set unit vector
    block._unitVecAxisWithMaxDist = 1.0;

    // set end-stop check requirements
    block.setEndStopsToCheck(args.getEndstopCheck());

    // Set numbered command index if present
    block.setNumberedCommandIndex(args.getNumberedCommandIndex());

    // feedrate override?
    if (args.isFeedrateValid())
        minFeedrateStepsPerSec = args.getFeedrate();
    block._feedrate = minFeedrateStepsPerSec;

    // Prepare for stepping
    if (block.prepareForStepping(axesParams, true))
    {
        // No more changes
        block._canExecute = true;
    }

    // Add the block
    motionPipeline.add(block);
    _prevMotionBlockValid = true;

    // Return the change in actuator position
    for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        curAxisPositions._stepsFromHome.setVal(axisIdx,
            curAxisPositions._stepsFromHome.getVal(axisIdx) + block.getStepsToTarget(axisIdx));

#ifdef DEBUG_MOTIONPLANNER_INFO
    Log.notice("^^^^^^^^^^^^^^^^^^^^^^^STEPWISE^^^^^^^^^^^^^^^^^^^^^^^^\n");
    motionPipeline.debugShowBlocks(axesParams);
#endif

    return true;
}
