// RBotFirmware
// Rob Dobson 2017

#include "application.h"
#include "PatternGenerator.h"
#include "CommandInterpreter.h"
#include "math.h"

// Generalized spiral pattern

class PatternGeneratorModSpiral : public PatternGenerator
{
public:
    PatternGeneratorModSpiral()
    {
        _centreX = 0;
        _centreY = 0;
        _startRadius = 180;
        _endRadius = 20;
        _modulationAmp = 10;
        _modulationDampingFactor = 0.6;
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

    void service(CommandInterpreter* pCommandInterpreter)
    {
        // Check running
        if (!_isRunning)
            return;

        // Check if the command interpreter can accept new stuff
        if (pCommandInterpreter->canAcceptCommand())
            return;

        // Get next point and send to commandInterpreter
        double xy[2];
        getPoint(_tVal, xy);
        String cmdStr = String::format("G0 X%0.2f Y%0.2f", xy[0], xy[1]);
        pCommandInterpreter->process(cmdStr.c_str());
        Log.trace("CommandInterpreter process %s", cmdStr.c_str());
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
    double _modulationDampingFactor;
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
        Log.trace("curRadius %0.2f endRad %0.2f stRad %0.2f v1 %d v2 %d",
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
        double radiusProp = (curRadius-min(_startRadius,_endRadius))/fabs(_startRadius-_endRadius);
        double modDampFact = (1 - radiusProp) * _modulationDampingFactor;
        double modAmpl = curRadius + curModu * _modulationAmp * (1 - modDampFact);
        double posX = _centreX + modAmpl * sin(rRadians);
        double posY = _centreY + modAmpl * cos(rRadians);
        xy[0] = posX;
        xy[1] = posY;
    }

};
