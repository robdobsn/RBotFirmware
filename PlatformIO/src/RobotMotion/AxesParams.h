// RBotFirmware
// Rob Dobson 2016-18

#pragma once

#include "RobotConsts.h"
#include "AxisParams.h"
#include "AxisValues.h"

class AxesParams
{
  private:
    // Axis parameters
    AxisParams _axisParams[RobotConsts::MAX_AXES];
    // Master axis
    int _masterAxisIdx;

  public:
    // Cache values for master axis as they are used frequently in the planner
    float _masterAxisMaxAccMMps2;
    // Cache max step rate
    AxisFloats _maxStepRatesPerSec;

  public:
    AxesParams()
    {
        clearAxes();
    }

    void clearAxes()
    {
        _masterAxisIdx = -1;
        _masterAxisMaxAccMMps2 = AxisParams::acceleration_default;
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
            _axisParams[axisIdx].clear();
    }

    float getStepsPerUnit(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return AxisParams::stepsPerRot_default / AxisParams::unitsPerRot_default;
        return _axisParams[axisIdx].stepsPerUnit();
    }

    float getStepsPerRot(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return AxisParams::stepsPerRot_default;
        return _axisParams[axisIdx]._stepsPerRot;
    }

    float getunitsPerRot(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return AxisParams::unitsPerRot_default;
        return _axisParams[axisIdx]._unitsPerRot;
    }

    long gethomeOffSteps(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return 0;
        return _axisParams[axisIdx]._homeOffSteps;
    }

    void sethomeOffSteps(int axisIdx, long newVal)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return;
        _axisParams[axisIdx]._homeOffSteps = newVal;
    }

    float getHomeOffsetVal(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return 0;
        return _axisParams[axisIdx]._homeOffsetVal;
    }

    AxisParams *getAxisParamsArray()
    {
        return _axisParams;
    }

    bool getMaxVal(int axisIdx, float &maxVal)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return false;
        if (!_axisParams[axisIdx]._maxValValid)
            return false;
        maxVal = _axisParams[axisIdx]._maxVal;
        return true;
    }

    bool getMinVal(int axisIdx, float &minVal)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return false;
        if (!_axisParams[axisIdx]._minValValid)
            return false;
        minVal = _axisParams[axisIdx]._minVal;
        return true;
    }

    float getMaxSpeed(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return AxisParams::maxSpeed_default;
        return _axisParams[axisIdx]._maxSpeedMMps;
    }

    float getMinSpeed(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return AxisParams::minSpeedMMps_default;
        return _axisParams[axisIdx]._minSpeedMMps;
    }

    float getMaxStepRatePerSec(int axisIdx, bool forceRecalc = false)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return AxisParams::maxRPM_default * AxisParams::stepsPerRot_default / 60;
        if (forceRecalc)
            return _axisParams[axisIdx]._maxRPM * _axisParams[axisIdx]._stepsPerRot / 60;
        return _maxStepRatesPerSec.getVal(axisIdx);
    }

    float getMaxAccel(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return AxisParams::acceleration_default;
        return _axisParams[axisIdx]._maxAccelMMps2;
    }

    bool isPrimaryAxis(int axisIdx)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return false;
        return _axisParams[axisIdx]._isPrimaryAxis;
    }

    bool ptInBounds(AxisFloats &pt, bool correctValueInPlace)
    {
        bool wasValid = true;
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
            wasValid = wasValid && _axisParams[axisIdx].ptInBounds(pt._pt[axisIdx], correctValueInPlace);
        return wasValid;
    }

    bool configureAxis(const char *robotConfigJSON, int axisIdx, String &axisJSON)
    {
        if (axisIdx < 0 || axisIdx >= RobotConsts::MAX_AXES)
            return false;

        // Get params
        String axisIdStr = "axis" + String(axisIdx);
        axisJSON = RdJson::getString(axisIdStr.c_str(), "{}", robotConfigJSON);
        if (axisJSON.length() == 0 || axisJSON.equals("{}"))
            return false;

        // Set the axis parameters
        _axisParams[axisIdx].setFromJSON(axisJSON.c_str());
        _axisParams[axisIdx].debugLog(axisIdx);

        // Find the master axis (dominant one, or first primary - or just first)
        setMasterAxis(axisIdx);

        // Cache axis max step rate
        for (int axisIdx = 0; axisIdx < RobotConsts::MAX_AXES; axisIdx++)
        {
            _maxStepRatesPerSec.setVal(axisIdx, getMaxStepRatePerSec(axisIdx, true));
        }
        return true;
    }

    // Set the master axis either to the dominant axis (if there is one)
    // or just the first one found
    void setMasterAxis(int fallbackAxisIdx)
    {
        int dominantIdx = -1;
        int primaryIdx = -1;
        for (int i = 0; i < RobotConsts::MAX_AXES; i++)
        {
            if (_axisParams[i]._isDominantAxis)
            {
                dominantIdx = i;
                break;
            }
            if (_axisParams[i]._isPrimaryAxis)
            {
                if (primaryIdx == -1)
                    primaryIdx = i;
            }
        }
        if (dominantIdx != -1)
            _masterAxisIdx = dominantIdx;
        else if (primaryIdx != -1)
            _masterAxisIdx = primaryIdx;
        else if (_masterAxisIdx == -1)
            _masterAxisIdx = fallbackAxisIdx;

        // Cache values for master axis
        _masterAxisMaxAccMMps2 = getMaxAccel(_masterAxisIdx);
    }
};
