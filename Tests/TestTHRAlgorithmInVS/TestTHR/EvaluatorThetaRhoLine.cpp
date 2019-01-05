// RBotFirmware
// Rob Dobson 2018

#include "EvaluatorThetaRhoLine.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string>

#define String string

#define THETA_RHO_DEBUG 1

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

// Process WorkItem
bool EvaluatorThetaRhoLine::execWorkItem(bool firstLine, double newTheta, double newRho)
{
	// Extract the details
#ifdef THETA_RHO_DEBUG
	//printf("%sexecWorkItem %f %f\n", MODULE_PREFIX, newTheta, newRho);
#endif
	if (firstLine)
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
		return false;
	}
	double deltaTheta = newTheta - _thetaStartOffset - _prevTheta;
	double absDeltaTheta = abs(deltaTheta);
	_thetaInc = deltaTheta >= 0 ? _stepAngle : -_stepAngle;
	double deltaRho = newRho - _prevRho;
	if (absDeltaTheta < _stepAngle)
	{
		_thetaInc = deltaTheta;
		_interpolateSteps = 1;
		_rhoInc = deltaRho;
	}
	else
	{
		_interpolateSteps = int(floor(absDeltaTheta / _stepAngle));
		if (_interpolateSteps < 1)
			return true;
		_rhoInc = deltaRho * _stepAngle / absDeltaTheta;
	}
	_curTheta = _prevTheta;
	_curRho = _prevRho;
	_prevTheta = newTheta;
	_prevRho = newRho;
	_curStep = 0;
	_inProgress = true;
#ifdef THETA_RHO_DEBUG
	char debugStr[200];
	sprintf_s(debugStr, 200, "Theta %8.6f Rho %8.6f CurTheta %8.6f CurRho %8.6f TotalSteps %d ThetaInc %8.6f RhoInc %8.6f AbsDeltaTheta %8.6f StepAng %8.6f",
					newTheta, newRho, _curTheta, _curRho, _interpolateSteps, _thetaInc, _rhoInc, absDeltaTheta, _stepAngle);
	printf("(index):1207 %s\n", debugStr);
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
			//printf("%sservice finished\n", MODULE_PREFIX);
#endif
			_inProgress = false;
			return;
		}

		// Check complete
		_curStep++;

		// Inc
		_curTheta += _thetaInc;
		_curRho += _rhoInc;

		// Next iteration
		char lineBuf[100];
		sprintf_s(lineBuf, 100, "G0 U%0.6f V%0.6f", roundf(_curTheta), roundf(_curRho));
#ifdef THETA_RHO_DEBUG
		printf("(index):1229 %s\n", lineBuf);
#endif
	}
}

void EvaluatorThetaRhoLine::stop()
{
	_inProgress = false;
}
