// RBotFirmware
// Rob Dobson 2016-18

#pragma once

//#define USE_OLD_PLANNER_CODE 1
//#define DEBUG_TEST_AB 1
//#define DEBUG_TEST_DUMP 1
//#define DEBUG_BLOCK_TO_DUMP_OR_MINUS1_FOR_ALL 15
//#define SHOW_DEBUG_INFO 1

#include "MotionPipeline.h"

typedef bool(*ptToActuatorFnType) (AxisFloats& pt, AxisFloats& actuatorCoords, AxesParams& axesParams);
typedef void(*actuatorToPtFnType) (AxisFloats& actuatorCoords, AxisFloats& pt, AxesParams& axesParams);
typedef void(*correctStepOverflowFnType) (AxesParams& axesParams);

class MotionPlanner
{
private:
	// Minimum planner speed mm/s
	float _minimumPlannerSpeedMMps;
	// Junction deviation
	float _junctionDeviation;

	// Structure to store details on last processed block
	struct MotionBlockSequentialData
	{
		AxisFloats _unitVectors;
		float _maxParamSpeedMMps;
	};
	// Data on previously processed block
	bool _prevMotionBlockValid;
	MotionBlockSequentialData _prevMotionBlock;

public:
	MotionPlanner()
	{
		_prevMotionBlockValid = false;
		_minimumPlannerSpeedMMps = 0;
		// Configure the motion pipeline - these values will be changed in config
		_junctionDeviation = 0;
	}

	void configure(float junctionDeviation)
	{
		_junctionDeviation = junctionDeviation;
	}

	bool moveTo(RobotCommandArgs& args,
		AxisFloats& destActuatorCoords,
		AxisPosition& curAxisPositions,
		AxesParams& axesParams, MotionPipeline& motionPipeline)
	{
		// Find axis deltas and sum of squares of motion on primary axes
		float deltas[RobotConsts::MAX_AXES];
		bool isAMove = false;
		bool isAPrimaryMove = false;
		float squareSum = 0;
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			deltas[axisIdx] = args.pt._pt[axisIdx] - curAxisPositions._axisPositionMM._pt[axisIdx];
			if (deltas[axisIdx] != 0)
			{
				isAMove = true;
				if (axesParams.isPrimaryAxis(axisIdx))
				{
					squareSum += powf(deltas[axisIdx], 2);
					isAPrimaryMove = true;
				}
			}
		}

		// Distance being moved
		float moveDist = sqrtf(squareSum);

		// Ignore if there is no real movement
		if (!isAMove || moveDist < MotionBlock::MINIMUM_MOVE_DIST_MM)
			return false;

		// Create a block for this movement which will end up on the pipeline
		MotionBlock block;

		// Max speed (may be overridden downwards by feedrate)
		float validFeedrateMMps = 1e8;
		if (args.feedrateValid)
			validFeedrateMMps = args.feedrateVal;

