// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "AxisParams.h"
#include "AxisPosition.h"
#include "RobotCommandArgs.h"
#include "MotionPlanner.h"
#include "MotionIO.h"

typedef bool(*ptToActuatorFnType) (MotionPipelineElem& motionElem, AxisFloats& actuatorCoords, AxisParams axisParams[], int numAxes);
typedef void(*actuatorToPtFnType) (AxisFloats& actuatorCoords, AxisFloats& xy, AxisParams axisParams[], int numAxes);
typedef void(*correctStepOverflowFnType) (AxisParams axisParams[], int numAxes);

class MotionHelper
{
public:
	static constexpr float blockDistanceMM_default = 1.0f;
	static constexpr float junctionDeviation_default = 0.05f;
	static constexpr float distToTravelMM_ignoreBelow = 0.01f;
	static constexpr float MINIMUM_PRIMARY_MOVE_DIST_MM = 0.0001f;
	static constexpr int MAX_AXES = 3;

private:
	// Pause
	bool _isPaused;
	bool _wasPaused;
    // Robot dimensions
    float _xMaxMM;
    float _yMaxMM;
	// Number of actual axes of motion
	int _numRobotAxes;
	// Axis parameters
	AxisParams _axisParams[MAX_AXES];
	// Callbacks for coordinate conversion etc
	ptToActuatorFnType _ptToActuatorFn;
	actuatorToPtFnType _actuatorToPtFn;
	correctStepOverflowFnType _correctStepOverflowFn;
	// Relative motion
	bool _moveRelative;
	// Planner used to plan the pipeline of motion
	MotionPlanner _motionPlanner;
	// Axis Current Motion
	AxisPosition _curAxisPosition;
	// Motion IO (Motors and end-stops)
	MotionIO _motionIO;

	// Debug
	unsigned long _debugLastPosDispMs;

public:
	MotionHelper();
	~MotionHelper();

	void setTransforms(ptToActuatorFnType ptToActuatorFn, actuatorToPtFnType actuatorToPtFn,
		correctStepOverflowFnType correctStepOverflowFn);

	void deinit();
	void configure(const char* robotConfigJSON);

	void axisSetHome(int axisIdx);
	long getAxisStepsFromHome(int axisIdx);
	double getStepsPerUnit(int axisIdx)
	{
		if (axisIdx < 0 || axisIdx >= MAX_AXES)
			return 0;
		return _axisParams[axisIdx].stepsPerUnit();
	}

	double getStepsPerRotation(int axisIdx)
	{
		if (axisIdx < 0 || axisIdx >= MAX_AXES)
			return 0;
		return _axisParams[axisIdx]._stepsPerRotation;
	}

	double getUnitsPerRotation(int axisIdx)
	{
		if (axisIdx < 0 || axisIdx >= MAX_AXES)
			return 0;
		return _axisParams[axisIdx]._unitsPerRotation;
	}

	long getHomeOffsetSteps(int axisIdx)
	{
		if (axisIdx < 0 || axisIdx >= MAX_AXES)
			return 0;
		return _axisParams[axisIdx]._homeOffsetSteps;
	}

	AxisParams* getAxisParamsArray()
	{
		return _axisParams;
	}

	bool isMoving();
	bool canAcceptCommand();

	// Pause (or un-pause) all motion
	void pause(bool pauseIt)
	{
		_isPaused = pauseIt;
	}

	// Check if paused
	bool isPaused()
	{
		return _isPaused;
	}

	// Stop
	void stop();


	bool moveTo(RobotCommandArgs& args);
	void setMotionParams(RobotCommandArgs& args);
	void getCurStatus(RobotCommandArgs& args);
	void service(bool processPipeline);

	unsigned long getLastActiveUnixTime()
	{
		return _motionIO.getLastActiveUnixTime();
	}

	void debugShowBlocks();
	int testGetPipelineCount();
	void testGetPipelineElem(int elIdx, MotionPipelineElem& elem);

private:
	bool isInBounds(double v, double b1, double b2)
	{
		return (v > std::min(b1, b2) && v < std::max(b1, b2));
	}

	bool configureRobot(const char* robotConfigJSON);
	bool configureAxis(const char* robotConfigJSON, int axisIdx);
	void pipelineService(bool hasBeenPaused);
};
