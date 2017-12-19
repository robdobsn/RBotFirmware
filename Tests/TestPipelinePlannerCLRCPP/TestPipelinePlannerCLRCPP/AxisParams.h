// RBotFirmware
// Rob Dobson 2017

#pragma once

class AxisParams
{
public:
	static constexpr float maxSpeed_default = 100.0f;
	static constexpr float acceleration_default = 100.0f;
	static constexpr float minNsBetweenSteps_default = 10000.0f;
	static constexpr float stepsPerRotation_default = 1.0f;
	static constexpr float unitsPerRotation_default = 1.0f;
	static constexpr float homeOffsetVal_default = 0.0f;
	static constexpr long homeOffsetSteps_default = 0;

	// Parameters
	float _maxSpeed;
	float _maxAcceleration;
	float _minNsBetweenSteps;
	float _stepsPerRotation;
	float _unitsPerRotation;
	bool _minValValid;
	float _minVal;
	bool _maxValValid;
	float _maxVal;
	bool _isPrimaryAxis;
	bool _isDominantAxis;
	// A servo axis is one which does not require blockwise stepping to a destination
	bool _isServoAxis;
	float _homeOffsetVal;
	long _homeOffsetSteps;

public:
	AxisParams()
	{
		_maxSpeed = maxSpeed_default;
		_maxAcceleration = acceleration_default;
		_minNsBetweenSteps = minNsBetweenSteps_default;
		_stepsPerRotation = stepsPerRotation_default;
		_unitsPerRotation = unitsPerRotation_default;
		_minValValid = false;
		_maxValValid = false;
		_isPrimaryAxis = false;
		_isDominantAxis = false;
		_isServoAxis = false;
		_homeOffsetVal = homeOffsetVal_default;
		_homeOffsetSteps = homeOffsetSteps_default;
	}

	float stepsPerUnit()
	{
		if (_unitsPerRotation != 0)
			return _stepsPerRotation / _unitsPerRotation;
		return 1;
	}

	void set(float maxSpeed, float maxAcceleration, float minNsBetweenSteps, float stepsPerRotation,
		float unitsPerRotation, bool minValValid, float minVal, bool maxValValid, float maxVal,
				bool isDominantAxis, bool isPrimaryAxis)
	{
		_maxSpeed = maxSpeed;
		_maxAcceleration = maxAcceleration;
		_minNsBetweenSteps = minNsBetweenSteps;
		_stepsPerRotation = stepsPerRotation;
		_unitsPerRotation = unitsPerRotation;
		_minValValid = minValValid;
		_minVal = minVal;
		_maxValValid = maxValValid;
		_maxVal = maxVal;
		_isDominantAxis = isDominantAxis;
		_isPrimaryAxis = isPrimaryAxis;
	}
};
