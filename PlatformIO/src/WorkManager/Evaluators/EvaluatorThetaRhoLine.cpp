// RBotFirmware
// Rob Dobson 2018

#include <Arduino.h>
#include <ArduinoLog.h>
#include "EvaluatorThetaRhoLine.h"
#include "RdJson.h"
#include "Utils.h"
#include "../WorkManager.h"

// #define THETA_RHO_DEBUG 1

static const char *MODULE_PREFIX = "EvaluatorThetaRhoLine: ";

EvaluatorThetaRhoLine::EvaluatorThetaRhoLine(WorkManager& workManager) :
                            _workManager(workManager)
{
    _inProgress = false;
    _curStep = 0;
    _stepAngle = AxisUtils::r2d(DEFAULT_STEP_ANGLE);
    _stepAdaptation = true;
    _curTheta = 0;
    _curRho = 0;
    _continueFromPrevious = true;
    _prevTheta = 0;
    _prevRho = 0;
}

void EvaluatorThetaRhoLine::setConfig(const char *configStr)
{
    // Set the theta-rho angle step
    _stepAngle = AxisUtils::d2r(RdJson::getDouble("thrStepDegs", AxisUtils::r2d(DEFAULT_STEP_ANGLE), configStr));
    _stepAdaptation = RdJson::getLong("thrStepAdaptation", 1, configStr) != 0;
    _continueFromPrevious = RdJson::getLong("thrContinue", 1, configStr) != 0;
    Log.trace("%ssetConfig StepAngleDegrees %F StepAdaptation %s continueFromPrevious %s\n", MODULE_PREFIX,
              _stepAngle, _stepAdaptation ? "Y" : "N", _continueFromPrevious ? "Y" : "N");
}

// Is Busy
bool EvaluatorThetaRhoLine::isBusy()
{
    return _inProgress;
}

const char *EvaluatorThetaRhoLine::getConfig()
{
    return "";
}

// Check if valid
bool EvaluatorThetaRhoLine::isValid(WorkItem &workItem)
{
    // Check if theta-rho
    String cmdStr = workItem.getString();
    cmdStr.trim();
    return cmdStr.startsWith("_THRLINE");
}

// Process WorkItem
bool EvaluatorThetaRhoLine::execWorkItem(WorkItem &workItem)
{
    // Extract the details
    String thetaStr = Utils::getNthField(workItem.getCString(), 1, '/');
    String rhoStr = Utils::getNthField(workItem.getCString(), 2, '/');
    double newTheta = atof(thetaStr.c_str());
    double newRho = atof(rhoStr.c_str());
#ifdef THETA_RHO_DEBUG
    Log.trace("%sexecWorkItem %s\n", MODULE_PREFIX,
              workItem.getCString());
#endif

    // Check for an uninterpolated line
    if (workItem.getString().startsWith("_THRLINE_"))
    {
        // Next iteration
        char lineBuf[100];
        sprintf(lineBuf, "G0 U%0.6f V%0.6f", newTheta, newRho);
        String retStr;
        WorkItem workItem(lineBuf);
#ifdef THETA_RHO_DEBUG
        Log.trace("%sexecWorkItem thrNonInterp %s\n", MODULE_PREFIX, lineBuf);
#endif
        _workManager.addWorkItem(workItem, retStr);
        return true;
    }

    // Check for first line of interpolated file
    if (workItem.getString().startsWith("_THRLINE0_"))
    {
        if (_continueFromPrevious)
        {
            _thetaStartOffset = newTheta - _prevTheta;
        }
        else
        {
            _thetaStartOffset = 0;
        }
        _prevTheta = newTheta;
        _prevRho = newRho;
        return true;
    }
    double deltaTheta = newTheta - _thetaStartOffset - _prevTheta;
    double absDeltaTheta = abs(deltaTheta);
    double adaptedStepAngle = _stepAngle;
    if (_stepAdaptation)
    {
        double avgRho = (fabs(newRho) + fabs(_prevRho)) / 2;
        if (avgRho > 1)
            avgRho = 1;
        double maxStepAngle = _stepAngle * 16;
        if (maxStepAngle > M_PI / 2)
            maxStepAngle = M_PI / 2;
        double minStepAngle = _stepAngle / 4;
        if (avgRho > RHO_AT_DEFAULT_STEP_ANGLE)
        {
            adaptedStepAngle = ((avgRho - RHO_AT_DEFAULT_STEP_ANGLE) / (1 - RHO_AT_DEFAULT_STEP_ANGLE)) * 
                    (minStepAngle - _stepAngle) + _stepAngle;
        }
        else
        {
            adaptedStepAngle = (avgRho / RHO_AT_DEFAULT_STEP_ANGLE) * 
                    (_stepAngle - maxStepAngle) + maxStepAngle;
        }
    }
    _thetaInc = deltaTheta >= 0 ? adaptedStepAngle : -adaptedStepAngle;
    double deltaRho = newRho - _prevRho;
    if (absDeltaTheta < adaptedStepAngle)
    {
        _thetaInc = deltaTheta;
        _interpolateSteps = 1;
        _rhoInc = deltaRho;
    }
    else
    {
        _interpolateSteps = int(floor(absDeltaTheta / adaptedStepAngle));
        if (_interpolateSteps < 1)
            return true;
        _rhoInc = deltaRho * adaptedStepAngle / absDeltaTheta;
    }
    _curTheta = _prevTheta;
    _curRho = _prevRho;
    _prevTheta = newTheta;
    _prevRho = newRho;
    _curStep = 0;
    _inProgress = true;
#ifdef THETA_RHO_DEBUG
    char debugStr[200];
    sprintf(debugStr, "Theta %8.6f Rho %8.6f CurTheta %8.6f CurRho %8.6f TotalSteps %d ThetaInc %8.6f RhoInc %8.6f AbsDeltaTheta %8.6f StepAng %8.6f AdaptedStepAng %8.6f",
            newTheta, newRho, _curTheta, _curRho, _interpolateSteps, _thetaInc, _rhoInc, absDeltaTheta, _stepAngle, adaptedStepAngle);
    Log.trace("%sexecWorkItem %s\n", MODULE_PREFIX, debugStr);
#endif
    return true;
}

void EvaluatorThetaRhoLine::service()
{
    // Process multiple if possible
    for (int i = 0; i < PROCESS_STEPS_PER_SERVICE; i++)
    {
        // Check in progress
        if (!_inProgress)
            return;

        if (_curStep >= _interpolateSteps)
        {
#ifdef THETA_RHO_DEBUG
            Log.trace("%sservice finished\n", MODULE_PREFIX);
#endif
            _inProgress = false;
            return;
        }

        // See if we can add to the queue
        if (!_workManager.canAcceptWorkItem())
            return;

        // Step
        _curStep++;

        // Inc
        _curTheta += _thetaInc;
        _curRho += _rhoInc;

        // Next iteration
        char lineBuf[100];
        sprintf(lineBuf, "G0 U%0.6f V%0.6f", _curTheta, _curRho);
        String retStr;
        WorkItem workItem(lineBuf);
#ifdef THETA_RHO_DEBUG
        Log.trace("%sservice %s\n", MODULE_PREFIX, lineBuf);
#endif
        _workManager.addWorkItem(workItem, retStr);
    }
}

void EvaluatorThetaRhoLine::stop()
{
    _inProgress = false;
}
