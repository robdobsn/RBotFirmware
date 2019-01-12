// RBotFirmware
// Rob Dobson 2018

#pragma once

class WorkManager;
class WorkItem;

class EvaluatorThetaRhoLine
{
public:
    EvaluatorThetaRhoLine(WorkManager& workManager);

    // Config
    void setConfig(const char* configStr, const char* robotAttributes);
    const char* getConfig();

    // Is Busy
    bool isBusy();

    // Check valid
    bool isValid(WorkItem& workItem);

    // Process WorkItem
    bool execWorkItem(WorkItem& workItem);

    // Call frequently
    void service();

    // Control
    void stop();

private:
    // Config
    const double DEFAULT_STEP_ANGLE = M_PI / 64;
    const double RHO_AT_DEFAULT_STEP_ANGLE = 0.5;
    double _stepAngle;
    bool _stepAdaptation;
    bool _continueFromPrevious;
    double _bedRadiusMM;
    double _centreOffsetX;
    double _centreOffsetY;

    // Work manager
    WorkManager& _workManager;

    // Pattern in progress
    bool _inProgress;

    // Pattern vars
    bool _isInterpolating;
    double _curTheta;
    double _curRho;
    int _interpolateSteps;
    int _curStep;
    double _thetaInc;
    double _rhoInc;
    double _thetaStartOffset;
    double _prevTheta;
    double _prevRho;

    // Process steps per service
    static const int PROCESS_STEPS_PER_SERVICE = 20;

    void calcXYPos(double theta, double rho, double& x, double& y);

};