		// Find the unit vectors for the primary axes and check the feedrate
		AxisFloats unitVectors;
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			if (axesParams.isPrimaryAxis(axisIdx))
			{
				// Unit vector calculation
				unitVectors._pt[axisIdx] = float(deltas[axisIdx] / moveDist);

				// Check the feedrate
				float axisSpeedAtRequestedFeedrateMMps = fabsf(unitVectors._pt[axisIdx] * validFeedrateMMps);
				float axisMaxSpeedMMps = axesParams.getMaxSpeed(axisIdx);
				if (axisSpeedAtRequestedFeedrateMMps > axisMaxSpeedMMps)
					validFeedrateMMps = fabsf(axisMaxSpeedMMps / unitVectors._pt[axisIdx]);
			}
		}
	
		// Calculate move time
		double reciprocalTime = validFeedrateMMps / moveDist;
		 //Log.trace("Feedrate %0.3f, reciprocalTime %0.3f", maxParamSpeedMMps, reciprocalTime);

		// Store values in the block
		block._feedrateMMps = float(validFeedrateMMps);
		block._moveDistPrimaryAxesMM = float(moveDist);

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

		// Check there are some actual steps
		if (!hasSteps)
			return false;

		// Comments from Smoothieware!
		// Compute the acceleration rate for the trapezoid generator. Depending on the slope of the line
		// average travel per step event changes. For a line along one axis the travel per step event
		// is equal to the travel/step in the particular axis. For a 45 degree line the steppers of both
		// axes might step for every step event. Travel per step event is then sqrt(travel_x^2+travel_y^2).

		// Compute maximum allowable entry speed at junction by centripetal acceleration approximation.
		// Let a circle be tangent to both previous and current path line segments, where the junction
		// deviation is defined as the distance from the junction to the closest edge of the circle,
		// colinear with the circle center. The circular segment joining the two paths represents the
		// path of centripetal acceleration. Solve for max velocity based on max acceleration about the
		// radius of the circle, defined indirectly by junction deviation. This may be also viewed as
		// path width or max_jerk in the previous grbl version. This approach does not actually deviate
		// from path, but used as a robust way to compute cornering speeds, as it takes into account the
		// nonlinearities of both the junction angle and junction velocity.

		// NOTE however it does not take into account independent axis, in most cartesian X and Y and Z are totally independent
		// and this allows one to stop with little to no decleration in many cases. This is particualrly bad on leadscrew based systems that will skip steps.

		// Junction deviation
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
				float cosTheta = -_prevMotionBlock._unitVectors.X() * unitVectors.X()
					- _prevMotionBlock._unitVectors.Y() * unitVectors.Y()
					- _prevMotionBlock._unitVectors.Z() * unitVectors.Z();

				// Skip and use default max junction speed for 0 degree acute junction.
				if (cosTheta < 0.95F) {
					vmaxJunction = fminf(prevParamSpeed, block._feedrateMMps);
					// Skip and avoid divide by zero for straight junctions at 180 degrees. Limit to min() of nominal speeds.
					if (cosTheta > -0.95F) {
						// Compute maximum junction velocity based on maximum acceleration and junction deviation
						float sinThetaD2 = sqrtf(0.5F * (1.0F - cosTheta)); // Trig half angle identity. Always positive.
						vmaxJunction = fminf(vmaxJunction, sqrtf(axesParams._masterAxisMaxAccMMps2 * junctionDeviation * sinThetaD2 / (1.0F - sinThetaD2)));
					}
				}
			}
		}
		block._maxEntrySpeedMMps = vmaxJunction;

		 Log.trace("PrevMoveInQueue %d, JunctionDeviation %0.3f, VmaxJunction %0.3f", motionPipeline.canGet(), junctionDeviation, vmaxJunction);

		// Calculate max achievable speed using v^2 = u^2 - 2as
		// float maxAchievableSpeedMMps = sqrtf(_minimumPlannerSpeedMMps * _minimumPlannerSpeedMMps + 2.0F * (axesParams._masterAxisMaxAccMMps2) * block._moveDistPrimaryAxesMM);
		// block._entrySpeedMMps = fminf(vmaxJunction, maxAchievableSpeedMMps);

		// Log.trace("MaxAchievableSpeed %0.3f, entrySpeedMMps %0.3f", maxAchievableSpeedMMps, block._entrySpeedMMps);

		// Initialize planner efficiency flags
		// Set flag if block will always reach maximum junction speed regardless of entry/exit speeds.
		// If a block can de/ac-celerate from nominal speed to zero within the length of the block, then
		// the current block and next block junction speeds are guaranteed to always be at their maximum
		// junction speeds in deceleration and acceleration, respectively. This is due to how the current
		// block nominal speed limits both the current and next maximum junction speeds. Hence, in both
		// the reverse and forward planners, the corresponding block junction speed will always be at the
		// the maximum junction speed and may always be ignored for any speed reduction checks.
		// block._canReachJnMax = block._maxParamSpeedMMps <= maxAchievableSpeedMMps;

		// Store the element in the queue and remember previous element
		motionPipeline.add(block);
		MotionBlockSequentialData prevBlockInfo;
		prevBlockInfo._maxParamSpeedMMps = block._feedrateMMps;
		prevBlockInfo._unitVectors = unitVectors;
		_prevMotionBlock = prevBlockInfo;
		_prevMotionBlockValid = true;

		// Recalculate the whole queue
		recalculatePipeline(motionPipeline, axesParams);

		// Return the actual change in actuator position
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
			curAxisPositions._stepsFromHome.setVal(axisIdx, curAxisPositions._stepsFromHome.getVal(axisIdx) + block.getStepsToTarget(axisIdx));

		return true;
	}

