#pragma once

#include "MotionPipeline.h"

typedef bool(*ptToActuatorFnType) (AxisFloats& pt, AxisFloats& actuatorCoords, AxisParams axisParams[], int numAxes);
typedef void(*actuatorToPtFnType) (AxisFloats& actuatorCoords, AxisFloats& pt, AxisParams axisParams[], int numAxes);
typedef void(*correctStepOverflowFnType) (AxisParams axisParams[], int numAxes);

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
		double maxParamSpeedMMps = -1;
		if (args.feedrateValid)
			maxParamSpeedMMps = args.feedrateVal;

		// Find the unit vectors
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			if (axesParams.isPrimaryAxis(axisIdx))
			{
				block.unitVectors._pt[axisIdx] = float(deltas[axisIdx] / moveDist);
				// Check that the move speed doesn't exceed max
				if (axesParams.getMaxSpeed(axisIdx) > 0)
				{
					if (maxParamSpeedMMps > 0)
					{
						double axisSpeed = fabs(block.unitVectors._pt[axisIdx] * maxParamSpeedMMps);
						if (axisSpeed > axesParams.getMaxSpeed(axisIdx))
						{
							maxParamSpeedMMps *= axisSpeed / axesParams.getMaxSpeed(axisIdx);
						}
					}
					else
					{
						maxParamSpeedMMps = axesParams.getMaxSpeed(axisIdx);
					}
				}
			}
		}

		// Calculate move time
		double reciprocalTime = maxParamSpeedMMps / moveDist;
		// Log.trace("Feedrate %0.3f, reciprocalTime %0.3f", maxParamSpeedMMps, reciprocalTime);

		// Check speed limits for each axis individually
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			// Speed and time
			double axisDist = fabs(args.pt._pt[axisIdx] - curAxisPositions._axisPositionMM._pt[axisIdx]);
			if (axisDist == 0)
				continue;
			double axisReqdAcc = axisDist * reciprocalTime;
			if (axisReqdAcc > axesParams.getMaxAccel(axisIdx))
			{
				maxParamSpeedMMps *= axesParams.getMaxAccel(axisIdx) / axisReqdAcc;
				reciprocalTime = maxParamSpeedMMps / moveDist;
			}
		}

		// Log.trace("Feedrate %0.3f, reciprocalTime %0.3f", maxParamSpeedMMps, reciprocalTime);

		// Store values in the block
		block._maxParamSpeedMMps = float(maxParamSpeedMMps);
		block._moveDistPrimaryAxesMM = float(moveDist);

		// Find if there are any steps
		bool hasSteps = false;

		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
		{
			// Check if any actual steps to perform
			float stepsFloat = destActuatorCoords._pt[axisIdx] - curAxisPositions._stepsFromHome._pt[axisIdx];
			int steps = int(ceilf(stepsFloat));
			if (steps != 0)
				hasSteps = true;
			// Value (and direction)
			block._axisStepsToTarget.setVal(axisIdx, steps);
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

		// Invalidate the prev element if the pipeline becomes empty
		if (!motionPipeline.canGet())
			_prevMotionBlockValid = false;

		// For primary axis moves compute the vmaxJunction
		// But only if there are still elements in the queue
		// If not assume the robot is idle and start the move
		// from scratch
		if (isAPrimaryMove && _prevMotionBlockValid)
		{
			float prevParamSpeed = isAPrimaryMove ? _prevMotionBlock._maxParamSpeedMMps : 0;
			if (junctionDeviation > 0.0f && prevParamSpeed > 0.0f)
			{
				// Compute cosine of angle between previous and current path. (prev_unit_vec is negative)
				// NOTE: Max junction velocity is computed without sin() or acos() by trig half angle identity.
				float cosTheta = -_prevMotionBlock._unitVectors.X() * block.unitVectors.X()
					- _prevMotionBlock._unitVectors.Y() * block.unitVectors.Y()
					- _prevMotionBlock._unitVectors.Z() * block.unitVectors.Z();

				// Skip and use default max junction speed for 0 degree acute junction.
				if (cosTheta < 0.95F) {
					vmaxJunction = fminf(prevParamSpeed, block._maxParamSpeedMMps);
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

		// Log.trace("PrevMoveInQueue %d, JunctionDeviation %0.3f, VmaxJunction %0.3f", motionPipeline.canGet(), junctionDeviation, vmaxJunction);

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
		prevBlockInfo._maxParamSpeedMMps = block._maxParamSpeedMMps;
		prevBlockInfo._unitVectors = block.unitVectors;
		_prevMotionBlock = prevBlockInfo;
		_prevMotionBlockValid = true;

		// Recalculate the whole queue
		recalculatePipeline(motionPipeline, axesParams);

		// Return the actual change in actuator position
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
			curAxisPositions._stepsFromHome.setVal(axisIdx, curAxisPositions._stepsFromHome.getVal(axisIdx) + block._axisStepsToTarget.getVal(axisIdx));

		return true;
	}

	void recalculatePipeline(MotionPipeline& motionPipeline, AxesParams& axesParams)
	{
		// A newly added block is decel limited as exit speed is forced to zero in case it really is the last block ever added
		// We find its max entry speed given zero exit speed
		//
		// For each block, walking backwards in the queue :
		//    if max entry speed == current entry speed
		//    then we can set recalculate to false, since clearly adding another block didn't allow us to enter faster
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
		MotionBlock* pBlock = NULL;
		MotionBlock* pPrevElem = NULL;
		MotionBlock* pNext = NULL;
		if (motionPipeline.peekNthFromPut(elemIdxFromPutPos) != NULL)
		{
			pBlock = motionPipeline.peekNthFromPut(elemIdxFromPutPos);
			while (!pBlock->_isExecuting && pBlock->_recalculateFlag)
			{
				// Get the max entry speed
				pBlock->calcMaxSpeedReverse(entrySpeed, axesParams);
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
					pPrevElem->calculateTrapezoid(axesParams);
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
				pPrevElem->calculateTrapezoid(axesParams);
				//Log.trace("Forward pass #%d after trapezoid entrySpeed %0.3f, exitSpeed %0.3f", elemIdxFromPutPos+1, pPrevElem->_entrySpeedMMps, pPrevElem->_exitSpeedMMps);
			}

		}
	}
};