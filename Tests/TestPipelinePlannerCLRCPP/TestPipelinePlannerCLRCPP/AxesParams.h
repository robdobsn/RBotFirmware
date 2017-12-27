#pragma once

#include "RobotConsts.h"
#include "AxisParams.h"

class AxesParams
{
private:
	// Axis parameters
	AxisParams _axisParams[RobotConsts::MAX_AXES];
	// Master axis
	int _masterAxisIdx;


public:

	AxesParams()
	{
		clearAxes();
	}

	void clearAxes()
	{
		_masterAxisIdx = -1;
		for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
			_axisParams[axisIdx].clear();
	}

	float getStepsPerUnit(int axisIdx)
	{
		if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
			return 0;
		return _axisParams[axisIdx].stepsPerUnit();
	}

	float getStepsPerRotation(int axisIdx)
	{
		if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
			return 0;
		return _axisParams[axisIdx]._stepsPerRotation;
	}

	float getUnitsPerRotation(int axisIdx)
	{
		if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
			return 0;
		return _axisParams[axisIdx]._unitsPerRotation;
	}

	long getHomeOffsetSteps(int axisIdx)
	{
		if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
			return 0;
		return _axisParams[axisIdx]._homeOffsetSteps;
	}

	AxisParams* getAxisParamsArray()
	{
		return _axisParams;
	}

	float getMaxSpeed(int axisIdx)
	{
		if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
			return 0;
		return _axisParams[axisIdx]._maxSpeedMMps;
	}

	float getMaxAccel(int axisIdx)
	{
		if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
			return 0;
		return _axisParams[axisIdx]._maxAccelMMps2;
	}

	bool isPrimaryAxis(int axisIdx)
	{
		if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
			return 0;
		return _axisParams[axisIdx]._isPrimaryAxis;
	}

	float getMasterMaxAccel()
	{
		if (_masterAxisIdx < 0 || _masterAxisIdx >= RobotConsts::MAX_AXES)
			return 0;
		return _axisParams[_masterAxisIdx]._maxAccelMMps2;
	}

	bool configureAxis(const char* robotConfigJSON, int axisIdx, String& axisJSON)
	{
		if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
			return false;

		// Get params
		String axisIdStr = "axis" + String(axisIdx);
		axisJSON = RdJson::getString(axisIdStr, "{}", robotConfigJSON);
		if (axisJSON.length() == 0 || axisJSON.equals("{}"))
			return false;

		// Set the axis parameters
		_axisParams[axisIdx].setFromJSON(axisJSON.c_str());
		_axisParams[axisIdx].debugLog(axisIdx);

		// Find the master axis (dominant one, or first primary - or just first)
		setMasterAxis(axisIdx);

		return true;
	}

	// Set the master axis either to the dominant axis (if there is one)
	// or from the first primary axis (if one is specified) or just the first one found
	void setMasterAxis(int fallbackAxisIdx)
	{
		int dominantIdx = -1;
		int firstPrimaryIdx = -1;
		for (int i = 0; i < RobotConsts::MAX_AXES; i++)
		{
			if (_axisParams[i]._isDominantAxis)
			{
				dominantIdx = i;
				break;
			}
			if (firstPrimaryIdx = -1 && _axisParams[i]._isPrimaryAxis)
			{
				firstPrimaryIdx = i;
			}
		}
		if (dominantIdx != -1)
			_masterAxisIdx = dominantIdx;
		else if (firstPrimaryIdx != -1)
			_masterAxisIdx = firstPrimaryIdx;
		else if (_masterAxisIdx == -1)
			_masterAxisIdx = fallbackAxisIdx;
	}
};