#ifdef DEBUG_TEST_AB

	std::vector<MotionBlock> __debugPrevElems;

	void debugCopyQueue(MotionPipeline& motionPipeline)
	{
		__debugPrevElems.clear();
		__debugPrevElems.resize(motionPipeline.count());
		for (unsigned int curIdx = 0; curIdx < motionPipeline.count(); curIdx++)
		{
			MotionBlock* pCurBlock = motionPipeline.peekNthFromGet(curIdx);
			__debugPrevElems[curIdx] = *pCurBlock;
		}
	}

	void debugCompareQueue(const char* comStr, MotionPipeline& motionPipeline)
	{
		for (unsigned int curIdx = 0; curIdx < motionPipeline.count(); curIdx++)
		{
			MotionBlock* pCurBlock = motionPipeline.peekNthFromGet(curIdx);
			if (__debugPrevElems[curIdx]._entrySpeedMMps != pCurBlock->_entrySpeedMMps ||
				__debugPrevElems[curIdx]._exitSpeedMMps != pCurBlock->_exitSpeedMMps)
			{
				Log.trace(">>>> BLOCK DIFFERS %s #%d En %0.3f Ex %0.3f En %0.3f Ex %0.3f", comStr, curIdx,
					__debugPrevElems[curIdx]._entrySpeedMMps, pCurBlock->_entrySpeedMMps, 
					__debugPrevElems[curIdx]._exitSpeedMMps, pCurBlock->_exitSpeedMMps);
			}
		}
	}
