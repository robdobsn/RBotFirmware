
#pragma push_macro("RD_DEBUG_FNAME")
#define RD_DEBUG_FNAME "TestWorkflowGCode.h"
#include "RdDebugLevel.h"

class TestWorkflowGCode
{
public:

    long millisRateIn = 2000;
    long millisRateOut = 4000;
    long lastMillisIn = 0;
    long lastMillisOut = 0;
    long lastMillisFlip = 0;
    long initialMemory = System.freeMemory();
    long lowestMemory = System.freeMemory();
    double curX = 0;
    double curY = 0;

    void testLoop(WorkflowManager& workflowManager, RobotController& robotController)
    {
        if (millis() > lastMillisIn + millisRateIn)
        {
            String cmdStr = "G01 X" + String(curX) + "Y" + String(curY);
            curY += 100;
            bool rslt = workflowManager.add(cmdStr);
            Log.info("Add %d", rslt);
            lastMillisIn = millis();
            if (lowestMemory > System.freeMemory())
                lowestMemory = System.freeMemory();
        }

        if (millis() > lastMillisOut + millisRateOut)
        {
            CommandElem cmdElem;
            bool rslt = workflowManager.get(cmdElem);
            Log.info("Get %d = %s, initMem %d, mem %d, lowMem %d", rslt,
                                cmdElem.getString().c_str(), initialMemory,
                                System.freeMemory(), lowestMemory);
            GCodeInterpreter::interpretGcode(cmdElem, robotController, true);
            lastMillisOut = millis();
        }

        if (millis() > lastMillisFlip + 30000)
        {
            if (millisRateOut > 3000)
                millisRateOut = 1000;
            else
                millisRateOut = 4000;
            lastMillisFlip = millis();
        }
    }
};

TestWorkflowGCode __testWorkflowGCode;

#pragma pop_macro("RD_DEBUG_FNAME")
