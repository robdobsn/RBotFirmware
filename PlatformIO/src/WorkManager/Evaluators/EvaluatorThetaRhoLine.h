// RBotFirmware
// Rob Dobson 2018

#pragma once

class WorkManager;
class WorkItem;

class EvaluatorThetaRhoLine
{
public:
    EvaluatorThetaRhoLine();

    // Config
    void setConfig(const char* configStr);
    const char* getConfig();

    // Is Busy
    bool isBusy();

    // Check valid
    bool isValid(WorkItem& workItem);

    // Process WorkItem
    bool execWorkItem(WorkItem& workItem);

    // Call frequently
    void service(WorkManager* pWorkManager);

    // Control
    void stop();

private:
    // Config
    double _stepAngle;
    bool _continueFromPrevious;

    // Pattern in progress
    bool _inProgress;

    // Pattern vars
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

};