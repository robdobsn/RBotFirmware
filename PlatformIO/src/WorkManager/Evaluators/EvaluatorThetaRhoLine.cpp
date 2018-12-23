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
    Log.trace("%ssetConfig StepAngleDegrees %F\n", MODULE_PREFIX, 
            _stepAngleDegrees);
    // TODO read from config?
    _maxAngle = M_PI / 64.0;
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

    /// new

    // Theta diff is minimal, no interpolation
    if (abs(thetaDiff) < _maxAngle) {
        _totalSteps = 1;
        _thetaInc = 0;
        _rhoInc = rhoDiff; //? not sure here
    } else {
        _thetaInc = _maxAngle;
        _rhoInc = _maxAngle / abs(thetaDiff) * (rhoDiff);
        _totalSteps = (int)((double)abs(thetaDiff) / _maxAngle);
    }

    /// old
    // _totalSteps = int(ceilf(abs(thetaDiff * 180 / M_PI / _stepAngleDegrees))) + 1;
    // _curStep = 0;
    // _thetaInc = thetaDiff / _totalSteps;
    // _rhoInc = rhoDiff / _totalSteps;

    ///
    _inProgress = true;
    Log.trace("%sexecWorkItem Theta %F Rho %F CurTheta %F CurRho %F TotalSteps %d ThetaInc %F RhoInc %F\n", MODULE_PREFIX, 
            newTheta, newRho, _curTheta, _curRho, _totalSteps, _thetaInc, _rhoInc);
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
    Log.verbose("%sservice %s\n", MODULE_PREFIX, lineBuf);

    // Inc
    _curTheta += _thetaInc;
    _curRho += _rhoInc;
    _curStep++;

    // Check complete
    if (_curStep >= _totalSteps)
    {
        Log.verbose("%sservice finished\n", MODULE_PREFIX);
        _inProgress = false;
    }

}

void EvaluatorThetaRhoLine::stop()
{
    _inProgress = false;
}
