#pragma once

#include "MotionPipeline.h"

class MotionPlanner
{
private:
	// Motion pipeline
	MotionPipeline* _pMotionPipeline;

public:
	MotionPlanner()
	{
		_pMotionPipeline = NULL;
	}
	void setPipeline(MotionPipeline* pMotionPipeline)
	{
		_pMotionPipeline = pMotionPipeline;
	}
	bool addBlock(MotionPipelineElem& elem, float moveDist, float *unitVec)
	{
		bool hasSteps = false;
		return true;
	}
};