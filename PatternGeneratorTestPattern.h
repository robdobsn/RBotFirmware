// RBotFirmware
// Rob Dobson 2017

#include "application.h"
#include "PatternGenerator.h"
#include "math.h"

// Test Pattern

class PatternGeneratorTestPattern : public PatternGenerator
{
public:
    PatternGeneratorTestPattern()
    {
        _tVal = 0;
        _tInc = 1;
        _patternName = "TestPattern";
    }

    void setParameters(const char* drawParams)
    {
        _patternName = "TestPattern";
    }

    void start()
    {
        _isRunning = true;
        _tVal = 0;
    }

    void service(CommandInterpreter& commandInterpreter)
    {
        // Check running
        if (!_isRunning)
            return;

        // Check if the command interpreter can accept new stuff
        if (!commandInterpreter.canAcceptCommand())
            return;

        // Get next point and send to commandInterpreter
        double xy[2];
        getPoint(_tVal, xy);
        String cmdStr = String::format("G0 X%0.2f Y%0.2f", xy[0], xy[1]);
        commandInterpreter.process(cmdStr.c_str());
        Log.trace("CommandInterpreter process %s", cmdStr.c_str());
        _tVal += _tInc;

        // Check if we reached a limit
        if (checkLimit(_tVal))
        {
            _tVal = 0;
            _isRunning = false;
        }
    }

private:
    // State vars
    bool _isRunning;
    double _tVal;
    double _tInc;

private:
    // Fixed pattern parameters
    static const int NUM_PTS = 4;

    bool checkLimit(double tVal)
    {
        if (tVal >= NUM_PTS)
            return true;
        return false;
    }

    void getPoint(double tVal, double xy[])
    {
        static const double _squareSide = 200;
        static const double pts[NUM_PTS][2] =
            {
                {_squareSide/2, _squareSide/2},
                {_squareSide/2, -_squareSide/2},
                {-_squareSide/2, -_squareSide/2},
                {-_squareSide/2, _squareSide/2}
            };

        // Compute point
        int idx = int(tVal);
        xy[0] = pts[0][idx];
        xy[1] = pts[1][idx];
        Log.trace("PatternGeneratorTestPattern getPoint tVal %0.2f, x %0.2f. y %0.2f ", tVal, xy[0], xy[1]);
    }

};