#endif

	void dumpQueue(const char* comStr, MotionPipeline& motionPipeline, unsigned int minQLen)
	{
#ifdef DEBUG_TEST_DUMP
		if (minQLen != -1 && motionPipeline.count() != minQLen)
			return;
		int curIdx = 0;
		while (MotionBlock* pCurBlock = motionPipeline.peekNthFromGet(curIdx))
		{
			Log.trace("%s #%d En %0.3f Ex %0.3f (maxEntry %0.3f, maxParam %0.3f)", comStr, curIdx,
				pCurBlock->_entrySpeedMMps, pCurBlock->_exitSpeedMMps,
				pCurBlock->_maxEntrySpeedMMps, pCurBlock->_feedrateMMps);
			// Next
			curIdx++;
		}
#endif
	}


	void recalculatePipeline(MotionPipeline& motionPipeline, AxesParams& axesParams)
	{
		// The last block in the pipe (most recently added) will have zero exit speed
		// For each block, walking backwards in the queue :
		//    We know the desired exit speed so calculate the entry speed using v^2 = u^2 + 2*a*s
		//    If the entry speed is already at maximum value then we can set recalculate to false
		//    since clearly adding another block didn't allow us to enter faster
		//    and thus we don't need to check entry speed for this block any more

#ifdef SHOW_DEBUG_INFO
		Log.trace("\n++++++++++++RECALCULATE PIPELINE\n");
		dumpQueue("NEW", motionPipeline, DEBUG_BLOCK_TO_DUMP_OR_MINUS1_FOR_ALL);
#endif

		// Iterate the block queue in backwards time order stopping at the first block that has its recalculateFlag false
		int blockIdx = 0;
		float followingBlockExitSpeed = 0;
		MotionBlock* pBlock = NULL;
		while (pBlock = motionPipeline.peekNthFromPut(blockIdx))
		{
			// Stop if we don't need to recalculate beyond here or if this block is already executing
			if (pBlock->_isExecuting)
				break;

			// Set the block's exit speed to the entry speed of the block after this one
			pBlock->_exitSpeedMMps = followingBlockExitSpeed;

			// If entry speed is already at the maximum entry speed then we can stop here as no further changes are
			// going to be made by going back further
			if (pBlock->_entrySpeedMMps == pBlock->_maxEntrySpeedMMps)
			{
#ifdef SHOW_DEBUG_INFO
				Log.trace("++++++++++++++++++++++++++++++ Breaking %d", blockIdx);
#endif				
				blockIdx++;
				break;
			}

			// Assume for now that that whole block will be deceleration and calculate the max speed we can enter to be able to slow
			// to the exit speed required
			float maxEntrySpeed = pBlock->maxAchievableSpeed(axesParams._masterAxisMaxAccMMps2, pBlock->_exitSpeedMMps, pBlock->_moveDistPrimaryAxesMM);
			pBlock->_entrySpeedMMps = fminf(maxEntrySpeed, pBlock->_maxEntrySpeedMMps);

			// Remember exit speed for next loop
			followingBlockExitSpeed = pBlock->_entrySpeedMMps;

			// Next
			blockIdx++;
		}

#ifdef SHOW_DEBUG_INFO
		dumpQueue("NEWMid", motionPipeline, DEBUG_BLOCK_TO_DUMP_OR_MINUS1_FOR_ALL);
#endif

		// Get the exit speed for the block just before our forward calculation starts
		// This is either 0 if there are no blocks at all, or the value of the first
		// block we don't need to recalculate
		float prevBlockExitSpeed = 0;
		if (pBlock != NULL)
			prevBlockExitSpeed = pBlock->_exitSpeedMMps;
		int earliestBlockIdx = blockIdx;

#ifdef SHOW_DEBUG_INFO
		Log.trace("=================================EarliestBlockIdx %d", earliestBlockIdx);
#endif
		// Now iterate in forward time order
		for (blockIdx = earliestBlockIdx - 1; blockIdx >= 0; blockIdx--)
		{
			// Get the block to calculate for
			pBlock = motionPipeline.peekNthFromPut(blockIdx);
			if (!pBlock)
				break;

			// Set the entry speed to the previous block exit speed if it is higher
			if (pBlock->_entrySpeedMMps > prevBlockExitSpeed)
				pBlock->_entrySpeedMMps = prevBlockExitSpeed;

			// Calculate maximum speed possible for the block - based on acceleration at the best rate
			float maxExitSpeed = pBlock->maxAchievableSpeed(axesParams._masterAxisMaxAccMMps2, pBlock->_entrySpeedMMps, pBlock->_moveDistPrimaryAxesMM);
			pBlock->_exitSpeedMMps = fminf(maxExitSpeed, pBlock->_exitSpeedMMps);

			// Remember for next block
			prevBlockExitSpeed = pBlock->_exitSpeedMMps;

			//Log.trace("Forward pass #%d element %08x, prev En %0.3f Ex %0.3f. cur En %0.3f Ex %0.3f", blockBeingProcessedIdx, pCurBlock,
			//	pPrevElem->_entrySpeedMMps, pPrevElem->_exitSpeedMMps, pBlock->_entrySpeedMMps, pBlock->_exitSpeedMMps);

		}

		// Get the previous block (or NULL if none)
		MotionBlock* pPrevBlockForStepParams = motionPipeline.peekNthFromPut(earliestBlockIdx);

		// Recalculate trapezoid for blocks that need it
		for (blockIdx = earliestBlockIdx - 1; blockIdx >= 0; blockIdx--)
		{
			// Get the block to calculate for
			pBlock = motionPipeline.peekNthFromPut(blockIdx);
			if (!pBlock)
				break;

			// Calculate trapezoid on this block
			pBlock->prepareForStepping(axesParams, pPrevBlockForStepParams);
			pPrevBlockForStepParams = pBlock;

			//Log.trace("Forward pass #%d after trapezoid entrySpeed %0.3f, exitSpeed %0.3f", blockBeingProcessedIdx +1, 
			//				pPrevElem->_entrySpeedMMps, pPrevElem->_exitSpeedMMps);
		}


#ifdef SHOW_DEBUG_INFO
		dumpQueue("NEW Done", motionPipeline, DEBUG_BLOCK_TO_DUMP_OR_MINUS1_FOR_ALL);
#endif

#ifdef USE_OLD_PLANNER_CODE

		debugCopyQueue(motionPipeline);

		// TEST clear blocks
		pBlock = NULL;
		blockIdx = 0;
		while (pBlock = motionPipeline.peekNthFromPut(blockIdx))
		{
			pBlock->_entrySpeedMMps = 0;
			pBlock->_exitSpeedMMps = 0;
			blockIdx++;
		}

		// A newly added block is decel limited as exit speed is forced to zero in case it really is the last block ever added
		// We find its max entry speed given zero exit speed
		//
		// For each block, walking backwards in the queue :
		//    we know the desired exit speed so calculate the entry speed using v^2 = u^2 + 2*a*s
		//    if max entry speed == current entry speed then we can set recalculate to false
		//    since clearly adding another block didn't allow us to enter faster
		//    and thus we don't need to check entry speed for this block any more
		//
		// Once we find an accel limited block, we must find the max exit speed and walk the queue forwards
		//
		// For each block, walking forwards in the queue :
		//
		//    given the exit speed of the previous block and our own max entry speed
		//    we can tell if we're accel or decel limited (or coasting)
		//
		//    if prev_exit > max_entry
		//    then we're still decel limited. update previous trapezoid with our max entry for prev exit
		//    if max_entry >= prev_exit
		//    then we're accel limited. set recalculate to false, work out max exit speed

		//Log.trace("------recalculatePipeline");
		// Step 1:
		// For each block, given the exit speed and acceleration, find the maximum entry speed
		float entrySpeed = _minimumPlannerSpeedMMps;
		int elemIdxFromPutPos = 0;
		pBlock = NULL;
		MotionBlock* pPrevElem = NULL;
		MotionBlock* pNext = NULL;
		if (motionPipeline.peekNthFromPut(elemIdxFromPutPos) != NULL)
		{
			pBlock = motionPipeline.peekNthFromPut(elemIdxFromPutPos);
			while (!pBlock->_isExecuting)
			{
				// Get the max entry speed
				pBlock->setEntrySpeedFromPassedExitSpeed(entrySpeed, axesParams);
				//Log.trace("Backwards pass #%d element %08x, tryEntrySpeed %0.3f, newEntrySpeed %0.3f", elemIdxFromPutPos, pBlock, entrySpeed, pBlock->_entrySpeedMMps);
				entrySpeed = pBlock->_entrySpeedMMps;
				// Next elem
				pNext = motionPipeline.peekNthFromPut(elemIdxFromPutPos + 1);
				if (!pNext)
					break;
				pBlock = pNext;
				// The exit speed for the previous block up the chain is the entry speed we've just calculated
				pBlock->_exitSpeedMMps = entrySpeed;
				elemIdxFromPutPos++;
			}

			dumpQueue("OLDMid", motionPipeline, DEBUG_BLOCK_TO_DUMP_OR_MINUS1_FOR_ALL);
			Log.trace("ElemIdx %d", elemIdxFromPutPos);

			// Calc exit speed of first block
			pBlock->maximizeExitSpeed(axesParams);

			// Step 2:
			// Pointing to the first block that doesn't need recalculating (or the get block)
			while (true)
			{
				// Save previous
				pPrevElem = pBlock;
				// Next element
				if (elemIdxFromPutPos == 0)
				{
					// Calculate trapezoid on last element
					pPrevElem->calculateStepParams(axesParams);
					//Log.trace("Forward pass cur #%d after trapezoid entrySpeed %0.3f, exitSpeed %0.3f", elemIdxFromPutPos, pPrevElem->_entrySpeedMMps, pPrevElem->_exitSpeedMMps);
					break;
				}
				elemIdxFromPutPos--;
				pBlock = motionPipeline.peekNthFromPut(elemIdxFromPutPos);
				if (!pBlock)
					break;
				// Pass the exit speed of the previous block
				// so this block can decide if it's accel or decel limited and update its fields as appropriate
				pBlock->calcMaxSpeedForward(pPrevElem->_exitSpeedMMps, axesParams);
				//Log.trace("Forward pass #%d element %08x, prev En %0.3f Ex %0.3f. cur En %0.3f Ex %0.3f", elemIdxFromPutPos, pBlock,
				//	pPrevElem->_entrySpeedMMps, pPrevElem->_exitSpeedMMps, pBlock->_entrySpeedMMps, pBlock->_exitSpeedMMps);

				// Set exit speed and calculate trapezoid on this block
				pPrevElem->calculateStepParams(axesParams);
				//Log.trace("Forward pass #%d after trapezoid entrySpeed %0.3f, exitSpeed %0.3f", elemIdxFromPutPos+1, pPrevElem->_entrySpeedMMps, pPrevElem->_exitSpeedMMps);
			}
			dumpQueue("OLDEnd", motionPipeline, DEBUG_BLOCK_TO_DUMP_OR_MINUS1_FOR_ALL);
		}

		debugCompareQueue("", motionPipeline);


#endif
	}
};