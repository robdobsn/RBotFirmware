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
    _stepAngleDegrees = 5;
    _curTheta = 0;
    _curRho = 0;
    _continueFromPrevious = true;
}

void EvaluatorThetaRhoLine::setConfig(const char* configStr)
{
    // Set the theta-rho angle step
    _stepAngleDegrees = RdJson::getDouble("thrStepDegs", 5, configStr);
    _continueFromPrevious = RdJson::getLong("thrContinue", 1, configStr) != 0;
    Log.trace("%ssetConfig StepAngleDegrees %F continueFromPrevious %s\n", MODULE_PREFIX, 
            _stepAngleDegrees, _continueFromPrevious ? "Y" : "N");
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
    double thetaDiff = newTheta - _thetaStartOffset - _curTheta;
    if (cmdStr.startsWith("_THRLINE0_"))
    {
        if (_continueFromPrevious)
        {
            _thetaStartOffset = newTheta - _curTheta;
        }
        else
        {
            _curTheta = newTheta;
            _thetaStartOffset = 0;
            thetaDiff = 0;
        }
        _curRho = newRho;
    }
    double rhoDiff = newRho - _curRho;
    double absThetaDiffDegs = abs(thetaDiff) * 180 / M_PI;
    _totalSteps = int(ceilf(absThetaDiffDegs / _stepAngleDegrees));
    if (_totalSteps <= 0)
        return true;
    _curStep = 0;
    _thetaInc = thetaDiff / _totalSteps;
    _rhoInc = rhoDiff / _totalSteps;
    // Bump total steps so that we end up at the right point - the reason for this is that the
    // first command issued will be the start point (and the final one should be the end point)
    // but will be one increment fewer unless we increment one more time
    _totalSteps++;
    _inProgress = true;
    Log.trace("%sexecWorkItem Theta %F Rho %F CurTheta %F CurRho %F TotalSteps %d ThetaInc %F RhoInc %F AbsThetaDiffDegs %F StepAng %F\n", MODULE_PREFIX, 
            newTheta, newRho, _curTheta, _curRho, _totalSteps, _thetaInc, _rhoInc, absThetaDiffDegs, _stepAngleDegrees);
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

    // Inc
    _curTheta += _thetaInc;
    _curRho += _rhoInc;
    _curStep++;

    // Check complete
    if (_curStep >= _totalSteps)
    {
        Log.trace("%sservice finished\n", MODULE_PREFIX);
        _inProgress = false;
    }

}

void EvaluatorThetaRhoLine::stop()
{
    _inProgress = false;
}
