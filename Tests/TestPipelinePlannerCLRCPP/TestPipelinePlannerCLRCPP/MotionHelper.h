// RBotFirmware
// Rob Dobson 2016

#pragma once

//#define USE_MOTION_ISR_MANAGER 1

#include "Application.h"
#include "AxisParams.h"
#include "AxisMotion.h"
#include "MotionPipeline.h"
#include "RobotCommandArgs.h"
#include "MotionPlanner.h"

typedef bool(*ptToActuatorFnType) (MotionPipelineElem& motionElem, AxisFloats& actuatorCoords, AxisParams axisParams[], int numAxes);
typedef void(*actuatorToPtFnType) (AxisFloats& actuatorCoords, AxisFloats& xy, AxisParams axisParams[], int numAxes);
typedef void(*correctStepOverflowFnType) (AxisParams axisParams[], int numAxes);

class MotionHelper
{
public:
	static const int MAX_ENDSTOPS_PER_AXIS = 2;
	static constexpr double stepDisableSecs_default = 60.0;
	static constexpr double blockDistanceMM_default = 1.0;
	static constexpr double distToTravelMM_ignoreBelow = 0.01;
	static constexpr float MINIMUM_PRIMARY_MOVE_DIST_MM = 0.0001f;

private:
	// Pause
	bool _isPaused;
	bool _wasPaused;
	// Axis parameters
	AxisParams _axisParams[MAX_AXES];
	// Step enable
	int _stepEnablePin;
	bool _stepEnableActiveLevel = true;
	// Motor enable
	float _stepDisableSecs;
	bool _motorsAreEnabled;
	unsigned long _motorEnLastMillis;
	unsigned long _motorEnLastUnixTime;
	// Callbacks for coordinate conversion etc
	ptToActuatorFnType _ptToActuatorFn;
	actuatorToPtFnType _actuatorToPtFn;
	correctStepOverflowFnType _correctStepOverflowFn;
	// Number of actual axes of motion
	int _numRobotAxes;
	// Block distance
	float _blockDistanceMM;
	// Junction deviation
	float _junctionDeviation;
	// Minimum planner speed mm/s
	float _minimumPlannerSpeedMMps;
	// Relative motion
	bool _moveRelative;
	// Planner used to plan the pipeline of motion
	MotionPlanner _motionPlanner;
	// Motion pipeline
	MotionPipeline _motionPipeline;
	// Axis Current Motion
	AxisMotion _axisMotion;
	// Previous pipeline element
	bool _prevPipelineElemValid;
	MotionPipelineElem _prevPipelineElem;

	// Debug
	unsigned long _debugLastPosDispMs;

private:
	bool isInBounds(double v, double b1, double b2)
	{
		return (v > std::min(b1, b2) && v < std::max(b1, b2));
	}

	bool configure(const char* robotConfigJSON);
	bool configureAxis(const char* robotConfigJSON, int axisIdx);
	void pipelineService(bool hasBeenPaused);
	bool addBlockToPlanner(MotionPipelineElem& elem);
	void recalculatePipeline();

public:
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

	void configMotionPipeline();

	MotionHelper();
	~MotionHelper();

	void setTransforms(ptToActuatorFnType ptToActuatorFn, actuatorToPtFnType actuatorToPtFn,
		correctStepOverflowFnType correctStepOverflowFn);

	void deinit();
	void enableMotors(bool en, bool timeout);
	void setAxisParams(const char* robotConfigJSON);
	void axisSetHome(int axisIdx);
	void step(int axisIdx, bool direction);
	void jump(int axisIdx, long targetStepsFromHome);
	void jumpHome(int axisIdx);
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

	// Endstops
	bool isEndStopValid(int axisIdx, int endStopIdx);
	bool isAtEndStop(int axisIdx, int endStopIdx);
	bool moveTo(RobotCommandArgs& args);
	void setMotionParams(RobotCommandArgs& args);
	void getCurStatus(RobotCommandArgs& args);
	void service(bool processPipeline);

	unsigned long getLastActiveUnixTime()
	{
		return _motorEnLastUnixTime;
	}

	void debugShowBlocks();

};
