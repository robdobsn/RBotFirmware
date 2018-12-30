// RBotFirmware
// Rob Dobson 2018

#include <Arduino.h>
#include <ArduinoLog.h>
#include "EvaluatorThetaRhoLine.h"
#include "RdJson.h"
#include "Utils.h"
#include "../WorkManager.h"

static const char* MODULE_PREFIX = "EvaluatorThetaRhoLine: ";

EvaluatorThetaRhoLine::EvaluatorThetaRhoLine()
{
    _inProgress = false;
    _curStep = 0;
    _stepAngle = M_PI / 64;
    _curTheta = 0;
    _curRho = 0;
    _continueFromPrevious = true;
    _prevTheta = 0;
    _prevRho = 0;
}

void EvaluatorThetaRhoLine::setConfig(const char* configStr)
{
    // Set the theta-rho angle step
    _stepAngle = RdJson::getDouble("thrStepDegs", 5, configStr) * M_PI / 180;
    _continueFromPrevious = RdJson::getLong("thrContinue", 1, configStr) != 0;
    Log.trace("%ssetConfig StepAngleDegrees %F continueFromPrevious %s\n", MODULE_PREFIX, 
            _stepAngle, _continueFromPrevious ? "Y" : "N");
}

// Is Busy
bool EvaluatorThetaRhoLine::isBusy()
{
    return _inProgress;
}

const char* EvaluatorThetaRhoLine::getConfig()
{
    return "";
}

// Check if valid
bool EvaluatorThetaRhoLine::isValid(WorkItem& workItem)
{
    // Check if theta-rho
    String cmdStr = workItem.getString();
    cmdStr.trim();
    return cmdStr.startsWith("_THRLINE0_") || cmdStr.startsWith("_THRLINEN_");
}

// Process WorkItem
bool EvaluatorThetaRhoLine::execWorkItem(WorkItem& workItem)
{
    // Extract the details
    String cmdStr = workItem.getString();
    String thetaStr = Utils::getNthField(cmdStr.c_str(), 1, '/');
    String rhoStr = Utils::getNthField(cmdStr.c_str(), 2, '/');
    double newTheta = atof(thetaStr.c_str());
    double newRho = atof(rhoStr.c_str());
    if (cmdStr.startsWith("_THRLINE0_"))
    {
        _prevRho = newRho;
        if (_continueFromPrevious)
        {
            _thetaStartOffset = newTheta - _prevTheta;
        }
        else
        {
            _prevTheta = newTheta;
            _thetaStartOffset = 0;
            return true;
        }
    }
    double deltaTheta = newTheta - _thetaStartOffset - _prevTheta;
    double absDeltaTheta = abs(deltaTheta);
    _thetaInc = deltaTheta >= 0 ? _stepAngle : -_stepAngle;
    double deltaRho = newRho - _curRho;
    if (absDeltaTheta < _stepAngle)
    {
        _thetaInc = deltaTheta;
        _interpolateSteps = 1;
        _rhoInc = deltaRho;
    }
    else
    {
        _interpolateSteps = int(ceilf(absDeltaTheta / _stepAngle));
        if (_interpolateSteps <= 0)
            return true;
        _rhoInc = deltaRho * _stepAngle / absDeltaTheta;
    }
    _curTheta = _prevTheta;
    _curRho = _prevRho;
    _prevTheta = newTheta;
    _prevRho = newRho;
    _curStep = 0;
    _inProgress = true;
    Log.trace("%sexecWorkItem Theta %F Rho %F CurTheta %F CurRho %F TotalSteps %d ThetaInc %F RhoInc %F AbsDeltaTheta %F StepAng %F\n", MODULE_PREFIX, 
            newTheta, newRho, _curTheta, _curRho, _interpolateSteps, _thetaInc, _rhoInc, absDeltaTheta, _stepAngle);
    return true;
}

void EvaluatorThetaRhoLine::service(WorkManager* pWorkManager)
{
    // Check in progress
    if (!_inProgress)
        return;

    // See if we can add to the queue
    if (!pWorkManager->canAcceptWorkItem())
        return;

    // Next iteration
    char lineBuf[100];
    sprintf(lineBuf, "G0 U%0.5f V%0.5f", _curTheta, _curRho);
    String retStr;
    WorkItem workItem(lineBuf);
    pWorkManager->addWorkItem(workItem, retStr);
    Log.trace("%sservice %s\n", MODULE_PREFIX, lineBuf);

    // Check complete
    _curStep++;
    if (_curStep >= _interpolateSteps)
    {
        Log.trace("%sservice finished\n", MODULE_PREFIX);
        _inProgress = false;
        return;
    }

    // Inc
    _curTheta += _thetaInc;
    _curRho += _rhoInc;
}

void EvaluatorThetaRhoLine::stop()
{
    _inProgress = false;
}
