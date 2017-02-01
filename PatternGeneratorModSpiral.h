// RBotFirmware
// Rob Dobson 2017

#include "application.h"
#include "PatternGenerator.h"
#include "math.h"

// Generalized spiral pattern

class PatternGeneratorModSpiral : public PatternGenerator
{
public:
    PatternGeneratorModSpiral()
    {
        _centreX = 0;
        _centreY = 0;
        _startRadius = 185;
        _endRadius = 20;
        _modulationAmp = 10;
        _modulationFreqMult = 20;
        _overRotationFactor = 1.03;
        _segmentsPerRev = 200;
        _outstepPerRev = -5;
        _isRunning = false;
        _tVal = 0;
        _tInc = 1;
        _patternName = "ModSpiral";
    }

    void setParameters(const char* drawParams)
    {
        _patternName = "ModSpiral";
    }

    void start()
    {
        _isRunning = true;
        _tVal = 0;
    }

    void service(WorkflowManager& workflowManager)
    {
        // Check running
        if (!_isRunning)
            return;

        // Check if there is space in the workflowManager
        if (workflowManager.isFull())
        {
            return;
        }

        // Get next point and add to workflow manager
        double xy[2];
        getPoint(_tVal, xy);
        String cmdStr = String::format("G0 X%0.2fY%0.2f", xy[0], xy[1]);
        workflowManager.add(cmdStr.c_str());
        Log.trace("WorkflowManager add %s", cmdStr.c_str());
        _tVal += _tInc;

        // Check if we reached a limit
        if (checkLimit(_tVal))
        {
            _tInc = -_tInc;
        }
    }

private:
    // Fixed pattern parameters
    double _centreX;
    double _centreY;
    double _startRadius;
    double _endRadius;
    double _modulationAmp;
    double _modulationFreqMult;
    double _overRotationFactor;
    double _segmentsPerRev;
    double _outstepPerRev;


    // State vars
    bool _isRunning;
    double _tVal;
    double _tInc;

private:
    bool checkLimit(double tVal)
    {
        double curRadius = _startRadius + _outstepPerRev * tVal / _segmentsPerRev;
        Serial.printlnf("curRadius %0.2f endRad %0.2f stRad %0.2f v1 %d v2 %d",
                curRadius, _endRadius, _startRadius,
                curRadius > max(_endRadius, _startRadius),
                curRadius < min(_endRadius, _startRadius));
        if (curRadius > max(_endRadius, _startRadius))
            return true;
        if (curRadius < min(_endRadius, _startRadius))
            return true;
        return false;
    }

    void getPoint(double tVal, double xy[])
    {
        // Compute point
        double rRadians = tVal * 2 * M_PI * _overRotationFactor / _segmentsPerRev;
        double curModu = sin(tVal * _modulationFreqMult * 2 * M_PI / _segmentsPerRev);
        double curRadius = _startRadius + _outstepPerRev * tVal / _segmentsPerRev;
        double posX = _centreX + (curRadius + curModu * _modulationAmp) * sin(rRadians);
        double posY = _centreY + (curRadius + curModu * _modulationAmp) * cos(rRadians);
        xy[0] = posX;
        xy[1] = posY;
    }

};
